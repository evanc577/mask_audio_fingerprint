#include "ingest.hpp"

int insert_file(const std::string &fullpath) {
  size_t last_slash_idx = fullpath.find_last_of('/');
  std::string filename;
  if (last_slash_idx != std::string::npos) {
    filename = fullpath.substr(last_slash_idx + 1);
  } else {
    filename = fullpath;
  }

  // open file
  std::ifstream f(fullpath, std::ios::binary);
  if (f.fail()) {
    f.close();
    std::cout << "Error: no such file " << fullpath << std::endl;
    return 1;
  }
  // calculate id
  std::array<unsigned char, 16> id;
  picosha2::hash256(f, id.begin(), id.end());
  f.close();

  // check database if song already exists
  database song_db("songs.db");
  auto song = song_db.get_song(id);
  if (song != "") {
    std::cerr << "Skipping \"" << fullpath << "\"" << std::endl;
    return 1;
  }

  // read file data
  audio_helper ah;
  auto data = ah.read_from_file(fullpath);
  if (data.size() == 0) {
    std::cerr << "Bad file \"" << fullpath << "\"" << std::endl;
    return 1;
  }

  // generate fingerpritns
  fingerprint fp;
  auto fingerprints = fp.get_fingerprints(data);

  // put fingerprints in database
  database fp_db("fingerprints.db");
  for (auto &fp : fingerprints) {
    fp_data_t temp;
    std::copy(id.begin(), id.end(), temp.id);
    temp.t = fp.t;
    fp_db.put_fp(fp.fp, temp);
  }

  // put song into database
  song_db.put_song(id, filename);

  std::cerr << "Inserted \"" << fullpath << "\"" << std::endl;

  return 0;
}

int main(int argc, char **argv) {
  // check arguments
  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " path/to/file [..]" << std::endl;
    return 1;
  }
    
  int ret = 0;
  for (int i = 1; i < argc; ++i) {
      std::string fullpath(argv[i]);
      ret |= insert_file(fullpath);
  }

  return ret;
}

