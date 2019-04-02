from fingerprint import add_song
import argparse

parser = argparse.ArgumentParser(description='Add a song or directory of songs to the reference database')
parser.add_argument('path', help='path/to/song/or/directory')
args = parser.parse_args()

add_song(args.path)
