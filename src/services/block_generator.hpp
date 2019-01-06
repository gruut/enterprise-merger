#ifndef GRUUT_ENTERPRISE_MERGER_BLOCK_GENERATOR_HPP
#define GRUUT_ENTERPRISE_MERGER_BLOCK_GENERATOR_HPP

#include "../chain/block.hpp"
#include "../chain/merkle_tree.hpp"
#include "../chain/signature.hpp"
#include "../chain/transaction.hpp"
#include "../chain/types.hpp"
#include "../utils/bytes_builder.hpp"
#include "../utils/compressor.hpp"
#include "../utils/rsa.hpp"
#include "../utils/time.hpp"
#include "../utils/type_converter.hpp"

#include "message_proxy.hpp"
#include "signature_requester.hpp"

#include <vector>

using namespace std;

namespace gruut {
class BlockGenerator {
public:
  BlockGenerator() { el::Loggers::getLogger("BGEN"); }

  // argument must be call-by-value due to multi-thread safe
  void generateBlock(BasicBlockInfo basic_info, vector<Signature> support_sigs,
                     MerkleTree merkle_tree) {

    // step-1) make block

    Block new_block;
    new_block.initalize(basic_info, merkle_tree.getMerkleTree());
    new_block.setSupportSignatures(support_sigs);
    new_block.linkPreviousBlock();
    new_block.finalize();

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
};
} // namespace gruut

#endif
