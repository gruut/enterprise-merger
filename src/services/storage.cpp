#include "storage.hpp"

namespace gruut {
using namespace std;
using namespace nlohmann;
using namespace macaron;

Storage::Storage() {
  m_options.create_if_missing = true;
  m_write_options.sync = true;

  handleCriticalError(
      leveldb::DB::Open(m_options, "./block_header", &m_db_block_header));
  handleCriticalError(leveldb::DB::Open(m_options, "./block_meta_header",
                                        &m_db_block_meta_header));
  handleCriticalError(leveldb::DB::Open(m_options, "./latest_block_header",
                                        &m_db_latest_block_header));
  handleCriticalError(
      leveldb::DB::Open(m_options, "./block_body", &m_db_block_body));
  handleCriticalError(
      leveldb::DB::Open(m_options, "./certificate", &m_db_certificate));
  handleCriticalError(
      leveldb::DB::Open(m_options, "./blockid_height", &m_db_blockid_height));
}

Storage::~Storage() {
  delete m_db_block_header;
  delete m_db_block_meta_header;
  delete m_db_latest_block_header;
  delete m_db_block_body;
  delete m_db_certificate;
  delete m_db_blockid_height;

  m_db_block_header = nullptr;
  m_db_block_meta_header = nullptr;
  m_db_latest_block_header = nullptr;
  m_db_block_body = nullptr;
  m_db_certificate = nullptr;
  m_db_blockid_height = nullptr;
}

void Storage::write(const string &what, json &data,
                    const string &block_id = "") {
  if (what == "block_header" || what == "latest_block_header" ||
      what == "block_meta_header") {
    for (auto it = data.cbegin(); it != data.cend(); ++it) {
      if (it.key() == "bID" && what == "block_header")
        continue;

      string prefix = (what == "block_header" || what == "block_meta_header")
                          ? what + '_'
                          : "latest_block_header";
      auto new_key = prefix + block_id + "_" + it.key();

      string new_value;
      if (it.value().is_array()) // SSig 저장
        new_value = it.value().dump();
      else
        new_value = it.value().get<string>();

      if (what == "block_header") {
        auto status =
            m_db_block_header->Put(m_write_options, new_key, new_value);
        handleTrivialError(status);
      } else if (what == "latest_block_header") {
        auto status =
            m_db_latest_block_header->Put(m_write_options, new_key, new_value);
        handleTrivialError(status);
      } else if (what == "block_meta_header") {
        auto status =
            m_db_block_meta_header->Put(m_write_options, new_key, new_value);
        handleTrivialError(status);
      }
    }
  } else if (what == "block_body") {
    unsigned int transaction_iterator_index;
    for (transaction_iterator_index = 0;
         transaction_iterator_index < data["tx"].size();
         ++transaction_iterator_index) {
      data["tx"][transaction_iterator_index]["bID"] = block_id;
      data["tx"][transaction_iterator_index]["mPos"] =
          to_string(transaction_iterator_index);

      string transaction_id = data["tx"][transaction_iterator_index]["txID"];

      for (auto it = data["tx"][transaction_iterator_index].cbegin();
           it != data["tx"][transaction_iterator_index].cend(); ++it) {
        if (it.key() == "type" &&
            it.value() == "certificates") { // Certificate 저장
          json content = data["tx"][transaction_iterator_index]["content"];
          for (auto content_index = 0; content_index < content.size();
               content_index += 2) {
            string user_id = content[content_index].get<string>();

            string cert_idx = findBy("certificate", user_id, "");
            auto new_key = "certificate_" + user_id;
            auto new_value =
                (cert_idx != "") ? to_string(stoi(cert_idx) + 1) : "0";
            auto status =
                m_db_certificate->Put(m_write_options, new_key, new_value);
            handleTrivialError(status);

            new_key = (cert_idx != "") ? "certificate_" + user_id + "_" +
                                             to_string(stoi(cert_idx) + 1)
                                       : "certificate_" + user_id + "_0";
            string pem = content[content_index + 1].get<string>();
            Botan::DataSource_Memory cert_datasource(pem);
            Botan::X509_Certificate cert(cert_datasource);

            json tmp_cert;
            tmp_cert[0] = to_string(
                Botan::X509_Time(cert.not_before()).time_since_epoch());
            tmp_cert[1] = to_string(
                Botan::X509_Time(cert.not_after()).time_since_epoch());
            tmp_cert[2] = pem;

            new_value = tmp_cert.dump();
            status = m_db_certificate->Put(m_write_options, new_key, new_value);
            handleTrivialError(status);
          }
        }

        auto new_key = "block_body_" + transaction_id + '_' + it.key();

        string new_value;
        if (it.value().is_array()) // Certificates or Digests
          new_value = it.value().dump();
        else
          new_value = it.value().get<string>();
        auto status = m_db_block_body->Put(m_write_options, new_key, new_value);
        handleTrivialError(status);
      }
    }
    auto new_key = "block_body_" + block_id;
    auto new_value = data["mtree"].dump();
    auto status = m_db_block_body->Put(m_write_options, new_key, new_value);
    handleTrivialError(status);
  } else if (what == "blockid_height") {
    auto new_key = data["hgt"].get<string>();
    auto new_value = block_id;

    auto status = m_db_blockid_height->Put(m_write_options, new_key, new_value);
    handleTrivialError(status);
  }
}

string Storage::findBy(const string &what, const string &id = "",
                       const string &field = "") {
  string key, value;

  if (id == "")
    key = (what == "latest_block_header") ? "latest_block_header_" + field
                                          : field;
  else
    key = field == "" ? what + '_' + id : what + '_' + id + '_' + field;

  leveldb::Status status;
  if (what == "latest_block_header")
    status = m_db_latest_block_header->Get(m_read_options, key, &value);
  else if (what == "block_meta_header")
    status = m_db_block_meta_header->Get(m_read_options, key, &value);
  else if (what == "block_header")
    status = m_db_block_header->Get(m_read_options, key, &value);
  else if (what == "certificate")
    status = m_db_certificate->Get(m_read_options, key, &value);
  else if (what == "block_body")
    status = m_db_block_body->Get(m_read_options, key, &value);
  else if (what == "blockid_height")
    status = m_db_blockid_height->Get(m_read_options, key, &value);
  if (!status.ok())
    value.assign("");

  return value;
}

void Storage::handleCriticalError(const leveldb::Status &status) {
  if (status.ok())
    return;
  const std::string errmsg = "Fatal LevelDB error: " + status.ToString();
  // TOOD: Logger
  cout << "You can use -debug=leveldb to get more complete diagnostic messages"
       << endl;
  throw;
}

void Storage::handleTrivialError(const leveldb::Status &status) {
  if (!status.ok()) {
    // TODO : logger
    cout << "저장 실패" << endl;
    return;
  }
}

void Storage::saveBlock(const string &block_meta_header, json &block_header,
                        json &block_body) {
  string block_id = block_header["bID"];

  write("latest_block_header", block_header, "");

  ostringstream tx_ids;
  for (size_t idx = 0; idx < block_body["tx"].size(); ++idx) {
    tx_ids << block_body["tx"][idx]["txID"].get<string>();
    tx_ids << '_';
  }
  block_header["txids"] = tx_ids.str();

  write("block_header", block_header, block_id);

  Sha256 sha;
  sha256 hash = sha.hash(block_meta_header);
  json block_meta_header_json = {{"b64", block_meta_header},
                                 {"h", sha.toString(hash)}};
  write("block_meta_header", block_meta_header_json, block_id);

  write("block_body", block_body, block_id);

  write("blockid_height", block_header, block_id);
}

pair<string, string> Storage::findLatestHashAndHeight() {
  auto block_id = findBy("latest_block_header", "", "bID");
  auto hash = findBy("block_meta_header", block_id, "h");
  auto height = findBy("latest_block_header", "", "hgt");
  return make_pair(hash, height);
}

vector<string> Storage::findLatestTxIdList() {
  auto block_id = findBy("latest_block_header", "", "bID");
  auto tx_ids = findBy("block_header", block_id, "txids");

  vector<string> tx_ids_list;
  std::istringstream iss(tx_ids);
  for (std::string token; std::getline(iss, token, '_');)
    tx_ids_list.push_back(token);
  return tx_ids_list;
}

string Storage::findCertificate(const string &user_id,
                                const timestamp_type &at_this_time) {
  string cert = "";
  string cert_size = findBy("certificate", user_id, "");
  if (cert_size != "") {
    if (at_this_time == 0)
      cert = json::parse(findBy("certificate", user_id, cert_size))[2]
                 .get<string>();
    else {
      timestamp_type max = 0;
      for (int i = 0; i <= stoi(cert_size); ++i) {
        json cert_json =
            json::parse(findBy("certificate", user_id, to_string(i)));
        timestamp_type start_date = stoi(cert_json[0].get<string>());
        timestamp_type end_date = stoi(cert_json[1].get<string>());
        if (start_date <= at_this_time && at_this_time <= end_date) {
          if (max < start_date) {
            max = start_date;
            cert = cert_json[2].get<string>();
          }
        }
      }
    }
  }
  return cert;
}

void Storage::deleteAllDirectory(const string &dir_path) {
  boost::filesystem::remove_all(dir_path);
}

int Storage::findTxIdPos(const string &blk_id, const string &tx_id) {
  string tx_list_str;
  auto status = m_db_block_header->Get(leveldb::ReadOptions(),
                                       "block_header_" + blk_id + '_' + "txids",
                                       &tx_list_str);
  if (!status.ok()) {
    return -1;
  }

  istringstream iss(tx_list_str);
  int ret_pos = 0;
  for (std::string token; std::getline(iss, token, '_');) {
    if (token == tx_id)
      return ret_pos;
    ++ret_pos;
  }
  return -1;
}

tuple<int, string, json> Storage::readBlock(int height) {
  tuple<int, string, json> result;
  string block_id = (height == -1)
                        ? findBy("latest_block_header", "", "bID")
                        : findBy("blockid_height", "", to_string(height));
  if (block_id == "")
    result = make_tuple(-1, "", "");
  else {
    if (height == -1)
      height = stoi(findBy("latest_block_header", "", "hgt"));
    string block_meta_header = findBy("block_meta_header", block_id, "b64");

    json transaction_json;
    string tx_ids = findBy("block_header", block_id, "txids");
    vector<string> tx_ids_list;
    std::istringstream iss(tx_ids);
    int tx_pos = 0;
    for (std::string tx_id; std::getline(iss, tx_id, '_');) {
      transaction_json["tx"][tx_pos]["txid"] = tx_id;
      transaction_json["tx"][tx_pos]["time"] =
          findBy("block_body", tx_id, "time");
      transaction_json["tx"][tx_pos]["rID"] =
          findBy("block_body", tx_id, "rID");
      transaction_json["tx"][tx_pos]["rSig"] =
          findBy("block_body", tx_id, "rSig");
      string type = findBy("block_body", tx_id, "type");
      transaction_json["tx"][tx_pos]["type"] = type;

      json content = json::parse(findBy("block_body", tx_id, "content"));
      if (type == "certificates" || type == "digests") {
        for (size_t i = 0; i < content.size(); ++i)
          transaction_json["tx"][tx_pos]["content"][i] =
              content[i].get<string>();
      }
      ++tx_pos;
    }
    result = make_tuple(height, block_meta_header, transaction_json);
  }
  return result;
}

vector<string> Storage::findSibling(const string &tx_id) {
  string tx_id_pos = findBy("block_body", tx_id, "mPos");
  string block_id = findBy("block_body", tx_id, "bID");
  json sibling_json = json::parse(findBy("block_body", block_id, ""));
  vector<string> siblings;

  int iteration_size = int(log2(MAX_MERKLE_LEAVES)); // log2(4096)=12
  int tx_id_pos_int = stoi(tx_id_pos);
  int node = tx_id_pos_int;
  int size = MAX_MERKLE_LEAVES;
  for (size_t i = 0; i < iteration_size; ++i) {
    node = (node % 2 != 0) ? node - 1 : node + 1;
    string sibling = sibling_json[node].get<string>();
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
