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

  auto setting = Setting::getInstance();

  auto storage = Storage::getInstance();
  tuple<string, string, size_t> latest_block_info =
      storage->findLatestBlockBasicInfo();

  PartialBlock block;

  block.merger_id = setting->getMyId();
  block.chain_id = setting->getLocalChainId();

  if (std::get<0>(latest_block_info).empty())
    block.height = 1; // this is genesis block
  else
    block.height = std::get<2>(latest_block_info) + 1;

  block.transaction_root = transactions_digest.back();
  block.transactions = transactions;

  return block;
}

void BlockGenerator::generateBlock(PartialBlock partial_block,
                                   vector<Signature> &support_sigs,
                                   MerkleTree &merkle_tree) {

  // step 1) preparing basic data

  auto setting = Setting::getInstance();
  auto storage = Storage::getInstance();

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
    prev_header_id_b64 = std::get<0>(latest_block_info);
    prev_header_hash_b64 = std::get<1>(latest_block_info);
  }

  vector<transaction_id_type> transaction_ids;
  transform(partial_block.transactions.begin(),
            partial_block.transactions.end(), back_inserter(transaction_ids),
            [](Transaction &t) { return t.getId(); });

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

  vector<string> tx_ids_b64;
  std::transform(transaction_ids.begin(), transaction_ids.end(),
                 back_inserter(tx_ids_b64),
                 [](const transaction_id_type &transaction_id) {
                   return TypeConverter::toBase64Str(transaction_id);
                 });
  block_header["txids"] = tx_ids_b64;

  for (size_t i = 0; i < support_sigs.size(); ++i) {

    block_header["SSig"][i]["sID"] =
        TypeConverter::toBase64Str(support_sigs[i].signer_id);
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
  block_raw_builder.append(header_length, 4);               // 4-bytes
  block_raw_builder.append(compressed_json);

  string rsa_sk = setting->getMySK();
  string rsa_sk_pass = setting->getMyPass();
  auto signature =
      RSA::doSign(rsa_sk, block_raw_builder.getBytes(), true, rsa_sk_pass);
  block_raw_builder.append(signature); // == mSig

  std::string block_raw_b64 =
      TypeConverter::toBase64Str(block_raw_builder.getBytes());

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
                 back_inserter(transactions_arr),
                 [this](Transaction &tx) { return tx.getJson(); });
  block_body["tx"] = transactions_arr;

  // step-5) save block

  storage->saveBlock(block_raw_b64, block_header, block_body);

  cout << "=========================== BGT: BLOCK GENERATED (height="
       << partial_block.height << ",size=" << partial_block.transactions.size()
       << ")" << endl;

  // setp-6) send blocks to others

  OutputMsgEntry msg_header_msg;
  msg_header_msg.type = MessageType::MSG_HEADER;  // MSG_HEADER = 0xB5
  msg_header_msg.body["blockraw"] = block_header; // original = block_raw_b64
  msg_header_msg.receivers = vector<id_type>{};

  OutputMsgEntry msg_block_msg;
  msg_block_msg.type = MessageType::MSG_BLOCK; // MSG_BLOCK = 0xB4
  msg_block_msg.body["blockraw"] = block_raw_b64;
  msg_block_msg.body["tx"] = block_body["tx"];
  msg_block_msg.receivers = vector<id_type>{};

  MessageProxy proxy;
  proxy.deliverOutputMessage(msg_header_msg);
  proxy.deliverOutputMessage(msg_block_msg);
}
} // namespace gruut
