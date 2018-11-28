#ifndef GRUUT_ENTERPRISE_MERGER_STORAGE_HPP
#define GRUUT_ENTERPRISE_MERGER_STORAGE_HPP

#include "../../utils/template_singleton.hpp"
#include "leveldb/db.h"
#include "leveldb/options.h"
#include "leveldb/write_batch.h"
#include "nlohmann/json.hpp"
#include <boost/filesystem/operations.hpp>
#include <iostream>
#include <map>
#include <sstream>

namespace gruut {
using namespace std;
using namespace nlohmann;

class Storage : public TemplateSingleton<Storage> {
public:
  Storage();
  ~Storage();

  void saveBlock(const string &block_binary, json &block_header,
                 json &transaction);
  pair<string, string> findLatestHashAndHeight();
  vector<string> findLatestTxIdList();
  string findCertificate(const string &user_id);
  void deleteAllDirectory(const string &dir_path);

private:
  void handleCriticalError(const leveldb::Status &status);
  void handleTrivialError(const leveldb::Status &status);
  void write(const string &what, json &data, const string &block_id);
  string findBy(const string &what, const string &id, const string &field);
  int findTxIdPos(const string &blk_id, const string &tx_id);

private:
  leveldb::Options m_options;
  leveldb::WriteOptions m_write_options;
  leveldb::ReadOptions m_read_options;

  leveldb::DB *m_db_block_header;
  leveldb::DB *m_db_block_binary;
  leveldb::DB *m_db_latest_block_header;
  leveldb::DB *m_db_transaction;
  leveldb::DB *m_db_certificate;
};
} // namespace gruut
#endif
