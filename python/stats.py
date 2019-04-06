from bsddb3 import db

statsdb = db.DB()
statsdb.open('mask_stats.bdb', None, db.DB_HASH)
num_songs = statsdb.get(bytes('num_songs', 'utf-8')).decode('utf-8')
num_fp = statsdb.get(bytes('num_fp', 'utf-8')).decode('utf-8')
statsdb.close()

print('songs:        {}'.format(num_songs))
print('fingerprints: {}'.format(num_fp))
