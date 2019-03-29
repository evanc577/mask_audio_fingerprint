import numpy as np
from scipy import signal
from scipy.io.wavfile import read
from IPython.display import Audio
import matplotlib.pyplot as plt
from numpy.fft import fft
from bitarray import bitarray
import sqlite3
import hashlib
import time

def downsample_4k(data, fs):
    return signal.resample_poly(data, 1, 11)

def bandpass(data):
    b = signal.firls(25, [0, 250, 300, 2000], [0, 0, 1, 1], fs=4000)
    return signal.lfilter(b, 1, data)

def stft(data, win_len=100, win_overlap=10):
    frame_start = 0

    win_len_frames = 4000 * win_len // 1000
    win_overlap_frames = 4000 * win_overlap // 1000

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

def compute_mel(data, nbins=18):
    low_mfreq = 0
    hi_mfreq = 2595 * np.log10(1 + 2000/700)
    m_points = np.linspace(low_mfreq, hi_mfreq, nbins + 2)
    hz_points = (700 * (10**(m_points / 2595) - 1))

    bin = (4000*100//1000 + 1) * hz_points / 4000
    f_bank = np.zeros((nbins, 4000*100//1000 // 2))

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

def find_peaks(data, query=True, ID=None):
    if not query and ID == None:
        raise Exception("Error: ID must be specified when adding song")

    matches = {}
    conn = sqlite3.connect('mask.db')
    c = conn.cursor()

    peaks = [[] for i in range(data.shape[1])]

    for t in range(9, data.shape[0]-9):
        for b in range(1, data.shape[1]-1):
            # test neighbors
            if data[t,b] <= data[t-1][b] or data[t,b] <= data[t+1][b]:
                continue
            elif data[t,b] <= data[t][b+1] or data[t,b] <= data[t][b+1]:
                continue

            # add peak if it's the first one in band
            # if len(peaks[b]) == 0 or True:
            peaks[b].append(t)
            # continue

            # check threshold
            # skip for now

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


            bits = [False for _ in range(22)]
            # compute bits

            # horizontal max
            bits[0] = r_1a > r_1b
            bits[1] = r_1b > r_1c
            bits[2] = r_1c > r_1d
            bits[3] = r_1d > r_1e
            bits[4] = r_1e > r_1f
            bits[5] = r_1f > r_1g
            bits[6] = r_1g > r_1h

            # vertical max
            bits[7] = r_2a > r_2b
            bits[8] = r_2b > r_2c
            bits[9] = r_2c > r_2d

            # intermediate quadrants
            bits[10] = r_3a > r_3b
            bits[11] = r_3d > r_3c
            bits[12] = r_3a > r_3d
            bits[13] = r_3b > r_3c

            # extended quadrants 1
            bits[14] = r_4a > r_4b
            bits[15] = r_4c > r_4d
            bits[16] = r_4e > r_4f
            bits[17] = r_4g > r_4h

            # extended quardants 2
            bits[18] = r_4a + r_4b > r_4c + r_4d
            bits[19] = r_4e + r_4f > r_4g + r_4h
            bits[20] = r_4c + r_4d > r_4e + r_4f
            bits[21] = r_4a + r_4b > r_4g + r_4h

            # calculate binary representation of band
            band_bits = [True for _ in range(4)]
            band_bits[0] = False if (b-1) & 0x8 == 0 else True
            band_bits[1] = False if (b-1) & 0x4 == 0 else True
            band_bits[2] = False if (b-1) & 0x2 == 0 else True
            band_bits[3] = False if (b-1) & 0x1 == 0 else True

            fp = bitarray(band_bits + bits)
            if query: # lookup in database
                prep = (int(fp.to01(),2),)
                c.execute('SELECT * FROM fingerprints WHERE fp=?', prep)
                temp = c.fetchall()
                for match in temp:
                    if match[1] not in matches:
                        matches[match[1]] = [[],[]]
                    matches[match[1]][0].append(match[2])
                    matches[match[1]][1].append(t)
            else: # add query to database
                prep = (int(fp.to01(),2), ID, t)        
                c.execute('INSERT INTO fingerprints VALUES(?, ?, ?)', prep)


    conn.commit()
    conn.close()
    if query:
        return matches
    else:
        return None


def add_song(name):
    conn = sqlite3.connect('mask.db')
    c = conn.cursor()

    m = hashlib.sha256()
    m.update(name.encode('UTF-8'))
    h = m.hexdigest()
    # h = hash(name)
    prep = (h,)
    c.execute('SELECT * FROM songs WHERE id=?', prep)
    if c.fetchone() != None:
        raise Exception('song already exists in database')
    
    prep = (h, name)
    c.execute('INSERT INTO songs VALUES(?, ?)', prep)
    conn.commit()
    conn.close()
    return h

def fingerprint(data, fs, query=True, ID=None):
    data_2 = downsample_4k(data, fs)
    data_3 = bandpass(data_2)
    stft_out = stft(data_3)
    mels = compute_mel(stft_out)
    matches = find_peaks(mels, query=query, ID=ID)
    return matches
