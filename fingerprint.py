import numpy as np
from scipy import signal
from numpy.fft import fft
from bitarray import bitarray
import hashlib
import time
import os
import subprocess
from subprocess import DEVNULL
import sounddevice
from bsddb3 import db
import ujson
import functools

print = functools.partial(print, flush=True)

# constant parameters
FS = 4000                   # sampling rate of audio (Hz)
REC_FRAME_SIZE = 2000       # size of each stream buffer
THRESHOLD = 15              # min score required to be considered a match
THRESHOLD_RATE = 0.5        # min score per second required
THRESHOLD_RATE_MIN_TIME = 3 # min time for THRESHOLD_RATE to become active (sec)
THRESHOLD_RATE_MIN = 10      # min score required at THRESHOLD_RATE_MIN_TIME
TIMEOUT = 15                # max duration to listen for (sec)
TOGGLE_BITS = 16            # number of bits to fuzz when searching fingerprint matches

# global variables
id_error = False
id_found = False
id_hash = ""
id_etime = 0
id_score = 0
start = 0
callback_idx = 0


# bandpass as recommended by MASK paper (actually just a hi-pass)
# 0.3 kHz cutoff
def bandpass(data):
    b = signal.firls(25, [0, 250, 300, REC_FRAME_SIZE], [0, 0, 1, 1], fs=FS)
    return signal.lfilter(b, 1, data)

