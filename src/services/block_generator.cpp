#include "block_generator.hpp"
#include "../chain/merkle_tree.hpp"
#include "../chain/signature.hpp"
#include "../chain/types.hpp"
#include "../src/application.hpp"
#include "../utils/bytes_builder.hpp"
#include "../utils/compressor.hpp"
#include "../utils/rsa.hpp"
#include "../utils/time.hpp"
#include "../utils/type_converter.hpp"
#include "message_proxy.hpp"

using namespace std;

namespace gruut {
PartialBlock
BlockGenerator::generatePartialBlock(vector<sha256> &transactions_digest,
                                     vector<Transaction> &transactions) {

  Storage *storage = Storage::getInstance();
  tuple<string, string, size_t> latest_block_info =
      storage->findLatestBlockBasicInfo();

  PartialBlock block;

  // TODO: 설정파일이 없어서 하드코딩(1)
  block.merger_id = TypeConverter::integerToBytes((uint64_t)1); // for 8-byte
  // TODO: 위와 같은 이유로 임시값 할당
  block.chain_id = TypeConverter::integerToArray<CHAIN_ID_TYPE_SIZE>(1);

  if (std::get<0>(latest_block_info).empty())
    block.height = 1; // this is genesis block
  else
    block.height = std::get<2>(latest_block_info) + 1;

  block.transaction_root = transactions_digest.back();
  block.transactions = transactions;

  return block;
}

void BlockGenerator::generateBlock(PartialBlock &partial_block,
                                   vector<Signature> &support_sigs,
                                   MerkleTree &merkle_tree) {

  // step 1) preparing basic data

  Storage *storage = Storage::getInstance();

  block_version_type version = 1;
  tuple<string, string, size_t> latest_block_info =
      storage->findLatestBlockBasicInfo();

  auto timestamp = Time::now_int();

  std::string prev_header_id_b64;
  std::string prev_header_hash_b64;

  if (std::get<0>(latest_block_info).empty()) { // this is genesis block
    prev_header_id_b64 = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=";
    prev_header_hash_b64 = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=";
  } else {
    prev_header_id_b64 = std::get<1>(latest_block_info);
    prev_header_hash_b64 = std::get<2>(latest_block_info);
  }

  vector<transaction_id_type> transaction_ids;
  transform(partial_block.transactions.begin(),
            partial_block.transactions.end(), back_inserter(transaction_ids),
            [](Transaction &t) { return t.transaction_id; });

  BytesBuilder b_id_builder;
  b_id_builder.append(partial_block.chain_id);
  b_id_builder.append(partial_block.height);
  b_id_builder.append(partial_block.merger_id);
  auto hashed_b_id = Sha256::hash(b_id_builder.getString());

  // step 2) making block_header (JSON)

  json block_header;
  block_header["ver"] = to_string(version);
  block_header["cID"] = TypeConverter::toBase64Str(partial_block.chain_id);
  block_header["prevH"] = prev_header_hash_b64;
  block_header["prevbID"] = prev_header_id_b64;
  block_header["bID"] = TypeConverter::toBase64Str(hashed_b_id);
  block_header["time"] = to_string(timestamp);
  block_header["hgt"] = to_string(partial_block.height);
  block_header["txrt"] =
      TypeConverter::toBase64Str(partial_block.transaction_root);

  vector<string> tx_ids;
  std::transform(transaction_ids.begin(), transaction_ids.end(),
                 back_inserter(tx_ids),
                 [](const transaction_id_type &transaction_id) {
                   return TypeConverter::toBase64Str(transaction_id);
                 });
  block_header["txids"] = tx_ids;

  vector<unordered_map<signer_id_type, signature_type>> sig_map;
  for (size_t i = 0; i < support_sigs.size(); ++i) {
    // TODO :: 나중에  signer 아이디 형태를 바꿀 때 수정할 것
    bytes signer_id_bytes =
        TypeConverter::integerToBytes(support_sigs[i].signer_id);
    block_header["SSig"][i]["sID"] =
        TypeConverter::toBase64Str(signer_id_bytes);
    block_header["SSig"][i]["sig"] =
        TypeConverter::toBase64Str(support_sigs[i].signer_signature);
  }

  block_header["mID"] = TypeConverter::toBase64Str(partial_block.merger_id);

  // step-3) making block_raw with mSig

  header_length_type header_length = 1;
  bytes compressed_json = Compressor::compressData(
      TypeConverter::stringToBytes(block_header.dump()));

  BytesBuilder block_raw_builder;
  block_raw_builder.append(
      static_cast<uint8_t>(config::COMPRESSION_ALGO_TYPE)); // 1-byte
  block_raw_builder.append(header_length);                  // 4-bytes
  block_raw_builder.append(compressed_json);

  string rsa_sk = ""; // TODO : from config
  auto signature = RSA::doSign(rsa_sk, block_raw_builder.getBytes(), true);
  block_raw_builder.append(signature); // == mSig

  bytes block_raw_bytes = block_raw_builder.getBytes();
  std::string block_raw_b64 = TypeConverter::toBase64Str(block_raw_bytes);

  // step-4) making block_body (JSON)
  json block_body;
  vector<string> mtree;
  auto tree_nodes_vector = merkle_tree.getMerkleTree();
  std::transform(tree_nodes_vector.begin(), tree_nodes_vector.end(),
                 back_inserter(mtree), [](sha256 &transaction_id) {
                   return TypeConverter::toBase64Str(transaction_id);
                 });
  block_body["mtree"] = mtree;
  block_body["txCnt"] = to_string(partial_block.transactions.size());

  vector<json> transactions_arr;
  std::transform(partial_block.transactions.begin(),
                 partial_block.transactions.end(),
                 back_inserter(transactions_arr), [this](Transaction &tx) {
                   json transaction_json;
                   this->toJson(transaction_json, tx);
                   return transaction_json;
                 });
  block_body["tx"] = transactions_arr;

  // step-5) save block

  storage->saveBlock(block_raw_b64, block_header, block_body);

  // setp-6) send blocks to others

  json msg_header; // MSG_HEADER = 0xB5
  msg_header["blockraw"] = block_raw_b64;

  json msg_block; // MSG_BLOCK = 0xB4
  msg_block["blockraw"] = block_raw_b64;
  msg_block["tx"] = block_body["tx"];

  // TODO : change to use OutputQueueAlt

  auto msg_header_msg =
      std::make_tuple(MessageType::MSG_HEADER, vector<uint64_t>{}, msg_header);
  auto msg_block_msg =
      std::make_tuple(MessageType::MSG_BLOCK, vector<uint64_t>{}, msg_block);

  MessageProxy proxy;
  proxy.deliverOutputMessage(msg_header_msg);
  proxy.deliverOutputMessage(msg_block_msg);
}

void BlockGenerator::toJson(json &j, const Transaction &tx) {
  auto tx_id = TypeConverter::toBase64Str(tx.transaction_id);
  auto tx_time = TypeConverter::toString(tx.sent_time);
  auto requester_id = TypeConverter::toBase64Str(tx.requestor_id);
  string transaction_type;
  if (tx.transaction_type == TransactionType::CERTIFICATE)
    transaction_type = TXTYPE_CERTIFICATES;
  else
    transaction_type = TXTYPE_DIGESTS;
  auto signature = TypeConverter::toBase64Str(tx.signature);

  j = json{{"txID", tx_id},       {"time", tx_time},
           {"rID", requester_id}, {"type", transaction_type},
           {"rSig", signature},   {"content", tx.content_list}};
}
} // namespace gruut
