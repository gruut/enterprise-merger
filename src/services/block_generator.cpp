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

  // step-1) make block

  Block new_block;
  new_block.initWithParitalBlock(partial_block, merkle_tree.getMerkleTree());
  new_block.setSupportSigs(support_sigs);
  new_block.linkPreviousBlock();
  new_block.refreshBlockRaw();

  json block_header = new_block.getBlockHeaderJson();
  bytes block_raw = new_block.getBlockRaw();
  json block_body = new_block.getBlockBodyJson();

  // step-2) save block

  auto storage = Storage::getInstance();
  auto setting = Setting::getInstance();

  storage->saveBlock(block_raw, block_header, block_body);

  CLOG(INFO, "BGEN") << "BLOCK GENERATED (height=" << new_block.getHeight()
                     << ",#tx=" << new_block.getNumTransactions() << ")";

  // setp-3) send blocks to others

  OutputMsgEntry msg_header_msg;
  msg_header_msg.type = MessageType::MSG_HEADER;  // MSG_HEADER = 0xB5
  msg_header_msg.body["blockraw"] = block_header; // original = block_raw_b64
  msg_header_msg.receivers = std::vector<id_type>{};

  OutputMsgEntry msg_block_msg;
  msg_block_msg.type = MessageType::MSG_BLOCK; // MSG_BLOCK = 0xB4
  msg_block_msg.body["mID"] = TypeConverter::encodeBase64(setting->getMyId());
  msg_block_msg.body["blockraw"] = TypeConverter::encodeBase64(block_raw);
  msg_block_msg.body["tx"] = block_body["tx"];
  msg_block_msg.receivers = std::vector<id_type>{};

  MessageProxy proxy;
  proxy.deliverOutputMessage(msg_header_msg);
  proxy.deliverOutputMessage(msg_block_msg);
}
} // namespace gruut