# compute stft (spectrogram)
def stft(data, win_len=100, win_overlap=10):
    frame_start = 0

    win_len_frames = FS * win_len // 1000
    win_overlap_frames = FS * win_overlap // 1000

    num_frames = data.shape[0] // win_overlap_frames

    stft_out = np.zeros((num_frames, win_len_frames//2))
    frame = np.zeros(win_len_frames)
    fft_frame = np.zeros(win_len_frames)

    for i in range(num_frames):
        frame_start = i * win_overlap_frames
        win = np.hamming(win_len_frames)
        if frame_start + win_len_frames >= data.shape[0]:
            frame[0:data.shape[0]-frame_start] = data[frame_start:]
            frame = frame * win
        else:
            frame = win * data[frame_start : frame_start + win_len_frames]
        fft_frame = abs(fft(frame))
        stft_out[i,:] = np.square(np.abs(fft_frame[:win_len_frames//2])) / win_len_frames

    return stft_out

# compute mel filterbank
def compute_mel(data, nbins=18):
    low_mfreq = 0
    hi_mfreq = 2595 * np.log10(1 + FS/2/700)
    m_points = np.linspace(low_mfreq, hi_mfreq, nbins + 2)
    hz_points = (700 * (10**(m_points / 2595) - 1))

    bin = (FS*100//1000 + 1) * hz_points / FS
    f_bank = np.zeros((nbins, FS*100//1000 // 2))

    for m in range(1, nbins + 1):
        f_m_minus = int(bin[m - 1])   # left
        f_m = int(bin[m])             # center
        f_m_plus = int(bin[m + 1])    # right

        for k in range(f_m_minus, f_m):
            f_bank[m - 1, k] = (k - bin[m - 1]) / (bin[m] - bin[m - 1])
        for k in range(f_m, f_m_plus):
            f_bank[m - 1, k] = (bin[m + 1] - k) / (bin[m + 1] - bin[m])

    f_bank = np.where(f_bank < 0, 0, f_bank)

    filter_banks = np.dot(data, f_bank.T)
    filter_banks = np.where(filter_banks <= 0, np.finfo(float).eps, filter_banks)
    return 20 * np.log10(filter_banks)

# find peaks, compute fingerprints, insert or query database
def find_peaks(data, query=True, ID=None):
    if not query and ID == None:
        raise Exception("Error: ID must be specified when adding song")

    matches = {}

    fpdb = db.DB()
    fpdb.set_cachesize(0, 536870912, 1)
    fpdb.open('mask_fp.bdb', None, db.DB_HASH, db.DB_CREATE)

    num_fp = 0

    for t in range(9, data.shape[0]-9):
        for b in range(1, data.shape[1]-1): # bands 1-16 (inclusive)
            # test neighbors
            if data[t,b] <= data[t-1][b] or data[t,b] <= data[t+1][b]:
                continue
            if data[t,b] <= data[t][b+1] or data[t,b] <= data[t][b+1]:
                continue

            # compute region values
            # region 1
            r_1a = (data[t-9][b] + data[t-8][b] + data[t-7][b]) / 3
            r_1b = (data[t-7][b] + data[t-6][b] + data[t-5][b]) / 3
            r_1c = (data[t-5][b] + data[t-4][b] + data[t-3][b]) / 3
            r_1d = (data[t-3][b] + data[t-2][b] + data[t-1][b]) / 3
            r_1e = (data[t+1][b] + data[t-2][b] + data[t-3][b]) / 3
            r_1f = (data[t+3][b] + data[t-4][b] + data[t-5][b]) / 3
            r_1g = (data[t+5][b] + data[t-6][b] + data[t-7][b]) / 3
            r_1h = (data[t+7][b] + data[t-8][b] + data[t-8][b]) / 3

            # region 2
            if b == data.shape[1] - 2:
                r_2a = (data[t-1][b+1] + data[t][b+1] + data[t+1][b+1]) / 3
            else:
                r_2a = (data[t-1][b+2] + data[t][b+2] + data[t+1][b+2]) / 3
            r_2b = (data[t-1][b+1] + data[t][b+1] + data[t+1][b+1]) / 3
            r_2c = (data[t-1][b-1] + data[t][b-1] + data[t+1][b-1]) / 3
            if b == 1:
                r_2d = (data[t-1][b-1] + data[t][b-1] + data[t+1][b-1]) / 3
            else:
                r_2d = (data[t-1][b-2] + data[t][b-2] + data[t+1][b-2]) / 3

            # region 3
            r_3a = (data[t-2][b+1] + data[t-1][b+1] + data[t][b+1] + data[t-2][b] + data[t-1][b]) / 5
            r_3b = (data[t][b+1] + data[t+1][b+1] + data[t+2][b+1] + data[t+1][b] + data[t+2][b]) / 5
            r_3c = (data[t+1][b] + data[t+2][b] + data[t][b-1] + data[t+1][b-1] + data[t+2][b-1]) / 5
            r_3d = (data[t-2][b] + data[t-1][b] + data[t-2][b-1] + data[t-1][b-1] + data[t][b-1]) / 5

            # region 4
            if b == data.shape[1] - 2:
                r_4a = (data[t-9][b+1] + data[t-8][b+1] + data[t-7][b+1] + data[t-6][b+1]) / 4
                r_4b = (data[t-5][b+1] + data[t-4][b+1] + data[t-3][b+1] + data[t-2][b+1]) / 4
                r_4e = (data[t+5][b+1] + data[t+4][b+1] + data[t+3][b+1] + data[t+2][b+1]) / 4
                r_4f = (data[t+9][b+1] + data[t+8][b+1] + data[t+7][b+1] + data[t+6][b+1]) / 4
            else:
                r_4a = (data[t-9][b+2] + data[t-8][b+2] + data[t-7][b+2] + data[t-6][b+2] + data[t-9][b+1] + data[t-8][b+1] + data[t-7][b+1] + data[t-6][b+1]) / 8
                r_4b = (data[t-5][b+2] + data[t-4][b+2] + data[t-3][b+2] + data[t-2][b+2] + data[t-5][b+1] + data[t-4][b+1] + data[t-3][b+1] + data[t-2][b+1]) / 8
                r_4e = (data[t+5][b+2] + data[t+4][b+2] + data[t+3][b+2] + data[t+2][b+2] + data[t+5][b+1] + data[t+4][b+1] + data[t+3][b+1] + data[t+2][b+1]) / 8
                r_4f = (data[t+9][b+2] + data[t+8][b+2] + data[t+7][b+2] + data[t+6][b+2] + data[t+9][b+1] + data[t+8][b+1] + data[t+7][b+1] + data[t+6][b+1]) / 8
            if b == 1:
                r_4c = (data[t-9][b-1] + data[t-8][b-1] + data[t-7][b-1] + data[t-6][b-1]) / 4
                r_4d = (data[t-5][b-1] + data[t-4][b-1] + data[t-3][b-1] + data[t-2][b-1]) / 4
                r_4g = (data[t+5][b-1] + data[t+4][b-1] + data[t+3][b-1] + data[t+2][b-1]) / 4
                r_4h = (data[t+9][b-1] + data[t+8][b-1] + data[t+7][b-1] + data[t+6][b-1]) / 4
            else:
                r_4c = (data[t-9][b-2] + data[t-8][b-2] + data[t-7][b-2] + data[t-6][b-2] + data[t-9][b-1] + data[t-8][b-1] + data[t-7][b-1] + data[t-6][b-1]) / 8
                r_4d = (data[t-5][b-2] + data[t-4][b-2] + data[t-3][b-2] + data[t-2][b-2] + data[t-5][b-1] + data[t-4][b-1] + data[t-3][b-1] + data[t-2][b-1]) / 8
                r_4g = (data[t+5][b-2] + data[t+4][b-2] + data[t-3][b-2] + data[t+2][b-2] + data[t+5][b-1] + data[t+4][b-1] + data[t+3][b-1] + data[t+2][b-1]) / 8
                r_4h = (data[t+9][b-2] + data[t+8][b-2] + data[t+7][b-2] + data[t+6][b-2] + data[t+9][b-1] + data[t+8][b-1] + data[t+7][b-1] + data[t+6][b-1]) / 8


            # compute mask
            energy_diffs = [False for _ in range(22)]

            # horizontal max
            energy_diffs[0] = r_1a - r_1b
            energy_diffs[1] = r_1b - r_1c
            energy_diffs[2] = r_1c - r_1d
            energy_diffs[3] = r_1d - r_1e
            energy_diffs[4] = r_1e - r_1f
            energy_diffs[5] = r_1f - r_1g
            energy_diffs[6] = r_1g - r_1h

            # vertical max
            energy_diffs[7] = r_2a - r_2b
            energy_diffs[8] = r_2b - r_2c
            energy_diffs[9] = r_2c - r_2d

            # intermediate quadrants
            energy_diffs[10] = r_3a - r_3b
            energy_diffs[11] = r_3d - r_3c
            energy_diffs[12] = r_3a - r_3d
            energy_diffs[13] = r_3b - r_3c

            # extended quadrants 1
            energy_diffs[14] = r_4a - r_4b
            energy_diffs[15] = r_4c - r_4d
            energy_diffs[16] = r_4e - r_4f
            energy_diffs[17] = r_4g - r_4h

            # extended quardants 2
            energy_diffs[18] = r_4a + r_4b - r_4c + r_4d
            energy_diffs[19] = r_4e + r_4f - r_4g + r_4h
            energy_diffs[20] = r_4c + r_4d - r_4e + r_4f
            energy_diffs[21] = r_4a + r_4b - r_4g + r_4h

            # calculate bits
            bits = [x > 0 for x in energy_diffs]

            # calculate binary representation of band
            band_bits = [True for _ in range(4)]
            band_bits[0] = False if (b-1) & 0x8 == 0 else True
            band_bits[1] = False if (b-1) & 0x4 == 0 else True
            band_bits[2] = False if (b-1) & 0x2 == 0 else True
            band_bits[3] = False if (b-1) & 0x1 == 0 else True

            fp = bitarray(band_bits + bits)
            fp_int = int(fp.to01(),2)
            if query: # lookup in database
                # toggle bits in increasing order of reliability
                reliability = sorted(range(len(energy_diffs)),
                        key=lambda k: abs(energy_diffs[k]))
                for idx_1 in range(-1, TOGGLE_BITS):
                    for idx_2 in range(-1, TOGGLE_BITS):
                        if idx_1 == idx_2:
                            continue

                        temp_fp = fp_int
                        if idx_1 != -1:
                            temp_fp ^= 1 << (reliability[idx_1])
                        if idx_2 != -1:
                            temp_fp ^= 1 << (reliability[idx_2])

                        # lookup in database
                        temp_k = bytes(ujson.dumps(temp_fp), 'utf-8')
                        temp = fpdb.get(temp_k)

                        if temp == None:
                            continue

                        temp = ujson.loads(temp.decode('utf-8'))

                        # add values to list
                        for match in temp:
                            if match[0] not in matches:
                                matches[match[0]] = [[],[]]
                            matches[match[0]][0].append(match[1])
                            matches[match[0]][1].append(t)
            else: # add to database
                prep = (ID, t)
                temp_k = bytes(ujson.dumps(fp_int), 'utf-8')
                temp = fpdb.get(temp_k)
                if temp == None:
                    temp = [prep]
                else:
                    temp = ujson.loads(temp.decode('utf-8'))
                    temp.append(prep)

                temp_v = bytes(ujson.dumps(temp), 'utf-8')

                fpdb.put(temp_k, temp_v)
                num_fp += 1

    fpdb.close()


    if query:
        return matches
    else:
        # update stats database
        statsdb = db.DB()
        statsdb.open('mask_stats.bdb', None, db.DB_HASH, db.DB_CREATE)
        temp_k = bytes('num_fp', 'utf-8')
        temp = statsdb.get(temp_k)
        if temp == None:
            temp = num_fp
        else:
            temp = int(temp.decode('utf-8'))
            temp += num_fp
        temp_v = bytes(str(temp), 'utf-8')
        statsdb.put(temp_k, temp_v)
        statsdb.close()
        return None

# generate fingerprints for data
def fingerprint(data, query=True, ID=None):
    bandpass_out = bandpass(data)
    stft_out = stft(bandpass_out)
    mels = compute_mel(stft_out)
    matches = find_peaks(mels, query=query, ID=ID)
    return matches

# add a song or directory to the reference database
def add_song(path):
    if os.path.isdir(path):
        for root, _, filenames in os.walk(path):
            for filename in filenames:
                try:
                    add_one_song(os.path.join(root, filename))
                except KeyError:
                    print('skipping {}'.format(filename))
                    continue
                except:
                    print('bad file {}'.format(filename))
                    continue
                print('added    {}'.format(filename))

                # updates stats database
                statsdb = db.DB()
                statsdb.open('mask_stats.bdb', None, db.DB_HASH, db.DB_CREATE)
                temp_k = bytes('num_songs', 'utf-8')
                temp = statsdb.get(temp_k)
                if temp == None:
                    temp = 1
                else:
                    temp = int(temp.decode('utf-8'))
                    temp += 1
                temp_v = bytes(str(temp), 'utf-8')
                statsdb.put(temp_k, temp_v)
                statsdb.close()
        pass
    elif os.path.isfile(path):
        add_one_song(path)
        pass
    else:
        raise Exception('invalid path')

# add a single song to the database
def add_one_song(path):
    path = os.path.abspath(path);

    # read file and convert to mono
    data = read_file(path)

    # generate ID based on the data
    h = hashlib.sha256(data.view()).hexdigest()

    # set up database
    sdb = db.DB()
    sdb.open('mask_s.bdb', None, db.DB_HASH, db.DB_CREATE)

    # test if song is already in the database
    if sdb.get(bytes(h, 'utf-8')):
        raise KeyError
    
    # add song to songs table
    sdb.put(bytes(h, 'utf-8'), bytes(path, 'utf-8'))

    # close sqlite
    sdb.close()

    # calculate fingerprints and add to fingerprints table
    fingerprint(data, query=False, ID=h)

    return


# read an input file into a numpy array
def read_file(path):
    command = [ 'ffmpeg',
        '-i', path,
        '-f', 's16le',
        '-acodec', 'pcm_s16le',
        '-ar', str(FS),
        '-ac', '1',
        '-']
    raw_audio = subprocess.check_output(command, stderr=DEVNULL)
    data = np.fromstring(raw_audio, dtype="int16")
    return data

# spin until a song is identified
def identify_song():
    while True:
        if id_error:
            raise LookupError
        if id_found:
            return id_hash, id_score, id_etime
        time.sleep(.1)


# process audio frames in real time
def audio_callback(indata, frames, callback_time, status):
    global hists
    global id_error
    global id_hash
    global id_score
    global id_etime
    global id_found
    global callback_idx

    if id_found:
        return

    if status.input_underflow:
        print('no input detected!')

    elapsed = callback_idx * frames * 100 // FS
    elapsed_sec = callback_idx * frames / FS
    callback_idx += 1

    # get all fingerprint matches
    matches = fingerprint(indata[:,0], query=True)

    temp_found = False

    for m in matches:
        # fetch previous histograms
        if m not in hists:
            hist = {}   # histogram of dts
        else:
            hist = hists[m]

        for i in range(len(matches[m][0])):
            # calculate dt
            dt = matches[m][0][i] - matches[m][1][i]
            dt -= elapsed

            if dt not in hist:
                hist[dt] = 0

            hist[dt] += 1

            # check if score is above defined threshold
            if hist[dt] >= THRESHOLD:
                temp_found = True
            elif elapsed_sec >= THRESHOLD_RATE_MIN_TIME \
                    and hist[dt] >= (elapsed_sec - THRESHOLD_RATE_MIN_TIME) * \
                        THRESHOLD_RATE + THRESHOLD_RATE_MIN:
                temp_found = True
            if temp_found and hist[dt] > id_score:
                id_score = hist[dt]
                id_hash = m
                id_etime = dt

        # save histograms
        hists[m] = hist

    # timeout
    if elapsed_sec >= TIMEOUT:
        id_error = True
        return

    # set global flag if a match is found
    if temp_found:
        id_found = True

    print('.', end='')


# returns the name of a song given its id
def get_song_info(song_id):
    sdb = db.DB()
    sdb.open('mask_s.bdb', None, db.DB_HASH, db.DB_CREATE)
    name = sdb.get(bytes(song_id, 'utf-8'))
    sdb.close()
    return name

# return a stream from the microphone
def record_mic():
    print('Listening', end='')
    stream = sounddevice.InputStream(samplerate=FS, channels=1,
            blocksize=REC_FRAME_SIZE, callback=audio_callback)
    return stream

# attempt to identify audio
def test():
    global start
    global hists
    global id_error
    global id_hash
    global id_score
    global id_etime
    global id_found
    global callback_idx

    start = 0
    hists = {}
    id_error = False
    id_hash = ""
    id_score = 0
    id_etime = 0
    id_found = False
    callback_idx = 0

    # open stream from microphne
    stream = record_mic()
    with stream:
        start = time.time()
        stream.start()
        try:
            song_id, score, elapsed_bin = identify_song()
        except LookupError:
            print('No matching song found')
            return
        stream.stop()

    # find song info
    filename = os.path.basename(get_song_info(song_id).decode('utf-8'))

    # calculate times
    end = time.time()
    execution_time = end - start

    elapsed_time = elapsed_bin * 0.01 + execution_time
    elapsed_time_m = int(elapsed_time / 60)
    elapsed_time_s = int(elapsed_time) % 60

    recorded_time = callback_idx * REC_FRAME_SIZE / FS

    # output results
    print()
    print('id:             {}'.format(song_id))
    print('name:           {}'.format(filename))
    print('score:          {}'.format(score))
    print('elapsed time:   {}:{:02d}'.format(elapsed_time_m, elapsed_time_s))
    print('execution time: {:0.3f}s'.format(execution_time))
    print('recorded time:  {:0.3f}s'.format(recorded_time))
