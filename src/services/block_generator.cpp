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

#include "easy_logging.hpp"

using namespace std;

namespace gruut {

BlockGenerator::BlockGenerator() { el::Loggers::getLogger("BGEN"); }

PartialBlock
BlockGenerator::generatePartialBlock(sha256 &merkle_root,
                                     vector<Transaction> &transactions) {

  auto setting = Setting::getInstance();

  auto storage = Storage::getInstance();
  tuple<string, string, size_t> latest_block_info =
      storage->findLatestBlockBasicInfo();

  PartialBlock partial_block;

  partial_block.time = Time::now_int();
  partial_block.merger_id = setting->getMyId();
  partial_block.chain_id = setting->getLocalChainId();

  if (std::get<0>(latest_block_info).empty())
    partial_block.height = 1; // this is genesis block
  else
    partial_block.height = std::get<2>(latest_block_info) + 1;

  partial_block.transaction_root = merkle_root;
  partial_block.transactions = transactions;

  return partial_block;
}

void BlockGenerator::generateBlock(PartialBlock partial_block,
                                   vector<Signature> support_sigs,
                                   MerkleTree merkle_tree) {

  // step 1) preparing basic data

  auto setting = Setting::getInstance();
  auto storage = Storage::getInstance();

  block_version_type version = 1;
  std::tuple<string, string, size_t> latest_block_info =
      storage->findLatestBlockBasicInfo();

  std::string prev_header_id_b64, prev_header_hash_b64;

  if (std::get<0>(latest_block_info).empty()) { // this is genesis block
    prev_header_id_b64 = config::GENESIS_BLOCK_PREV_ID_B64;
    prev_header_hash_b64 = config::GENESIS_BLOCK_PREV_HASH_B64;
  } else {
    prev_header_id_b64 = std::get<0>(latest_block_info);
    prev_header_hash_b64 =
        TypeConverter::encodeBase64(std::get<1>(latest_block_info));
  }

  std::vector<transaction_id_type> transaction_ids;
  std::transform(
      partial_block.transactions.begin(), partial_block.transactions.end(),
      back_inserter(transaction_ids), [](Transaction &t) { return t.getId(); });

  BytesBuilder block_id_builder;
  block_id_builder.append(partial_block.chain_id);
  block_id_builder.append(partial_block.height);
  block_id_builder.append(partial_block.merger_id);
  auto block_id =
      static_cast<block_id_type>(Sha256::hash(block_id_builder.getString()));

  std::string my_id_b64 = TypeConverter::encodeBase64(partial_block.merger_id);

  // step 2) making block_header (JSON)

  json block_header;
  block_header["ver"] = to_string(version);
  block_header["cID"] = TypeConverter::encodeBase64(partial_block.chain_id);
  block_header["prevH"] = prev_header_hash_b64;
  block_header["prevbID"] = prev_header_id_b64;
  block_header["bID"] = TypeConverter::encodeBase64(block_id);
  block_header["time"] = to_string(partial_block.time); // important!!
  block_header["hgt"] = to_string(partial_block.height);
  block_header["txrt"] =
      TypeConverter::encodeBase64(partial_block.transaction_root);

  std::vector<std::string> tx_ids_b64;
  std::transform(transaction_ids.begin(), transaction_ids.end(),
                 back_inserter(tx_ids_b64),
                 [](const transaction_id_type &transaction_id) {
                   return TypeConverter::encodeBase64(transaction_id);
                 });
  block_header["txids"] = tx_ids_b64;

  for (auto &ssig : support_sigs) {
    block_header["SSig"].push_back(json({
      {"sID", TypeConverter::encodeBase64(ssig.signer_id) },
      {"sig", TypeConverter::encodeBase64(ssig.signer_signature) }
    }));
  }

  block_header["mID"] = my_id_b64;

  // step-3) making block_raw with mSig

  bytes compressed_json = Compressor::compressData(TypeConverter::stringToBytes(
      block_header.dump())); // now, we cannot change block_raw any more

  auto header_length =
      static_cast<header_length_type>(compressed_json.size() + 5);

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

  bytes block_raw = block_raw_builder.getBytes();
  std::string block_raw_b64 = TypeConverter::encodeBase64(block_raw);

  // step-4) making block_body (JSON)

  size_t num_txs = partial_block.transactions.size();

  std::vector<std::string> mtree_node_b64(num_txs);
  std::vector<sha256> mtree_nodes = merkle_tree.getMerkleTree();

  for (size_t i = 0; i < num_txs; ++i) {
    mtree_node_b64[i] = TypeConverter::encodeBase64(mtree_nodes[i]);
  }

  std::vector<json> transactions_arr;
  std::transform(partial_block.transactions.begin(),
                 partial_block.transactions.end(),
                 back_inserter(transactions_arr),
                 [this](Transaction &tx) { return tx.getJson(); });

  nlohmann::json block_body;
  block_body["mtree"] = mtree_node_b64;
  block_body["txCnt"] = to_string(num_txs);
  block_body["tx"] = transactions_arr;

  // step-5) save block

  storage->saveBlock(block_raw, block_header, block_body);

  CLOG(INFO, "BGEN") << "BLOCK GENERATED (height=" << partial_block.height
                     << ",size=" << partial_block.transactions.size() << ")";

  // setp-6) send blocks to others

  OutputMsgEntry msg_header_msg;
  msg_header_msg.type = MessageType::MSG_HEADER;  // MSG_HEADER = 0xB5
  msg_header_msg.body["blockraw"] = block_header; // original = block_raw_b64
  msg_header_msg.receivers = vector<id_type>{};

  OutputMsgEntry msg_block_msg;
  msg_block_msg.type = MessageType::MSG_BLOCK; // MSG_BLOCK = 0xB4
  msg_block_msg.body["mID"] = my_id_b64;
  msg_block_msg.body["blockraw"] = block_raw_b64;
  msg_block_msg.body["tx"] = block_body["tx"];
  msg_block_msg.receivers = vector<id_type>{};

  MessageProxy proxy;
  proxy.deliverOutputMessage(msg_header_msg);
  proxy.deliverOutputMessage(msg_block_msg);
}
} // namespace gruut
