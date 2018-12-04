#include "storage.hpp"

namespace gruut {
using namespace std;
using namespace nlohmann;

Storage::Storage() {
  m_options.create_if_missing = true;
  m_write_options.sync = true;

  handleCriticalError(
      leveldb::DB::Open(m_options, "./block_header", &m_db_block_header));
  handleCriticalError(
      leveldb::DB::Open(m_options, "./block_binary", &m_db_block_binary));
  handleCriticalError(leveldb::DB::Open(m_options, "./latest_block_header",
                                        &m_db_latest_block_header));
  handleCriticalError(
      leveldb::DB::Open(m_options, "./transaction", &m_db_transaction));
  handleCriticalError(
      leveldb::DB::Open(m_options, "./certificate", &m_db_certificate));
  handleCriticalError(
      leveldb::DB::Open(m_options, "./blockid_height", &m_db_blockid_height));
}

Storage::~Storage() {
  delete m_db_block_header;
  delete m_db_block_binary;
  delete m_db_latest_block_header;
  delete m_db_transaction;
  delete m_db_certificate;
  delete m_db_blockid_height;

  m_db_block_header = nullptr;
  m_db_block_binary = nullptr;
  m_db_latest_block_header = nullptr;
  m_db_transaction = nullptr;
  m_db_certificate = nullptr;
  m_db_blockid_height = nullptr;
}

void Storage::write(const string &what, json &data,
                    const string &block_id = "") {
  if (what == "block_header" || what == "latest_block_header") {
    for (auto it = data.cbegin(); it != data.cend(); ++it) {
      if (it.key() == "bID" && what == "block_header")
        continue;

      string prefix =
          (what == "block_header") ? "block_header_" : "latest_block_header";
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
      } else {
        auto status =
            m_db_latest_block_header->Put(m_write_options, new_key, new_value);
        handleTrivialError(status);
      }
    }
  } else if (what == "block_binary") {
    auto new_key = "block_binary_" + block_id;
    auto new_value = data["block_binary"].get<string>();

    auto status = m_db_block_binary->Put(m_write_options, new_key, new_value);
    handleTrivialError(status);
  } else if (what == "transaction") {
    unsigned int transaction_iterator_index;
    for (transaction_iterator_index = 0;
         transaction_iterator_index < data.size() - 1;
         ++transaction_iterator_index) {
      data[transaction_iterator_index]["bID"] = block_id;
      data[transaction_iterator_index]["mPos"] =
          to_string(transaction_iterator_index);

      string transaction_id = data[transaction_iterator_index]["txID"];

      for (auto it = data[transaction_iterator_index].cbegin();
           it != data[transaction_iterator_index].cend(); ++it) {
        if (it.key() == "type" &&
            it.value() == "certificates") { // Certificate 저장
          json content = data[transaction_iterator_index]["content"];
          for (auto content_index = 0; content_index < content.size();
               content_index += 2) {
            auto new_key =
                "certificate_" + content[content_index].get<string>();
            auto new_value = content[content_index + 1].get<string>();
            auto status =
                m_db_certificate->Put(m_write_options, new_key, new_value);
            handleTrivialError(status);
          }
        }

        auto new_key = "transaction_" + transaction_id + '_' + it.key();

        string new_value;
        if (it.value().is_array()) // Certificates or Digests
          new_value = it.value().dump();
        else
          new_value = it.value().get<string>();
        auto status =
            m_db_transaction->Put(m_write_options, new_key, new_value);
        handleTrivialError(status);
      }
    }
    auto new_key = "transaction_" + block_id;
    auto new_value = data[transaction_iterator_index]["mtree"].dump();
    auto status = m_db_transaction->Put(m_write_options, new_key, new_value);
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
  else if (what == "block_binary")
    status = m_db_block_binary->Get(m_read_options, key, &value);
  else if (what == "block_header")
    status = m_db_block_header->Get(m_read_options, key, &value);
  else if (what == "certificate")
    status = m_db_certificate->Get(m_read_options, key, &value);
  else if (what == "transaction")
    status = m_db_transaction->Get(m_read_options, key, &value);
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

void Storage::saveBlock(const string &block_binary, json &block_header,
                        json &transaction) {
  string block_id = block_header["bID"];

  write("latest_block_header", block_header, "");

  ostringstream tx_ids;
  for (size_t idx = 0; idx < transaction.size() - 1; ++idx) {
    tx_ids << transaction[idx]["txID"].get<string>();
    tx_ids << '_';
  }
  block_header["txids"] = tx_ids.str();

  write("block_header", block_header, block_id);

  json block_binary_json = {{"block_binary", block_binary}};
  write("block_binary", block_binary_json, block_id);

  write("transaction", transaction, block_id);

  write("blockid_height", block_header, block_id);
}

pair<string, string> Storage::findLatestHashAndHeight() {
  auto block_id = findBy("latest_block_header", "", "bID");
  auto hash = findBy("block_binary", block_id, "");
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

string Storage::findCertificate(const string &user_id) {
  return findBy("certificate", user_id, "");
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
  string block_id = (height == -1)
                        ? findBy("latest_block_header", "", "bID")
                        : findBy("blockid_height", "", to_string(height));
  if (height == -1)
    height = stoi(findBy("latest_block_header", "", "hgt"));
  string block_binary = findBy("block_binary", block_id, "");

  json transaction_json;
  string tx_ids = findBy("block_header", block_id, "txids");
  vector<string> tx_ids_list;
  std::istringstream iss(tx_ids);
  int tx_pos = 0;
  for (std::string tx_id; std::getline(iss, tx_id, '_');) {
    transaction_json[tx_pos]["txID"] = tx_id;
    transaction_json[tx_pos]["time"] = findBy("transaction", tx_id, "time");
    transaction_json[tx_pos]["rID"] = findBy("transaction", tx_id, "rID");
    transaction_json[tx_pos]["rSig"] = findBy("transaction", tx_id, "rSig");
    transaction_json[tx_pos]["bID"] = findBy("transaction", tx_id, "bID");
    transaction_json[tx_pos]["mPos"] = findBy("transaction", tx_id, "mPos");
    string type = findBy("transaction", tx_id, "type");
    transaction_json[tx_pos]["type"] = type;

    json content = json::parse(findBy("transaction", tx_id, "content"));
    if (type == "certificates" || type == "digests") {
      for (size_t i = 0; i < content.size(); ++i)
        transaction_json[tx_pos]["content"][i] = content[i].get<string>();
    }
    ++tx_pos;
  }
  transaction_json[tx_pos]["mtree"] = findBy("transaction", block_id, "");

  return make_tuple(height, block_binary, transaction_json);
}

vector<string> Storage::findSibling(const string &tx_id) {
  string tx_id_pos = findBy("transaction", tx_id, "mPos");
  string block_id = findBy("transaction", tx_id, "bID");
  json sibling_json = json::parse(findBy("transaction", block_id, ""));
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

string Storage::findCertificate(const uint64_t &user_id) {
  return findCertificate(to_string(user_id));
}
} // namespace gruut
