#include "storage.hpp"

namespace gruut {
using namespace std;
using namespace nlohmann;
using namespace macaron;

Storage::Storage() {
  m_options.create_if_missing = true;
  m_write_options.sync = true;

  errorOnCritical(
      leveldb::DB::Open(m_options, "./block_header", &m_db_block_header));
  errorOnCritical(leveldb::DB::Open(m_options, "./block_raw", &m_db_block_raw));
  errorOnCritical(leveldb::DB::Open(m_options, "./latest_block_header",
                                    &m_db_latest_block_header));
  errorOnCritical(
      leveldb::DB::Open(m_options, "./block_body", &m_db_block_body));
  errorOnCritical(
      leveldb::DB::Open(m_options, "./certificate", &m_db_certificate));
  errorOnCritical(
      leveldb::DB::Open(m_options, "./blockid_height", &m_db_blockid_height));
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

bool Storage::saveBlock(const string &block_raw, json &block_header,
                        json &block_body) {
  string block_id = block_header["bID"];
  sha256 hash = Sha256::hash(block_raw);
  json block_raw_json = {{"b64", block_raw}, {"hash", Sha256::toString(hash)}};

  if (putBlockHeader(block_header, block_id) &&
      putBlockHeight(block_header, block_id) &&
      putLatestBlockHeader(block_header) &&
      putBlockBody(block_body, block_id) &&
      putBlockRaw(block_raw_json, block_id))
    return true;
  return false;
}

string Storage::getPrefix(DBType what) {
  string prefix;
  for (auto it_map = DB_PREFIX.begin(); it_map != DB_PREFIX.end(); ++it_map) {
    if (it_map->first == what) {
      prefix = it_map->second;
      break;
    }
  }
  return prefix;
}

bool Storage::put(DBType what, const string &key, const string &value) {
  leveldb::Status status;
  switch (what) {
  case DBType::BLOCK_HEADER:
    status = m_db_block_header->Put(m_write_options, key, value);
    break;
  case DBType::BLOCK_HEIGHT:
    status = m_db_blockid_height->Put(m_write_options, key, value);
    break;
  case DBType::BLOCK_RAW:
    status = m_db_block_raw->Put(m_write_options, key, value);
    break;
  case DBType::BLOCK_LATEST:
    status = m_db_latest_block_header->Put(m_write_options, key, value);
    break;
  case DBType::BLOCK_BODY:
    status = m_db_block_body->Put(m_write_options, key, value);
    break;
  case DBType::BLOCK_CERT:
    status = m_db_certificate->Put(m_write_options, key, value);
    break;
  default:
    break;
  }
  return errorOn(status);
}

bool Storage::putBlockHeader(json &data, const string &block_id) {
  string key, value;
  string prefix = getPrefix(DBType::BLOCK_HEADER);
  for (auto &item : DB_BLOCK_HEADER_SUFFIX) {
    if (item.first == "SSig" && data["SSig"].is_array()) {
      for (size_t i = 0; i < data["SSig"].size(); ++i) {
        key = prefix + block_id + item.second + "_sID_" + to_string(i);
        value = data[item.first][i]["sID"].get<string>();
        if (!put(DBType::BLOCK_HEADER, key, value))
          return false;

        key = prefix + block_id + item.second + "_sig_" + to_string(i);
        value = data["SSig"][i]["sig"].get<string>();
        if (!put(DBType::BLOCK_HEADER, key, value))
          return false;
      }
    } else {
      key = prefix + block_id + item.second;
      if (item.first == "txids") {
        value = data[item.first].dump();
      } else
        value = data[item.first].get<string>();
      if (!put(DBType::BLOCK_HEADER, key, value))
        return false;
    }
  }
  return true;
}

bool Storage::putBlockHeight(json &data, const string &block_id) {
  string key, value;
  string prefix = getPrefix(DBType::BLOCK_HEIGHT);
  key = prefix + data["hgt"].get<string>();
  value = block_id;
  return put(DBType::BLOCK_HEIGHT, key, value);
}
bool Storage::putBlockRaw(json &data, const string &block_id) {
  string key, value;
  string prefix = getPrefix(DBType::BLOCK_RAW);
  key = prefix + block_id + "_b64";
  value = data["b64"].get<string>();
  if (!put(DBType::BLOCK_RAW, key, value))
    return false;

  key = prefix + block_id + "_hash";
  value = data["hash"].get<string>();
  if (!put(DBType::BLOCK_RAW, key, value))
    return false;
  return true;
}
bool Storage::putLatestBlockHeader(json &data) {
  string key, value;
  string prefix = getPrefix(DBType::BLOCK_LATEST);
  // TODO : 복잡한 if-else 패턴을 단순화 시켜야 함
  for (auto &item : DB_BLOCK_HEADER_SUFFIX) {
    if (item.first == "SSig" && data["SSig"].is_array()) {
      for (size_t i = 0; i < data["SSig"].size(); ++i) {
        key = prefix + item.first + "_sID_" + to_string(i);
        value = data[item.first][i]["sID"].get<string>();
        if (!put(DBType::BLOCK_LATEST, key, value))
          return false;

        key = prefix + item.first + "_sig_" + to_string(i);
        value = data["SSig"][i]["sig"].get<string>();
        if (!put(DBType::BLOCK_LATEST, key, value))
          return false;
      }
    } else {
      key = prefix + item.first;
      if (item.first == "txids")
        value = data[item.first].dump();
      else
        value = data[item.first].get<string>();
      if (!put(DBType::BLOCK_LATEST, key, value))
        return false;
    }
  }
  return true;
}
bool Storage::putBlockBody(json &data, const string &block_id) {
  if (!data["tx"].is_array())
    return false;

  string key, value;
  string prefix = getPrefix(DBType::BLOCK_BODY);

  for (size_t tx_idx = 0; tx_idx < data["tx"].size(); ++tx_idx) {
    json content = data["tx"][tx_idx]["content"];
    if (!content.is_array())
      return false;
    data["tx"][tx_idx]["bID"] = block_id;
    data["tx"][tx_idx]["mPos"] = to_string(tx_idx);
    string tx_id = data["tx"][tx_idx]["txID"];
    for (auto &item : DB_BLOCK_TX_SUFFIX) {
      // Certificate 저장하는 부분
      if (item.first == "type" &&
          data["tx"][tx_idx][item.first].get<string>() == "certificates") {
        for (auto content_idx = 0; content_idx < content.size();
             content_idx += 2) {
          string user_id = content[content_idx].get<string>();
          string cert_idx = getDataByKey(DBType::BLOCK_CERT, user_id);

          // User ID에 해당하는 Certification Size 저장
          key = "certificate_" + user_id;
          value = (cert_idx.empty()) ? "1" : to_string(stoi(cert_idx) + 1);
          if (!put(DBType::BLOCK_CERT, key, value))
            return false;

          // User ID에 해당하는 n번째 Certification 저장 (발급일, 만료일,
          // 인증서)
          key = (cert_idx.empty()) ? "certificate_" + user_id + "_0"
                                   : "certificate_" + user_id + "_" + cert_idx;
          string pem = content[content_idx + 1].get<string>();
          try {
            Botan::DataSource_Memory cert_datasource(pem);
            Botan::X509_Certificate cert(cert_datasource);

            json tmp_cert;
            tmp_cert[0] = to_string(
                Botan::X509_Time(cert.not_before()).time_since_epoch());
            tmp_cert[1] = to_string(
                Botan::X509_Time(cert.not_after()).time_since_epoch());
            tmp_cert[2] = pem;

            value = tmp_cert.dump();
            if (!put(DBType::BLOCK_CERT, key, value))
              return false;

          } catch (Botan::Exception &exception) {
            return false;
          }
        }
      }

      key = prefix + tx_id + item.second;
      if (item.first == "content") {
        value = data["tx"][tx_idx][item.first].dump();
      } else
        value = data["tx"][tx_idx][item.first].get<string>();
      if (!put(DBType::BLOCK_BODY, key, value))
        return false;
    }
  }

  key = prefix + block_id + "_mtree";
  value = data["mtree"].dump();
  if (!put(DBType::BLOCK_BODY, key, value))
    return false;

  key = prefix + block_id + "_txCnt";
  value = data["txCnt"].get<string>();
  if (!put(DBType::BLOCK_BODY, key, value))
    return false;
  return true;
}

string Storage::getDataByKey(DBType what, const string &keys) {
  string key = getPrefix(what) + keys;
  string value;

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
    value.assign("");

  return value;
}

bool Storage::errorOnCritical(const leveldb::Status &status) {
  if (status.ok())
    return true;

  const std::string errmsg = "Fatal LevelDB error: " + status.ToString();
  cout << errmsg << endl;

  throw;

  return false;
}

bool Storage::errorOn(const leveldb::Status &status) {
  if (status.ok())
    return true;

  const std::string errmsg = "LevelDB error: " + status.ToString();
  cout << errmsg << endl;

  return false;
}

pair<string, string> Storage::findLatestHashAndHeight() {
  auto block_id = getDataByKey(DBType::BLOCK_LATEST, "bID");
  if (block_id.empty())
    return make_pair("", "");

  auto hash = getDataByKey(DBType::BLOCK_RAW, block_id + "_hash");
  auto height = getDataByKey(DBType::BLOCK_LATEST, "hgt");
  return make_pair(hash, height);
}

vector<string> Storage::findLatestTxIdList() {
  vector<string> tx_ids_list;

  auto block_id = getDataByKey(DBType::BLOCK_LATEST, "bID");
  if (!block_id.empty()) {
    auto tx_ids = getDataByKey(DBType::BLOCK_HEADER, block_id + "_txids");
    if (!tx_ids.empty()) {
      json tx_ids_json = json::parse(tx_ids);
      for (size_t tx_ids_idx = 0; tx_ids_idx < tx_ids_json.size();
           ++tx_ids_idx) {
        tx_ids_list.emplace_back(tx_ids_json[tx_ids_idx]);
      }
    }
  }
  return tx_ids_list;
}

string Storage::findCertificate(const string &user_id,
                                const timestamp_type &at_this_time) {
  string cert;
  string cert_size = getDataByKey(DBType::BLOCK_CERT, user_id);
  if (!cert_size.empty()) {
    if (at_this_time == 0) {
      string latest_cert = getDataByKey(
          DBType::BLOCK_CERT, user_id + "_" + to_string(stoi(cert_size) - 1));
      if (latest_cert.empty())
        cert.assign("");
      else
        cert = json::parse(latest_cert)[2].get<string>();
    } else {
      timestamp_type max_start_date = 0;
      for (int i = 0; i < stoi(cert_size); ++i) {
        string nth_cert =
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
  } else
    cert.assign("");
  return cert;
}

void Storage::deleteAllDirectory() {
  boost::filesystem::remove_all("./block_header");
  boost::filesystem::remove_all("./block_raw");
  boost::filesystem::remove_all("./certificate");
  boost::filesystem::remove_all("./latest_block_header");
  boost::filesystem::remove_all("./block_body");
  boost::filesystem::remove_all("./blockid_height");
}

tuple<int, string, json> Storage::readBlock(int height) {
  tuple<int, string, json> result;
  string block_id = (height == -1)
                        ? getDataByKey(DBType::BLOCK_LATEST, "bID")
                        : getDataByKey(DBType::BLOCK_HEIGHT, to_string(height));
  if (block_id.empty())
    result = make_tuple(-1, "", "");
  else {
    if (height == -1)
      height = stoi(getDataByKey(DBType::BLOCK_LATEST, "hgt"));
    string block_meta_header =
        getDataByKey(DBType::BLOCK_RAW, block_id + "_b64");

    json transaction_json;
    vector<string> tx_ids_list;
    string tx_ids = getDataByKey(DBType::BLOCK_HEADER, block_id + "_txids");
    int tx_pos = 0;
    if (!tx_ids.empty()) {
      json tx_ids_json = json::parse(tx_ids);

      for (size_t tx_ids_idx = 0; tx_ids_idx < tx_ids_json.size();
           ++tx_ids_idx) {
        string tx_id = tx_ids_json[tx_ids_idx];
        transaction_json["tx"][tx_pos]["txid"] = tx_id;
        transaction_json["tx"][tx_pos]["time"] =
            getDataByKey(DBType::BLOCK_BODY, tx_id + "_time");
        transaction_json["tx"][tx_pos]["rID"] =
            getDataByKey(DBType::BLOCK_BODY, tx_id + "_rID");
        transaction_json["tx"][tx_pos]["rSig"] =
            getDataByKey(DBType::BLOCK_BODY, tx_id + "_rSig");
        string type = getDataByKey(DBType::BLOCK_BODY, tx_id + "_type");
        transaction_json["tx"][tx_pos]["type"] = type;
        json content =
            json::parse(getDataByKey(DBType::BLOCK_BODY, tx_id + "_content"));
        if (type == "certificates" || type == "digests") {
          for (size_t i = 0; i < content.size(); ++i)
            transaction_json["tx"][tx_pos]["content"][i] =
                content[i].get<string>();
        }
        ++tx_pos;
      }
    }
    result = make_tuple(height, block_meta_header, transaction_json);
  }
  return result;
}

vector<string> Storage::findSibling(const string &tx_id) {
  string tx_id_pos = getDataByKey(DBType::BLOCK_BODY, tx_id + "_mPos");
  string block_id = getDataByKey(DBType::BLOCK_BODY, tx_id + "_bID");
  vector<string> siblings;

  if (!block_id.empty()) {

    string mtree_json_str =
        getDataByKey(DBType::BLOCK_BODY, block_id + "_mtree");
    if (!mtree_json_str.empty()) {
      json mtree_json = json::parse(mtree_json_str);

      int iteration_size = int(log2(MAX_MERKLE_LEAVES)); // log2(4096)=12
      int tx_id_pos_int = stoi(tx_id_pos);
      int node = tx_id_pos_int;
      int size = MAX_MERKLE_LEAVES;
      for (size_t i = 0; i < iteration_size; ++i) {
        node = (node % 2 != 0) ? node - 1 : node + 1;
        string sibling = mtree_json[node].get<string>();
        if (sibling.empty()) {
          siblings.clear();
          break;
        } else {
          siblings.emplace_back(sibling);
          tx_id_pos_int /= 2;
          if (i != 0)
            size += MAX_MERKLE_LEAVES / pow(2, i);
          node = size + tx_id_pos_int;
        }
      }
    }
  }

  return siblings;
}

string Storage::findCertificate(const uint64_t user_id,
                                const timestamp_type &at_this_time) {
  BytesBuilder b64_id_builder;
  b64_id_builder.append(user_id);

  return findCertificate(macaron::Base64::Encode(b64_id_builder.getString()),
                         at_this_time);
}
} // namespace gruut
