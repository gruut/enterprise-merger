#include "storage.hpp"
#include "easy_logging.hpp"

namespace gruut {

using namespace std;

Storage::Storage() {
  el::Loggers::getLogger("STRG");

  auto setting = Setting::getInstance();
  m_db_path = setting->getMyDbPath();

  m_options.block_cache = leveldb::NewLRUCache(100 * 1048576); // 100MB cache
  m_options.create_if_missing = true;
  m_write_options.sync = true;

  boost::filesystem::create_directories(m_db_path);

  // clang-format off
  errorOnCritical(leveldb::DB::Open(m_options, m_db_path + "/" + config::DB_SUB_DIR_HEADER, &m_db_block_header));
  errorOnCritical(leveldb::DB::Open(m_options, m_db_path + "/" + config::DB_SUB_DIR_RAW, &m_db_block_raw));
  errorOnCritical(leveldb::DB::Open(m_options, m_db_path + "/" + config::DB_SUB_DIR_LATEST, &m_db_latest_block_header));
  errorOnCritical(leveldb::DB::Open(m_options, m_db_path + "/" + config::DB_SUB_DIR_TRANSACTION, &m_db_transaction));
  errorOnCritical(leveldb::DB::Open(m_options, m_db_path + "/" + config::DB_SUB_DIR_IDHEIGHT, &m_db_blockid_height));
  errorOnCritical(leveldb::DB::Open(m_options, m_db_path + "/" + config::DB_SUB_DIR_LEDGER, &m_db_ledger));
  errorOnCritical(leveldb::DB::Open(m_options, m_db_path + "/" + config::DB_SUB_DIR_BACKUP,&m_db_backup));
  // clang-format on
}

Storage::~Storage() {
  delete m_db_block_header;
  delete m_db_block_raw;
  delete m_db_latest_block_header;
  delete m_db_transaction;
  delete m_db_blockid_height;
  delete m_db_ledger;
  delete m_db_backup;

  m_db_block_header = nullptr;
  m_db_block_raw = nullptr;
  m_db_latest_block_header = nullptr;
  m_db_transaction = nullptr;
  m_db_blockid_height = nullptr;
  m_db_ledger = nullptr;
  m_db_backup = nullptr;
}

bool Storage::saveBlock(bytes &block_raw, json &block_header,
                        json &block_transaction) {
  string block_id_b64 = Safe::getString(block_header, "bID");

  if (putBlockHeader(block_header, block_id_b64) &&
      putBlockHeight(block_header, block_id_b64) &&
      putLatestBlockHeader(block_header) &&
      putTransaction(block_transaction, block_id_b64) &&
      putBlockRaw(block_raw, block_id_b64)) {
    commitBatchAll();

    // CLOG(INFO, "STRG") << "Success to save block";

    return true;
  }

  CLOG(ERROR, "STRG") << "Failed to save block";

  rollbackBatchAll();
  return false;
}

bool Storage::saveBlock(const string &block_raw_b64, json &block_header,
                        json &block_transaction) {
  bytes block_raw = TypeConverter::decodeBase64(block_raw_b64);
  return saveBlock(block_raw, block_header, block_transaction);
}

std::string Storage::getPrefix(DBType what) {
  std::string prefix;
  auto it_map = DB_PREFIX.find(what);
  if (it_map != DB_PREFIX.end()) {
    prefix = it_map->second;
  }
  return prefix;
}

bool Storage::addBatch(DBType what, const string &base_suffix_key,
                       const string &value) {
  string key = getPrefix(what) + base_suffix_key;

  switch (what) {
  case DBType::BLOCK_HEADER:
    m_batch_block_header.Put(key, value);
    break;
  case DBType::BLOCK_HEIGHT:
    m_batch_blockid_height.Put(key, value);
    break;
  case DBType::BLOCK_RAW:
    m_batch_block_raw.Put(key, value);
    break;
  case DBType::BLOCK_LATEST:
    m_batch_latest_block_header.Put(key, value);
    break;
  case DBType::TRANSACTION:
    m_batch_transaction.Put(key, value);
    break;
  case DBType::LEDGER:
    m_batch_ledger.Put(key, value);
    break;
  case DBType::BLOCK_BACKUP:
    m_batch_backup.Put(key, value);
    break;
  default:
    break;
  }
  return true;
}

void Storage::commitBatchAll() {
  m_db_block_header->Write(m_write_options, &m_batch_block_header);
  m_db_blockid_height->Write(m_write_options, &m_batch_blockid_height);
  m_db_block_raw->Write(m_write_options, &m_batch_block_raw);
  m_db_latest_block_header->Write(m_write_options,
                                  &m_batch_latest_block_header);
  m_db_transaction->Write(m_write_options, &m_batch_transaction);

  clearBatchAll();
}

void Storage::rollbackBatchAll() { clearBatchAll(); }

void Storage::clearBatchAll() {
  m_batch_block_header.Clear();
  m_batch_block_raw.Clear();
  m_batch_latest_block_header.Clear();
  m_batch_transaction.Clear();
  m_batch_blockid_height.Clear();
}

bool Storage::putBlockHeader(json &block_header_json,
                             const string &block_id_b64) {
  std::string key, value;
  for (auto &item : DB_BLOCK_HEADER_SUFFIX) {
    if (item.first == "SSig") {
      if (!block_header_json[item.first].is_array())
        return false;

      key = block_id_b64 + item.second + "_c";
      value = TypeConverter::bytesToString(
          json::to_cbor(block_header_json[item.first]));
      if (!addBatch(DBType::BLOCK_HEADER, key, value))
        return false;

      key = block_id_b64 + item.second + "_n";
      value = to_string(block_header_json[item.first].size());
      if (!addBatch(DBType::BLOCK_HEADER, key, value))
        return false;

    } else {
      key = block_id_b64 + item.second;

      if (item.first == "txids") {
        if (!block_header_json[item.first].is_array())
          return false;

        value = block_header_json[item.first].dump();
      } else
        value = Safe::getString(block_header_json, item.first);
      if (!addBatch(DBType::BLOCK_HEADER, key, value))
        return false;
    }
  }
  return true;
}

bool Storage::putBlockHeight(json &block_header_json,
                             const string &block_id_b64) {
  std::string key = Safe::getString(block_header_json, "hgt");
  return addBatch(DBType::BLOCK_HEIGHT, key, block_id_b64);
}

bool Storage::putBlockRaw(bytes &block_raw, const string &block_id_b64) {

  std::string key, value;

  key = block_id_b64;
  value = TypeConverter::bytesToString(block_raw);
  if (!addBatch(DBType::BLOCK_RAW, key, value))
    return false;

  key = block_id_b64 + "_hash";
  value = TypeConverter::bytesToString(Sha256::hash(block_raw));
  if (!addBatch(DBType::BLOCK_RAW, key, value))
    return false;

  return true;
}

bool Storage::putLatestBlockHeader(json &block_header_json) {

  auto latest_block_info = getNthBlockLinkInfo();

  if (latest_block_info.height >=
      Safe::getInt(block_header_json, "hgt")) // do not overwrite lower block
    return true;

  std::string key, value;

  key = "bID";
  value = Safe::getString(block_header_json, "bID");
  if (!addBatch(DBType::BLOCK_LATEST, key, value))
    return false;

  key = "hgt";
  value = Safe::getString(block_header_json, "hgt");
  if (!addBatch(DBType::BLOCK_LATEST, key, value))
    return false;

  return true;
}

bool Storage::putTransaction(json &block_body_json,
                             const string &block_id_b64) {
  if (!block_body_json["tx"].is_array())
    return false;

  std::string key, value;

  int i = 0;
  for (auto &tx_json : block_body_json["tx"]) {

    std::string txid_b64 = Safe::getString(tx_json, "txid");

    key = txid_b64 + "_c";
    value = TypeConverter::bytesToString(json::to_cbor(tx_json));
    if (!addBatch(DBType::TRANSACTION, key, value))
      return false;

    key = txid_b64 + "_mPos";
    value = to_string(i);
    if (!addBatch(DBType::TRANSACTION, key, value))
      return false;

    key = txid_b64 + "_bID";
    value = block_id_b64;
    if (!addBatch(DBType::TRANSACTION, key, value))
      return false;

    ++i;
  }

  key = block_id_b64 + "_mtree";
  value = block_body_json["mtree"].dump();
  if (!addBatch(DBType::TRANSACTION, key, value))
    return false;

  key = block_id_b64 + "_txCnt";
  value = Safe::getString(block_body_json, "txCnt");
  if (!addBatch(DBType::TRANSACTION, key, value))
    return false;

  return true; // everthing is ok!
}

std::string Storage::getValueByKey(DBType what,
                                   const string &base_suffix_keys) {
  std::string key = getPrefix(what) + base_suffix_keys;
  std::string value;

  leveldb::Status status;

  switch (what) {
  case DBType::BLOCK_HEADER:
    status = m_db_block_header->Get(m_read_options, key, &value);
    break;
  case DBType::BLOCK_HEIGHT:
    status = m_db_blockid_height->Get(m_read_options, key, &value);
    break;
  case DBType::BLOCK_RAW:
    status = m_db_block_raw->Get(m_read_options, key, &value);
    break;
  case DBType::BLOCK_LATEST:
    status = m_db_latest_block_header->Get(m_read_options, key, &value);
    break;
  case DBType::TRANSACTION:
    status = m_db_transaction->Get(m_read_options, key, &value);
    break;
  case DBType::LEDGER:
    status = m_db_ledger->Get(m_read_options, key, &value);
    break;
  case DBType::BLOCK_BACKUP:
    status = m_db_backup->Get(m_read_options, key, &value);
    break;
  default:
    break;
  }

  if (!status.ok())
    value = "";

  return value;
}

bool Storage::errorOnCritical(const leveldb::Status &status) {
  if (status.ok())
    return true;

  CLOG(FATAL, "STRG") << "FATAL ERROR on LevelDB " << status.ToString();

  // throw; // it terminates this program
  return false;
}

bool Storage::errorOn(const leveldb::Status &status) {
  if (status.ok())
    return true;

  CLOG(ERROR, "STRG") << "ERROR on LevelDB " << status.ToString();

  return false;
}

nth_link_type Storage::getLatestHashAndHeight() {
  nth_link_type ret_link_info;

  auto block_id = getValueByKey(DBType::BLOCK_LATEST, "bID");
  if (block_id.empty()) {
    ret_link_info.hash =
        TypeConverter::decodeBase64(config::GENESIS_BLOCK_PREV_HASH_B64);
    ret_link_info.height = 0;
  } else {
    ret_link_info.hash = TypeConverter::stringToBytes(
        getValueByKey(DBType::BLOCK_RAW, block_id + "_hash"));
    ret_link_info.height =
        Safe::getSize(getValueByKey(DBType::BLOCK_LATEST, "hgt"));
  }

  return ret_link_info;
}

nth_link_type Storage::getNthBlockLinkInfo(block_height_type t_height) {
  nth_link_type ret_link_info;
  ret_link_info.height = t_height;

  std::string t_block_id_b64 =
      (t_height == 0)
          ? getValueByKey(DBType::BLOCK_LATEST, "bID")
          : getValueByKey(DBType::BLOCK_HEIGHT, to_string(t_height));

  if (!t_block_id_b64.empty()) {
    ret_link_info.id = TypeConverter::decodeBase64(t_block_id_b64);
    ret_link_info.hash = TypeConverter::stringToBytes(
        getValueByKey(DBType::BLOCK_RAW, t_block_id_b64 + "_hash"));
    ret_link_info.prev_id = TypeConverter::decodeBase64(
        getValueByKey(DBType::BLOCK_HEADER, t_block_id_b64 + "_prevbID"));
    ret_link_info.prev_hash = TypeConverter::decodeBase64(
        getValueByKey(DBType::BLOCK_HEADER, t_block_id_b64 + "_prevH"));
    ret_link_info.height = Safe::getSize(
        getValueByKey(DBType::BLOCK_HEADER, t_block_id_b64 + "_hgt"));
    ret_link_info.time = Safe::getTime(
        getValueByKey(DBType::BLOCK_HEADER, t_block_id_b64 + "_time"));
  } else {
    ret_link_info.height = 0;
    ret_link_info.hash =
        TypeConverter::decodeBase64(config::GENESIS_BLOCK_PREV_HASH_B64);
    ret_link_info.id =
        TypeConverter::decodeBase64(config::GENESIS_BLOCK_PREV_ID_B64);
    ret_link_info.time = 0;
  }

  return ret_link_info;
}

std::vector<std::string> Storage::getNthTxIdList(block_height_type t_height) {
  std::vector<std::string> tx_ids_list;

  std::string t_block_id_b64 = getNthBlockIdB64(t_height);

  if (!t_block_id_b64.empty()) {
    auto tx_ids_json_str =
        getValueByKey(DBType::BLOCK_HEADER, t_block_id_b64 + "_txids");

    if (!tx_ids_json_str.empty()) {
      json tx_ids_json = Safe::parseJsonAsArray(tx_ids_json_str);
      if (!tx_ids_json.empty() && tx_ids_json.is_array()) {
        for (auto &tx_id_json : tx_ids_json) {
          tx_ids_list.emplace_back(Safe::getString(tx_id_json));
        }
      }
    }
  }
  return tx_ids_list;
}

void Storage::destroyDB() {
  // clang-format off
  boost::filesystem::remove_all(m_db_path + "/" + config::DB_SUB_DIR_HEADER);
  boost::filesystem::remove_all(m_db_path + "/" + config::DB_SUB_DIR_RAW);
  boost::filesystem::remove_all(m_db_path + "/" + config::DB_SUB_DIR_LATEST);
  boost::filesystem::remove_all(m_db_path + "/" + config::DB_SUB_DIR_TRANSACTION);
  boost::filesystem::remove_all(m_db_path + "/" + config::DB_SUB_DIR_IDHEIGHT);
  boost::filesystem::remove_all(m_db_path + "/" + config::DB_SUB_DIR_LEDGER);
  boost::filesystem::remove_all(m_db_path + "/" + config::DB_SUB_DIR_BACKUP);
  // clang-format on
}

std::string Storage::getNthBlockIdB64(block_height_type height) {
  std::string block_id_b64 =
      (height == 0) ? getValueByKey(DBType::BLOCK_LATEST, "bID")
                    : getValueByKey(DBType::BLOCK_HEIGHT, to_string(height));

  return block_id_b64;
}

storage_block_type Storage::readBlock(block_height_type height) {
  storage_block_type result;
  std::string block_id_b64 = getNthBlockIdB64(height);
  if (block_id_b64.empty())
    result.height = 0;
  else {
    if (height == 0) {
      std::string height_str = getValueByKey(DBType::BLOCK_LATEST, "hgt");
      if (!height_str.empty())
        result.height = static_cast<block_height_type>(stoll(height_str));
    }

    result.block_raw = TypeConverter::stringToBytes(
        getValueByKey(DBType::BLOCK_RAW, block_id_b64));

    json txs_json = json::array();
    std::string txids_json_str =
        getValueByKey(DBType::BLOCK_HEADER, block_id_b64 + "_txids");
    if (!txids_json_str.empty()) {
      try {
        json txids_json = Safe::parseJsonAsArray(txids_json_str);
        for (auto &each_txid : txids_json) {
          std::string tx_cbor_str = getValueByKey(
              DBType::TRANSACTION, Safe::getString(each_txid) + "_c");
          txs_json.push_back(json::from_cbor(tx_cbor_str));
        }
      } catch (json::exception &e) {
        CLOG(FATAL, "STRG") << "FATAL ERROR on LevelDB " << e.what();
      }
    }

    nth_link_type link_info = getNthBlockLinkInfo(height);
    result.height = height;
    result.prev_id = link_info.prev_id;
    result.prev_hash = link_info.prev_hash;
    result.hash = link_info.hash;
    result.id = link_info.id;
    result.time = link_info.time;
    result.txs = txs_json;
  }

  return result;
}

bool Storage::empty() {
  return getValueByKey(DBType::BLOCK_LATEST, "bID").empty();
}

proof_type Storage::getProof(const std::string &txid_b64) {
  int base_offset =
      stoi(getValueByKey(DBType::TRANSACTION, txid_b64 + "_mPos"));
  std::string block_id_b64 =
      getValueByKey(DBType::TRANSACTION, txid_b64 + "_bID");
  proof_type proof;

  if (!block_id_b64.empty()) {

    proof.block_id_b64 = block_id_b64;

    std::string mtree_json_str =
        getValueByKey(DBType::TRANSACTION, block_id_b64 + "_mtree");

    if (!mtree_json_str.empty()) {
      json mtree_json = Safe::parseJson(mtree_json_str);
      std::vector<hash_t> mtree_digests(mtree_json.size());

      for (size_t i = 0; i < mtree_json.size(); ++i) {
        mtree_digests[i] =
            TypeConverter::decodeBase64(Safe::getString(mtree_json[i]));
      }

      MerkleTree mtree_generator(mtree_digests);

      std::vector<hash_t> mtree = mtree_generator.getMerkleTree();

      int merkle_tree_height =
          (int)log2(config::MAX_MERKLE_LEAVES); // log2(4096)=12
      int node_idx = base_offset;
      int merkle_tree_size = config::MAX_MERKLE_LEAVES;

      std::string my_digest_b64 = Safe::getString(mtree_json[node_idx]);

      if (my_digest_b64.empty()) {
        proof.siblings.clear();
        return proof;
      }

      proof.siblings.emplace_back(
          std::make_pair((node_idx % 2 != 0), my_digest_b64));

      for (size_t i = 0; i < merkle_tree_height; ++i) {
        node_idx = (node_idx % 2 != 0) ? node_idx - 1 : node_idx + 1;
        if (node_idx >= mtree.size()) {
          proof.siblings.clear();
          break;
        }

        std::string sibling = TypeConverter::encodeBase64(mtree[node_idx]);

        if (sibling.empty()) {
          proof.siblings.clear();
          break;
        }

        proof.siblings.emplace_back(
            std::make_pair((node_idx % 2 != 0), sibling));
        base_offset /= 2;

        if (i != 0)
          merkle_tree_size += (int)(config::MAX_MERKLE_LEAVES / pow(2, i));

        node_idx = merkle_tree_size + base_offset;
      }
    }
  }

  return proof;
}

bool Storage::isDuplicatedTx(const std::string &txid_b64) {
  std::string block_id_b64 =
      getValueByKey(DBType::TRANSACTION, txid_b64 + "_bID");
  return !block_id_b64.empty();
}

bool Storage::saveLedger(std::string &key, std::string &ledger) {
  addBatch(DBType::LEDGER, key, ledger);
  return true;
}

std::string Storage::readLedgerByKey(std::string &key) {
  return getValueByKey(DBType::LEDGER, key);
}

void Storage::clearLedger() { m_batch_ledger.Clear(); }

void Storage::flushLedger() {
  m_db_ledger->Write(m_write_options, &m_batch_ledger);
  clearLedger();
}

void Storage::saveBackup(const std::string &key, const std::string &value) {
  addBatch(DBType::BLOCK_BACKUP, key, value);
}

std::string Storage::readBackup(const std::string &key) {
  return getValueByKey(DBType::BLOCK_BACKUP, key);
}

void Storage::flushBackup() {
  m_db_backup->Write(m_write_options, &m_batch_backup);
  clearBackup();
}

void Storage::clearBackup() { m_batch_backup.Clear(); }

void Storage::delBackup(const std::string &block_id_b64) {
  if (!block_id_b64.empty())
    m_batch_backup.Delete(block_id_b64);
}

} // namespace gruut
