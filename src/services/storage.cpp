#include "storage.hpp"
#include "setting.hpp"
#include <botan-2/botan/asn1_time.h>
namespace gruut {
using namespace std;
using namespace nlohmann;

Storage::Storage() {
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
  string block_id = block_header["bID"];

  if (putBlockHeader(block_header, block_id) &&
      putBlockHeight(block_header, block_id) &&
      putLatestBlockHeader(block_header) &&
      putBlockBody(block_body, block_id) && putBlockRaw(block_raw, block_id)) {
    commitBatchAll();
    return true;
  }

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

bool Storage::putBatch(DBType what, const string &key, const string &value) {
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

bool Storage::putBlockHeader(json &data, const string &block_id) {
  std::string key, value;
  std::string prefix = getPrefix(DBType::BLOCK_HEADER);
  for (auto &item : DB_BLOCK_HEADER_SUFFIX) {
    if (item.first == "SSig" && data["SSig"].is_array()) {
      for (size_t i = 0; i < data["SSig"].size(); ++i) {
        key = prefix + block_id + item.second + "_sID_" + to_string(i);
        value = data[item.first][i]["sID"].get<string>();
        if (!putBatch(DBType::BLOCK_HEADER, key, value))
          return false;

        key = prefix + block_id + item.second + "_sig_" + to_string(i);
        value = data["SSig"][i]["sig"].get<string>();
        if (!putBatch(DBType::BLOCK_HEADER, key, value))
          return false;
      }
    } else {
      key = prefix + block_id + item.second;
      if (item.first == "txids") {
        value = data[item.first].dump();
      } else
        value = data[item.first].get<string>();
      if (!putBatch(DBType::BLOCK_HEADER, key, value))
        return false;
    }
  }
  return true;
}

bool Storage::putBlockHeight(json &data, const string &block_id) {
  string prefix = getPrefix(DBType::BLOCK_HEIGHT);
  std::string key = prefix + data["hgt"].get<string>();
  return putBatch(DBType::BLOCK_HEIGHT, key, block_id);
}

bool Storage::putBlockRaw(bytes &block_raw, const string &block_id) {

  std::string prefix = getPrefix(DBType::BLOCK_RAW);

  std::string block_raw_key = prefix + block_id;
  std::string block_raw_str(block_raw.begin(), block_raw.end());
  if (!putBatch(DBType::BLOCK_RAW, block_raw_key, block_raw_str))
    return false;

  sha256 block_hash = Sha256::hash(block_raw);

  std::string hash_key = block_raw_key + "_hash";
  std::string hash_value(block_hash.begin(), block_hash.end());
  if (!putBatch(DBType::BLOCK_RAW, hash_key, hash_value))
    return false;

  return true;
}

bool Storage::putLatestBlockHeader(json &data) {
  std::string key, value;
  std::string prefix = getPrefix(DBType::BLOCK_LATEST);
  // TODO : 복잡한 if-else 패턴을 단순화 시켜야 함
  for (auto &item : DB_BLOCK_HEADER_SUFFIX) {
    if (item.first == "SSig" && data["SSig"].is_array()) {
      for (size_t i = 0; i < data["SSig"].size(); ++i) {
        key = prefix + item.first + "_sID_" + to_string(i);
        value = data[item.first][i]["sID"].get<string>();
        if (!putBatch(DBType::BLOCK_LATEST, key, value))
          return false;

        key = prefix + item.first + "_sig_" + to_string(i);
        value = data["SSig"][i]["sig"].get<string>();
        if (!putBatch(DBType::BLOCK_LATEST, key, value))
          return false;
      }
    } else {
      key = prefix + item.first;
      if (item.first == "txids")
        value = data[item.first].dump();
      else
        value = data[item.first].get<string>();
      if (!putBatch(DBType::BLOCK_LATEST, key, value))
        return false;
    }
  }
  return true;
}

bool Storage::putBlockBody(json &data, const string &block_id) {
  if (!data["tx"].is_array())
    return false;

  std::string key, value;
  std::string prefix = getPrefix(DBType::BLOCK_BODY);
  leveldb::WriteBatch batch;

  std::string cert_prefix = getPrefix(DBType::BLOCK_CERT);

  for (size_t i = 0; i < data["tx"].size(); ++i) {
    json content = data["tx"][i]["content"];
    if (!content.is_array())
      return false;

    data["tx"][i]["bID"] = block_id;
    data["tx"][i]["mPos"] = to_string(i);

    for (auto &item : DB_BLOCK_TX_SUFFIX) {
      // To save user certificates
      if (item.first == "type" &&
          data["tx"][i][item.first].get<string>() == TXTYPE_CERTIFICATES) {
        for (size_t c_idx = 0; c_idx < content.size(); c_idx += 2) {
          string user_id = content[c_idx].get<string>();
          string cert_idx = getDataByKey(DBType::BLOCK_CERT, user_id);

          // User ID에 해당하는 Certification Size 저장
          key = cert_prefix + user_id;
          value = (cert_idx.empty()) ? "1" : to_string(stoi(cert_idx) + 1);
          if (!putBatch(DBType::BLOCK_CERT, key, value))
            return false;

          key += (cert_idx.empty()) ? "_0" : "_" + cert_idx;
          std::string pem = content[c_idx + 1].get<string>();
          value = parseCert(pem);
          if (value.empty())
            return false;

          if (!putBatch(DBType::BLOCK_CERT, key, value))
            return false;
        }
      }

      std::string tx_id = data["tx"][i]["txID"];
      key = prefix + tx_id + item.second;
      if (item.first == "content") {
        value = data["tx"][i][item.first].dump();
      } else
        value = data["tx"][i][item.first].get<string>();
      if (!putBatch(DBType::BLOCK_BODY, key, value))
        return false;
    }
  }

  key = prefix + block_id + "_mtree";
  value = data["mtree"].dump();
  if (!putBatch(DBType::BLOCK_BODY, key, value))
    return false;

  key = prefix + block_id + "_txCnt";
  value = data["txCnt"].get<string>();
  if (!putBatch(DBType::BLOCK_BODY, key, value))
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

std::string Storage::getDataByKey(DBType what, const string &keys) {
  std::string key = getPrefix(what) + keys;
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

  cout << "STG: FATAL ERROR on LevelDB " << status.ToString() << endl;

  // throw; // it terminates this program
  return false;
}

bool Storage::errorOn(const leveldb::Status &status) {
  if (status.ok())
    return true;

  cout << "STG: ERROR on LevelDB " << status.ToString() << endl;

  return false;
}

std::pair<std::string, size_t> Storage::findLatestHashAndHeight() {
  auto block_id = getDataByKey(DBType::BLOCK_LATEST, "bID");
  if (block_id.empty())
    return make_pair("", 0);

  auto hash = getDataByKey(DBType::BLOCK_RAW, block_id + "_hash");
  auto height = getDataByKey(DBType::BLOCK_LATEST, "hgt");
  if (height.empty())
    return std::make_pair(hash, 0);
  else
    return std::make_pair(hash, static_cast<size_t>(stoll(height)));
}

std::tuple<std::string, std::string, size_t>
Storage::findLatestBlockBasicInfo() {
  std::tuple<std::string, std::string, size_t> ret_tuple;

  auto block_id = getDataByKey(DBType::BLOCK_LATEST, "bID");
  if (!block_id.empty()) {
    std::get<0>(ret_tuple) = block_id;
    std::get<1>(ret_tuple) =
        getDataByKey(DBType::BLOCK_RAW, block_id + "_hash");
    auto height = getDataByKey(DBType::BLOCK_LATEST, "hgt");
    if (height.empty())
      std::get<2>(ret_tuple) = 0;
    else
      std::get<2>(ret_tuple) = static_cast<size_t>(stoll(height));
  }
  return ret_tuple;
}

std::vector<std::string> Storage::findLatestTxIdList() {
  std::vector<std::string> tx_ids_list;

  auto block_id = getDataByKey(DBType::BLOCK_LATEST, "bID");
  if (!block_id.empty()) {
    auto tx_ids = getDataByKey(DBType::BLOCK_HEADER, block_id + "_txids");
    if (!tx_ids.empty()) {
      json tx_ids_json = json::parse(tx_ids);
      for (auto &tx_id_json : tx_ids_json) {
        tx_ids_list.emplace_back(tx_id_json.get<string>());
      }
    }
  }
  return tx_ids_list;
}

std::string Storage::findCertificate(const std::string &user_id,
                                     const timestamp_type &at_this_time) {
  std::string cert;
  std::string cert_size = getDataByKey(DBType::BLOCK_CERT, user_id);
  if (!cert_size.empty()) {
    if (at_this_time == 0) {
      std::string latest_cert = getDataByKey(
          DBType::BLOCK_CERT, user_id + "_" + to_string(stoi(cert_size) - 1));
      if (latest_cert.empty())
        cert.assign("");
      else
        cert = json::parse(latest_cert)[2].get<string>();
    } else {
      timestamp_type max_start_date = 0;
      for (int i = 0; i < stoi(cert_size); ++i) {
        std::string nth_cert =
            getDataByKey(DBType::BLOCK_CERT, user_id + "_" + to_string(i));
        if (nth_cert.empty()) {
          cert.assign("");
          break;
        } else {
          json cert_json = json::parse(nth_cert);
          auto start_date = (timestamp_type)stoi(cert_json[0].get<string>());
          auto end_date = (timestamp_type)stoi(cert_json[1].get<string>());
          if (start_date <= at_this_time && at_this_time <= end_date) {
            if (max_start_date < start_date) {
              max_start_date = start_date;
              cert = cert_json[2].get<string>();
            }
          }
        }
      }
    }
  }

  return cert;
}

void Storage::deleteAllDirectory() {
  boost::filesystem::remove_all(m_db_path + "/block_header");
  boost::filesystem::remove_all(m_db_path + "/block_raw");
  boost::filesystem::remove_all(m_db_path + "/certificate");
  boost::filesystem::remove_all(m_db_path + "/latest_block_header");
  boost::filesystem::remove_all(m_db_path + "/block_body");
  boost::filesystem::remove_all(m_db_path + "/blockid_height");
}

std::tuple<int, std::string, json> Storage::readBlock(int height) {
  std::tuple<int, std::string, json> result;
  std::string block_id =
      (height == -1) ? getDataByKey(DBType::BLOCK_LATEST, "bID")
                     : getDataByKey(DBType::BLOCK_HEIGHT, to_string(height));
  if (block_id.empty())
    result = std::make_tuple(-1, "", json({}));
  else {
    if (height == -1)
      height = stoi(getDataByKey(DBType::BLOCK_LATEST, "hgt"));
    std::string block_raw = getDataByKey(DBType::BLOCK_RAW, block_id);

    json txs_json;
    std::string tx_ids_json_str =
        getDataByKey(DBType::BLOCK_HEADER, block_id + "_txids");

    if (!tx_ids_json_str.empty()) {
      json tx_ids_json = json::parse(tx_ids_json_str);

      for (size_t i = 0; i < tx_ids_json.size(); ++i) {
        std::string tx_id = tx_ids_json[i].get<std::string>();

        txs_json[i]["txid"] = tx_id;
        txs_json[i]["time"] = getDataByKey(DBType::BLOCK_BODY, tx_id + "_time");
        txs_json[i]["rID"] = getDataByKey(DBType::BLOCK_BODY, tx_id + "_rID");
        txs_json[i]["rSig"] = getDataByKey(DBType::BLOCK_BODY, tx_id + "_rSig");
        txs_json[i]["type"] = getDataByKey(DBType::BLOCK_BODY, tx_id + "_type");

        json content =
            json::parse(getDataByKey(DBType::BLOCK_BODY, tx_id + "_content"));
        if (txs_json[i]["type"] == TXTYPE_CERTIFICATES ||
            txs_json[i]["type"] == TXTYPE_DIGESTS) {
          for (size_t j = 0; j < content.size(); ++j)
            txs_json[i]["content"][j] = content[j].get<string>();
        }
      }
    }

    result = std::make_tuple(height, block_raw, txs_json);
  }
  return result;
}

std::vector<std::string> Storage::findSibling(const std::string &tx_id) {
  int base_offset = stoi(getDataByKey(DBType::BLOCK_BODY, tx_id + "_mPos"));
  std::string block_id = getDataByKey(DBType::BLOCK_BODY, tx_id + "_bID");
  std::vector<std::string> siblings;

  if (!block_id.empty()) {

    std::string mtree_json_str =
        getDataByKey(DBType::BLOCK_BODY, block_id + "_mtree");

    if (!mtree_json_str.empty()) {
      json mtree_json = json::parse(mtree_json_str);
      std::vector<sha256> mtree_digests(mtree_json.size());

      for (size_t i = 0; i < mtree_json.size(); ++i) {
        mtree_digests[i] =
            TypeConverter::decodeBase64(mtree_json[i].get<std::string>());
      }

      MerkleTree mtree_generator(mtree_digests);

      std::vector<sha256> mtree = mtree_generator.getMerkleTree();

      int iter_size = (int)log2(config::MAX_MERKLE_LEAVES); // log2(4096)=12
      int node_idx = base_offset;
      int size = config::MAX_MERKLE_LEAVES;
      for (size_t i = 0; i < iter_size; ++i) {
        node_idx = (node_idx % 2 != 0) ? node_idx - 1 : node_idx + 1;
        if (node_idx >= mtree.size()) {
          siblings.clear();
          break;
        }

        std::string sibling = TypeConverter::toBase64Str(mtree[node_idx]);

        if (sibling.empty()) {
          siblings.clear();
          break;
        }

        siblings.emplace_back(sibling);
        base_offset /= 2;

        if (i != 0)
          size += (int)(config::MAX_MERKLE_LEAVES / pow(2, i));

        node_idx = size + base_offset;
      }
    }
  }

  return siblings;
}

std::string Storage::findCertificate(const signer_id_type &user_id,
                                     const timestamp_type &at_this_time) {
  std::string user_id_b64 = TypeConverter::toBase64Str(user_id);
  return findCertificate(user_id_b64);
}
} // namespace gruut
