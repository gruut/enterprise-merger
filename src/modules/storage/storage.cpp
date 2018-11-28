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
}

Storage::~Storage() {
  delete m_db_block_header;
  delete m_db_block_binary;
  delete m_db_latest_block_header;
  delete m_db_transaction;
  delete m_db_certificate;

  m_db_block_header = nullptr;
  m_db_block_binary = nullptr;
  m_db_latest_block_header = nullptr;
  m_db_transaction = nullptr;
  m_db_certificate = nullptr;
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

      string transaction_id = data[transaction_iterator_index]["txID"];
      for (auto it = data[transaction_iterator_index].cbegin();
           it != data[transaction_iterator_index].cend(); ++it) {
        if (it.key() == "type" &&
            it.value() == "certificate") { // Certification 저장
          auto new_key =
              "certificate_" +
              data[transaction_iterator_index]["content"]["nID"].get<string>();
          auto new_value =
              data[transaction_iterator_index]["content"]["x509"].get<string>();
          auto status =
              m_db_certificate->Put(m_write_options, new_key, new_value);
          handleTrivialError(status);
        }

        auto new_key = "transaction_" + transaction_id + '_' + it.key();

        string new_value;
        if (it.value().is_object()) // Content 저장
          new_value = it.value().dump();
        else
          new_value = it.value().get<string>();

        auto status =
            m_db_transaction->Put(m_write_options, new_key, new_value);
        handleTrivialError(status);
      }
    }
    // TODO : mtree
  }
}

string Storage::findBy(const string &what, const string &id = "",
                       const string &field = "") {
  string key, value;

  if (id == "")
    key = "latest_block_header_" + field;
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
  if (!status.ok())
    return "";

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
  int ret_pos = 1;
  for (std::string token; std::getline(iss, token, '_');) {
    if (token == tx_id)
      return ret_pos;
    ++ret_pos;
  }
  return -1;
}
} // namespace gruut