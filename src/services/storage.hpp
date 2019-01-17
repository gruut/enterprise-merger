#ifndef GRUUT_ENTERPRISE_MERGER_STORAGE_HPP
#define GRUUT_ENTERPRISE_MERGER_STORAGE_HPP

#include "leveldb/cache.h"
#include "leveldb/db.h"
#include "leveldb/options.h"
#include "leveldb/write_batch.h"
#include "nlohmann/json.hpp"

#include "../chain/merkle_tree.hpp"
#include "../chain/types.hpp"
#include "../utils/bytes_builder.hpp"
#include "../utils/rsa.hpp"
#include "../utils/safe.hpp"
#include "../utils/sha256.hpp"
#include "../utils/template_singleton.hpp"
#include "../utils/time.hpp"

#include "setting.hpp"

#include <botan-2/botan/asn1_time.h>
#include <botan-2/botan/auto_rng.h>
#include <botan-2/botan/data_src.h>
#include <botan-2/botan/exceptn.h>
#include <botan-2/botan/pkcs8.h>
#include <botan-2/botan/pubkey.h>
#include <botan-2/botan/rsa.h>
#include <botan-2/botan/x509cert.h>

#include <boost/filesystem/operations.hpp>
#include <cmath>
#include <iostream>
#include <map>
#include <sstream>

namespace gruut {
using namespace std;

const map<DBType, std::string> DB_PREFIX = {
    {DBType::BLOCK_HEADER, "b_"}, {DBType::BLOCK_HEIGHT, "h_"},
    {DBType::BLOCK_RAW, "r_"},    {DBType::BLOCK_LATEST, "l_"},
    {DBType::BLOCK_BODY, "o_"},   {DBType::BLOCK_CERT, "c_"}};

const vector<pair<string, string>> DB_BLOCK_HEADER_SUFFIX = {
    {"bID", "_bID"},   {"ver", "_ver"},         {"cID", "_cID"},
    {"time", "_time"}, {"hgt", "_hgt"},         {"SSig", "_SSig"},
    {"mID", "_mID"},   {"prevbID", "_prevbID"}, {"prevH", "_prevH"},
    {"txrt", "_txrt"}, {"txids", "_txids"}};

const vector<pair<string, string>> DB_TX_SUFFIX = {
    {"time", "_time"}, {"rID", "_rID"},         {"rSig", "_rSig"},
    {"type", "_type"}, {"content", "_content"}, {"bID", "_bID"},
    {"mPos", "_mPos"}};

class Storage : public TemplateSingleton<Storage> {
public:
  Storage();
  ~Storage();

  bool saveBlock(bytes &block_raw, json &block_header, json &block_body);
  bool saveBlock(const std::string &block_raw_b64, json &block_header,
                 json &block_body);
  std::pair<std::string, size_t> findLatestHashAndHeight();
  std::tuple<std::string, std::string, size_t> findLatestBlockBasicInfo();
  std::vector<std::string> findLatestTxIdList();
  std::string findCertificate(const std::string &user_id,
                              const timestamp_type &at_this_time = 0);
  std::string findCertificate(const signer_id_type &user_id,
                              const timestamp_type &at_this_time = 0);
  void deleteAllDirectory();
  read_block_type readBlock(size_t height);
  proof_type findSibling(const std::string &txid_b64);
  bool isDuplicatedTx(const std::string &txid_b64);

private:
  bool errorOnCritical(const leveldb::Status &status);
  bool errorOn(const leveldb::Status &status);
  bool addBatch(DBType what, const std::string &key, const std::string &value);
  bool putBlockHeader(json &block_json, const std::string &block_id_b64);
  bool putBlockHeight(json &block_json, const std::string &block_id_b64);
  bool putBlockRaw(bytes &block_raw, const std::string &block_id_b64);
  bool putLatestBlockHeader(json &block_json);
  bool putBlockBody(json &block_body_json, const std::string &block_id_b64);
  std::string getDataByKey(DBType what,
                           const std::string &base_suffix_keys = "");
  std::string getPrefix(DBType what);
  std::string parseCert(std::string &pem);
  void rollbackBatchAll();
  void commitBatchAll();
  void clearBatchAll();

private:
  string m_db_path;

  leveldb::Options m_options;
  leveldb::WriteOptions m_write_options;
  leveldb::ReadOptions m_read_options;

  leveldb::DB *m_db_block_header;
  leveldb::DB *m_db_block_raw;
  leveldb::DB *m_db_latest_block_header;
  leveldb::DB *m_db_block_body;
  leveldb::DB *m_db_certificate;
  leveldb::DB *m_db_blockid_height;

  leveldb::WriteBatch m_batch_block_header;
  leveldb::WriteBatch m_batch_block_raw;
  leveldb::WriteBatch m_batch_latest_block_header;
  leveldb::WriteBatch m_batch_block_body;
  leveldb::WriteBatch m_batch_certificate;
  leveldb::WriteBatch m_batch_blockid_height;
};
} // namespace gruut
#endif
