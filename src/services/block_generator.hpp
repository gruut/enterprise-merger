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

#include "easy_logging.hpp"

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
    new_block.initialize(basic_info, merkle_tree.getMerkleTree());
    new_block.setSupportSignatures(support_sigs);
    new_block.linkPreviousBlock(basic_info.prev_id_b64,
                                basic_info.prev_hash_b64);
    new_block.finalize();

    json block_header = new_block.getBlockHeaderJson();
    bytes block_raw = new_block.getBlockRaw();
    json block_body = new_block.getBlockBodyJson();

    CLOG(INFO, "BGEN") << "BLOCK GENERATED (height=" << new_block.getHeight()
                       << ",#tx=" << new_block.getNumTransactions()
                       << ",#ssig=" << new_block.getNumSSigs() << ")";

    // setp-2) send blocks to others

    auto setting = Setting::getInstance();

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

    OutputMsgEntry msg_block_hgt;
    msg_block_hgt.type = MessageType::MSG_BLOCK_HGT; // MSG_BLOCK_HGT = 0xB6
    msg_block_hgt.body["mID"] = msg_block_msg.body["mID"];
    msg_block_hgt.body["time"] = Time::now();
    msg_block_hgt.body["hgt"] = to_string(new_block.getHeight());
    msg_block_hgt.body["bID"] = TypeConverter::encodeBase64(new_block.getBlockId());
    msg_block_hgt.body["prevbID"] = basic_info.prev_id_b64;
    msg_block_hgt.body["prevHash"] = basic_info.prev_hash_b64;

    InputMsgEntry msg_block_msg_input;
    msg_block_msg_input.type = msg_block_msg.type;
    msg_block_msg_input.body = msg_block_msg.body;

    MessageProxy proxy;
    proxy.deliverOutputMessage(msg_header_msg);
    proxy.deliverOutputMessage(msg_block_msg);
    proxy.deliverOutputMessage(msg_block_hgt);
    proxy.deliverBlockProcessor(msg_block_msg_input);
  }
};
} // namespace gruut

#endif
