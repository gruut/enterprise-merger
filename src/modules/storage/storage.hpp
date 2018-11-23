#ifndef GRUUT_ENTERPRISE_MERGER_STORAGE_HPP
#define GRUUT_ENTERPRISE_MERGER_STORAGE_HPP

#include <map>

#include "leveldb/db.h"
#include "leveldb/options.h"
#include "leveldb/write_batch.h"
#include "nlohmann/json.hpp"

namespace gruut {
using namespace std;
using namespace nlohmann;

class Storage {
public:
  Storage();

  ~Storage();

  void openDB(const string &path);

  void write(json &val, const string &what, string &blk_id);

  json findBy(const string &what, const string &id, const string &field);

private:
  void handleCriticalError(const leveldb::Status &status);

  void handleTrivialError(const leveldb::Status &status);

  string m_path;
  leveldb::Options m_options;
  leveldb::WriteOptions m_write_options;
  leveldb::ReadOptions m_read_options;
  leveldb::DB *m_db;
};
} // namespace gruut
#endif
