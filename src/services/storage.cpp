#include "storage.hpp"
#include "../chain/transaction.hpp"
#include "../utils/type_converter.hpp"

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
            auto new_key =
                "certificate_" + content[content_index].get<string>();
            auto new_value = content[content_index + 1].get<string>();
            auto status =
                m_db_certificate->Put(m_write_options, new_key, new_value);
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

void Storage::saveBlock(Block &block) {
  // Meta
  BytesBuilder meta_header_builder;

  string compression_algo_type_str;
  if (block.compression_algo_type == CompressionAlgorithmType::LZ4)
    compression_algo_type_str = "LZ4";
  else
    compression_algo_type_str = "NONE";

  meta_header_builder.append(compression_algo_type_str);
  meta_header_builder.append(block.header_length);
  string meta_header = meta_header_builder.getString();

  // Header
  json block_header;
  block_header["ver"] = to_string(block.version);
  auto chain_id_str = to_string(block.chain_id);
  block_header["cID"] = TypeConverter::toBase64Str(chain_id_str);
  block_header["prevH"] =
      TypeConverter::toBase64Str(block.previous_header_hash);
  block_header["prevbID"] = TypeConverter::toBase64Str(block.previous_block_id);
  block_header["time"] = to_string(block.timestamp);
  block_header["hgt"] = block.height;
  block_header["txrt"] = TypeConverter::toBase64Str(block.transaction_root);

  vector<string> tx_ids;
  std::transform(block.transaction_ids.begin(), block.transaction_ids.end(),
                 back_inserter(tx_ids),
                 [](const transaction_id_type &transaction_id) {
                   return TypeConverter::toBase64Str(transaction_id);
                 });
  block_header["txids"] = tx_ids;

  unordered_map<signer_id_type, signature_type> sig_map;
  std::for_each(block.signer_signatures.begin(), block.signer_signatures.end(),
                [&sig_map](Signature &sig) {
                  sig_map[sig.signer_id] = sig.signer_signature;
                });
  block_header["SSig"] = sig_map;

  auto merger_id_str = to_string(block.merger_id);
  block_header["mID"] = TypeConverter::toBase64Str(merger_id_str);

  BytesBuilder b_id_builder;
  b_id_builder.append(chain_id_str);
  b_id_builder.append(block.height);
  b_id_builder.append(block.merger_id);
  auto b_id_bytes = b_id_builder.getBytes();

  auto hashed_b_id = Sha256::hash(b_id_bytes);
  block_header["bID"] = TypeConverter::toBase64Str(hashed_b_id);

  // Body
  json block_body;
  vector<string> mtree;
  auto merkle_tree = block.merkle_tree;
  auto tree_nodes_vector = merkle_tree.getMerkleTree();
  std::transform(tree_nodes_vector.begin(), tree_nodes_vector.end(),
                 back_inserter(mtree), [](sha256 &transaction_id) {
                   return TypeConverter::toBase64Str(transaction_id);
                 });
  block_body["mtree"] = mtree;

  block_body["txCnt"] = to_string(block.transactions_count);

  vector<json> transactions_arr;
  std::transform(block.transactions.begin(), block.transactions.end(),
                 back_inserter(transactions_arr), [this](Transaction &tx) {
                   json transaction_json;
                   this->toJson(transaction_json, tx);

                   return transaction_json;
                 });
  block_body["tx"] = transactions_arr;

  saveBlock(meta_header, block_header, block_body);
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

string Storage::findCertificate(const uint64_t &user_id) {
  BytesBuilder b64_id_builder;
  b64_id_builder.append(user_id);

  return findCertificate(macaron::Base64::Encode(b64_id_builder.getString()));
}

void Storage::toJson(json &j, const Transaction &tx) {
  auto tx_id = TypeConverter::toBase64Str(tx.transaction_id);
  auto tx_time = TypeConverter::toBase64Str(tx.sent_time);
  auto requester_id = TypeConverter::toBase64Str(tx.requestor_id);

  std::string transaction_type_str;
  if (tx.transaction_type == TransactionType::CERTIFICATE)
    transaction_type_str = "CERTIFICATE";
  else
    transaction_type_str = "DIGESTS";
  auto transaction_type = TypeConverter::toBase64Str(transaction_type_str);

  auto signature = TypeConverter::toBase64Str(tx.signature);

  std::vector<std::string> encoded_content_list;
  std::transform(tx.content_list.cbegin(), tx.content_list.cend(),
                 back_inserter(encoded_content_list),
                 [](const content_type &content) {
                   return TypeConverter::toBase64Str(content);
                 });

  j = json{{"txID", tx_id},       {"time", tx_time},
           {"rID", requester_id}, {"type", transaction_type},
           {"rSig", signature},   {"content", encoded_content_list}};
}
} // namespace gruut
