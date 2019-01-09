#include "storage.hpp"
#include "easy_logging.hpp"

namespace gruut {

using namespace std;

Storage::Storage() {
  el::Loggers::getLogger("STRG");

  auto setting = Setting::getInstance();
  m_db_path = setting->getMyDbPath();

  m_options.create_if_missing = true;
  m_write_options.sync = true;

  boost::filesystem::create_directories(m_db_path);

  errorOnCritical(leveldb::DB::Open(m_options, m_db_path + "/block_header",
                                    &m_db_block_header));
  errorOnCritical(
      leveldb::DB::Open(m_options, m_db_path + "/block_raw", &m_db_block_raw));
  errorOnCritical(leveldb::DB::Open(m_options,
                                    m_db_path + "/latest_block_header",
                                    &m_db_latest_block_header));
  errorOnCritical(leveldb::DB::Open(m_options, m_db_path + "/block_body",
                                    &m_db_block_body));
  errorOnCritical(leveldb::DB::Open(m_options, m_db_path + "/certificate",
                                    &m_db_certificate));
  errorOnCritical(leveldb::DB::Open(m_options, m_db_path + "/blockid_height",
                                    &m_db_blockid_height));
}

Storage::~Storage() {
  delete m_db_block_header;
  delete m_db_block_raw;
  delete m_db_latest_block_header;
  delete m_db_block_body;
  delete m_db_certificate;
  delete m_db_blockid_height;

  m_db_block_header = nullptr;
  m_db_block_raw = nullptr;
  m_db_latest_block_header = nullptr;
  m_db_block_body = nullptr;
  m_db_certificate = nullptr;
  m_db_blockid_height = nullptr;
}

bool Storage::saveBlock(bytes &block_raw, json &block_header,
                        json &block_body) {
  string block_id_b64 = Safe::getString(block_header["bID"]);

  if (putBlockHeader(block_header, block_id_b64) &&
      putBlockHeight(block_header, block_id_b64) &&
      putLatestBlockHeader(block_header) &&
      putBlockBody(block_body, block_id_b64) &&
      putBlockRaw(block_raw, block_id_b64)) {
    commitBatchAll();

    CLOG(INFO, "STRG") << "Success to save block";

    return true;
  }

  CLOG(ERROR, "STRG") << "Failed to save block";

  rollbackBatchAll();
  return false;
}

bool Storage::saveBlock(const string &block_raw_b64, json &block_header,
                        json &block_body) {
  bytes block_raw = TypeConverter::decodeBase64(block_raw_b64);
  return saveBlock(block_raw, block_header, block_body);
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
  case DBType::BLOCK_BODY:
    m_batch_block_body.Put(key, value);
    break;
  case DBType::BLOCK_CERT:
    m_batch_certificate.Put(key, value);
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
  m_db_block_body->Write(m_write_options, &m_batch_block_body);
  m_db_certificate->Write(m_write_options, &m_batch_certificate);

  clearBatchAll();
}

void Storage::rollbackBatchAll() { clearBatchAll(); }

void Storage::clearBatchAll() {
  m_batch_block_header.Clear();
  m_batch_block_raw.Clear();
  m_batch_latest_block_header.Clear();
  m_batch_block_body.Clear();
  m_batch_certificate.Clear();
  m_batch_blockid_height.Clear();
}

bool Storage::putBlockHeader(json &block_header_json,
                             const string &block_id_b64) {
  std::string key, value;
  for (auto &item : DB_BLOCK_HEADER_SUFFIX) {
    if (item.first == "SSig" && block_header_json["SSig"].is_array()) {
      for (size_t i = 0; i < block_header_json["SSig"].size(); ++i) {
        key = block_id_b64 + item.second + "_sID_" + to_string(i);
        value = Safe::getString(block_header_json[item.first][i]["sID"]);
        if (!addBatch(DBType::BLOCK_HEADER, key, value))
          return false;

        key = block_id_b64 + item.second + "_sig_" + to_string(i);
        value = Safe::getString(block_header_json["SSig"][i]["sig"]);
        if (!addBatch(DBType::BLOCK_HEADER, key, value))
          return false;
      }
    } else {
      key = block_id_b64 + item.second;
      if (item.first == "txids") {
        value = block_header_json[item.first].dump();
      } else
        value = Safe::getString(block_header_json[item.first]);
      if (!addBatch(DBType::BLOCK_HEADER, key, value))
        return false;
    }
  }
  return true;
}

bool Storage::putBlockHeight(json &block_header_json,
                             const string &block_id_b64) {
  std::string key = Safe::getString(block_header_json["hgt"]);
  return addBatch(DBType::BLOCK_HEIGHT, key, block_id_b64);
}

bool Storage::putBlockRaw(bytes &block_raw, const string &block_id_b64) {

  std::string block_raw_key = block_id_b64;
  std::string block_raw_str(block_raw.begin(), block_raw.end());
  if (!addBatch(DBType::BLOCK_RAW, block_raw_key, block_raw_str))
    return false;

  std::string block_hash =
      TypeConverter::bytesToString(Sha256::hash(block_raw));

  std::string hash_key = block_raw_key + "_hash";
  if (!addBatch(DBType::BLOCK_RAW, hash_key, block_hash))
    return false;

  return true;
}

bool Storage::putLatestBlockHeader(json &block_header_json) {

  auto latest_block_info = getNthBlockLinkInfo();

  if (latest_block_info.height >=
      Safe::getInt(block_header_json, "hgt")) // do not overwrite lower block
    return true;

  std::string key, value;
  for (auto &item : DB_BLOCK_HEADER_SUFFIX) {
    if (item.first == "SSig" && block_header_json["SSig"].is_array()) {
      for (size_t i = 0; i < block_header_json["SSig"].size(); ++i) {
        key = item.first + "_sID_" + to_string(i);
        value = Safe::getString(block_header_json[item.first][i]["sID"]);
        if (!addBatch(DBType::BLOCK_LATEST, key, value))
          return false;

        key = item.first + "_sig_" + to_string(i);
        value = Safe::getString(block_header_json["SSig"][i]["sig"]);
        if (!addBatch(DBType::BLOCK_LATEST, key, value))
          return false;
      }
    } else {
      key = item.first;
      if (item.first == "txids")
        value = block_header_json[item.first].dump();
      else
        value = Safe::getString(block_header_json[item.first]);
      if (!addBatch(DBType::BLOCK_LATEST, key, value))
        return false;
    }
  }
  return true;
}

bool Storage::putBlockBody(json &block_body_json, const string &block_id_b64) {
  if (!block_body_json["tx"].is_array())
    return false;

  std::string key, value;

  int i = 0;
  for (auto &tx_json : block_body_json["tx"]) {
    json content = tx_json["content"];
    if (!content.is_array())
      return false;

    tx_json["bID"] = block_id_b64;
    tx_json["mPos"] = to_string(i);

    for (auto &item : DB_TX_SUFFIX) {
      // To save user certificates
      if (item.first == "type" &&
          Safe::getString(tx_json[item.first]) == TXTYPE_CERTIFICATES) {
        for (size_t c_idx = 0; c_idx < content.size(); c_idx += 2) {
          string user_id_b64 = Safe::getString(content[c_idx]);
          string cert_idx = getValueByKey(DBType::BLOCK_CERT, user_id_b64);

          // User ID에 해당하는 Certification Size 저장
          key = user_id_b64;
          value = (cert_idx.empty()) ? "1" : to_string(stoi(cert_idx) + 1);
          if (!addBatch(DBType::BLOCK_CERT, key, value))
            return false;

          key += (cert_idx.empty()) ? "_0" : "_" + cert_idx;
          std::string pem = Safe::getString(content[c_idx + 1]);
          value = parseCert(pem);
          if (value.empty())
            return false;

          if (!addBatch(DBType::BLOCK_CERT, key, value))
            return false;
        }
      }

      std::string txid_b64 = Safe::getString(tx_json["txid"]);
      key = txid_b64 + item.second;
      if (item.first == "content") {
        value = tx_json[item.first].dump();
      } else
        value = Safe::getString(tx_json[item.first]);
      if (!addBatch(DBType::BLOCK_BODY, key, value))
        return false;
    }

    ++i;
  }

  key = block_id_b64 + "_mtree";
  value = block_body_json["mtree"].dump();
  if (!addBatch(DBType::BLOCK_BODY, key, value))
    return false;

  key = block_id_b64 + "_txCnt";
  value = Safe::getString(block_body_json["txCnt"]);
  if (!addBatch(DBType::BLOCK_BODY, key, value))
    return false;

  return true; // everthing is ok!
}

std::string Storage::parseCert(std::string &pem) {

  // User ID에 해당하는 n번째 Certification 저장 (발급일, 만료일, 인증서)

  std::string json_str;

  try {
    Botan::DataSource_Memory cert_datasource(pem);
    Botan::X509_Certificate cert(cert_datasource);

    json tmp_cert;
    tmp_cert[0] =
        to_string(Botan::X509_Time(cert.not_before()).time_since_epoch());
    tmp_cert[1] =
        to_string(Botan::X509_Time(cert.not_after()).time_since_epoch());
    tmp_cert[2] = pem;
    json_str = tmp_cert.dump();
  } catch (...) {
    // do nothing
  }

  return json_str;
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
  case DBType::BLOCK_BODY:
    status = m_db_block_body->Get(m_read_options, key, &value);
    break;
  case DBType::BLOCK_CERT:
    status = m_db_certificate->Get(m_read_options, key, &value);
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

std::pair<std::string, size_t> Storage::getLatestHashAndHeight() {
  auto block_id = getValueByKey(DBType::BLOCK_LATEST, "bID");
  if (block_id.empty())
    return make_pair("", 0);

  auto hash_b64 = TypeConverter::encodeBase64(
      getValueByKey(DBType::BLOCK_RAW, block_id + "_hash"));
  auto height = getValueByKey(DBType::BLOCK_LATEST, "hgt");
  if (height.empty())
    return std::make_pair(hash_b64, 0);
  else
    return std::make_pair(hash_b64, static_cast<size_t>(stoll(height)));
}

nth_block_link_type Storage::getNthBlockLinkInfo(size_t t_height) {
  nth_block_link_type ret_link_info;
  ret_link_info.height = t_height;

  std::string t_block_id_b64 =
      (t_height == 0)
          ? getValueByKey(DBType::BLOCK_LATEST, "bID")
          : getValueByKey(DBType::BLOCK_HEIGHT, to_string(t_height));

  if (!t_block_id_b64.empty()) {
    ret_link_info.id_b64 = t_block_id_b64;
    ret_link_info.hash_b64 = TypeConverter::encodeBase64(
        getValueByKey(DBType::BLOCK_RAW, t_block_id_b64 + "_hash"));
    ret_link_info.prev_id_b64 =
        getValueByKey(DBType::BLOCK_HEADER, t_block_id_b64 + "_prevbID");
    ret_link_info.prev_hash_b64 =
        getValueByKey(DBType::BLOCK_HEADER, t_block_id_b64 + "_prevH");
    ret_link_info.height = Safe::getSize(
        getValueByKey(DBType::BLOCK_HEADER, t_block_id_b64 + "_hgt"));
  } else {
    ret_link_info.height = 0;
  }

  return ret_link_info;
}

std::vector<std::string> Storage::getNthTxIdList(size_t t_height) {
  std::vector<std::string> tx_ids_list;

  std::string t_block_id_b64 =
      (t_height == 0)
          ? getValueByKey(DBType::BLOCK_LATEST, "bID")
          : getValueByKey(DBType::BLOCK_HEIGHT, to_string(t_height));

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

std::string Storage::getCertificate(const std::string &user_id_b64,
                                    const timestamp_type &at_this_time) {
  std::string cert;
  std::string cert_size = getValueByKey(DBType::BLOCK_CERT, user_id_b64);
  if (!cert_size.empty()) {
    int num_certs = stoi(cert_size);
    if (at_this_time == 0) {
      std::string latest_cert = getValueByKey(
          DBType::BLOCK_CERT, user_id_b64 + "_" + to_string(num_certs - 1));

      json latest_cert_json = Safe::parseJson(latest_cert);
      if (!latest_cert_json.empty())
        cert = Safe::getString(latest_cert_json[2]);

    } else {
      timestamp_type max_start_date = 0;
      for (int i = 0; i < num_certs; ++i) {
        std::string nth_cert =
            getValueByKey(DBType::BLOCK_CERT, user_id_b64 + "_" + to_string(i));

        json cert_json = Safe::parseJson(nth_cert);
        if (cert_json.empty())
          break;

        auto start_date =
            static_cast<timestamp_type>(stoi(Safe::getString(cert_json[0])));
        auto end_date =
            static_cast<timestamp_type>(stoi(Safe::getString(cert_json[1])));
        if (start_date <= at_this_time && at_this_time <= end_date) {
          if (max_start_date < start_date) {
            max_start_date = start_date;
            cert = Safe::getString(cert_json[2]);
          }
        }
      }
    }
  }

  return cert;
}

void Storage::destroyDB() {
  boost::filesystem::remove_all(m_db_path + "/block_header");
  boost::filesystem::remove_all(m_db_path + "/block_raw");
  boost::filesystem::remove_all(m_db_path + "/certificate");
  boost::filesystem::remove_all(m_db_path + "/latest_block_header");
  boost::filesystem::remove_all(m_db_path + "/block_body");
  boost::filesystem::remove_all(m_db_path + "/blockid_height");
}

read_block_type Storage::readBlock(size_t height) {
  read_block_type result;
  std::string block_id_b64 =
      (height == 0) ? getValueByKey(DBType::BLOCK_LATEST, "bID")
                    : getValueByKey(DBType::BLOCK_HEIGHT, to_string(height));
  if (block_id_b64.empty())
    result.height = 0;
  else {
    if (height == 0)
      result.height =
          static_cast<size_t>(stoll(getValueByKey(DBType::BLOCK_LATEST, "hgt")));

    result.block_raw = TypeConverter::stringToBytes(
        getValueByKey(DBType::BLOCK_RAW, block_id_b64));

    std::string txids_json_str =
        getValueByKey(DBType::BLOCK_HEADER, block_id_b64 + "_txids");

    json txs_json = json::array();

    if (!txids_json_str.empty()) {
      json txids_json = Safe::parseJson(txids_json_str);

      if (txids_json.is_array()) {
        for (auto &each_txid : txids_json) {
          std::string txid_b64 = Safe::getString(each_txid);

          json tx_json = json(
              {{"txid", txid_b64},
               {"time", getValueByKey(DBType::BLOCK_BODY, txid_b64 + "_time")},
               {"rID", getValueByKey(DBType::BLOCK_BODY, txid_b64 + "_rID")},
               {"rSig", getValueByKey(DBType::BLOCK_BODY, txid_b64 + "_rSig")},
               {"type",
                getValueByKey(DBType::BLOCK_BODY, txid_b64 + "_type")}});

          tx_json["content"] = Safe::parseJsonAsArray(
              getValueByKey(DBType::BLOCK_BODY, txid_b64 + "_content"));

          txs_json.push_back(tx_json);
        }
      }
    }

    result.txs = txs_json;
  }

  return result;
}

proof_type Storage::getProof(const std::string &txid_b64) {
  int base_offset = stoi(getValueByKey(DBType::BLOCK_BODY, txid_b64 + "_mPos"));
  std::string block_id_b64 =
      getValueByKey(DBType::BLOCK_BODY, txid_b64 + "_bID");
  proof_type proof;

  if (!block_id_b64.empty()) {

    proof.block_id_b64 = block_id_b64;

    std::string mtree_json_str =
        getValueByKey(DBType::BLOCK_BODY, block_id_b64 + "_mtree");

    if (!mtree_json_str.empty()) {
      json mtree_json = Safe::parseJson(mtree_json_str);
      std::vector<sha256> mtree_digests(mtree_json.size());

      for (size_t i = 0; i < mtree_json.size(); ++i) {
        mtree_digests[i] =
            TypeConverter::decodeBase64(Safe::getString(mtree_json[i]));
      }

      MerkleTree mtree_generator(mtree_digests);

      std::vector<sha256> mtree = mtree_generator.getMerkleTree();

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

std::string Storage::getCertificate(const signer_id_type &user_id,
                                    const timestamp_type &at_this_time) {
  std::string user_id_b64 = TypeConverter::encodeBase64(user_id);
  return getCertificate(user_id_b64);
}
} // namespace gruut
