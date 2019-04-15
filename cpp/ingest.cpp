#include "ingest.hpp"

int main(int argc, char **argv) {
  // check arguments
  if (argc != 2) {
    std::cout << "Usage: " << argv[0] << " path/to/file" << std::endl;
    return 1;
  }

  // open file
  std::ifstream f(argv[1], std::ios::binary);
  if (f.fail()) {
    f.close();
    std::cout << "Error: no such file " << argv[1] << std::endl;
    return 1;
  }
  // calculate id
  std::array<unsigned char, 16> id;
  picosha2::hash256(f, id.begin(), id.end());
  f.close();

  for (auto c : id) {
    std::cerr << std::hex << static_cast<int>(c);
  }
  std::cerr << std::endl;

  // check database if song already exists
  database song_db("songs.db");
  auto song = song_db.get_song(id);
  if (song != "") {
    std::cerr << "song is already in the database" << std::endl;
    return 1;
  }


  // read file data
  audio_helper ah;
  auto data = ah.read_from_file(argv[1]);

  // generate fingerpritns
  fingerprint fp;
  auto fingerprints = fp.get_fingerprints(data);

  std::cerr << "num fingerprints: " << std::dec << fingerprints.size() << std::endl;

  // put fingerprints in database
  database fp_db("fingerprints.db");
  for (auto &fp : fingerprints) {
    fp_data_t temp;
    std::copy(id.begin(), id.end(), temp.id);
    temp.t = fp.t;
    fp_db.put_fp(fp.fp, temp);
  }

  // put song into database
  song_db.put_song(id, argv[1]);

  return 0;
}
