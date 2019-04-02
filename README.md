# MASK Audio Fingerprinting and Song Identification

An implementation of the [MASK audio fingerprint algorithm](https://www.researchgate.net/publication/261131744_MASK_Robust_Local_Features_for_Audio_Fingerprinting).

![gif of song identification](/images/mask.gif)

## Dependencies

### Python

- python 3
- numpy
- scipy
- bitarray
- sounddevice
- bsddb3
- ujson

### Other

- ffmpeg
- berkeley db

## Usage


### Add songs to the database

```
python insert.py path/to/files/or/directory
```

### Identify a song

```
python identify.py
```

### See number of songs and fingerprints stored

```
python stats.py
```
