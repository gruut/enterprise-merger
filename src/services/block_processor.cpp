#include "block_processor.hpp"
#include "../application.hpp"
#include "easy_logging.hpp"

namespace gruut {

BlockProcessor::BlockProcessor() {
  m_storage = Storage::getInstance();
  auto setting = Setting::getInstance();
  m_my_id = setting->getMyId();
  el::Loggers::getLogger("BPRO");
}

bool BlockProcessor::handleMessage(InputMsgEntry &entry) {
  switch (entry.type) {
  case MessageType::MSG_REQ_BLOCK:
    return handleMsgReqBlock(entry);
  case MessageType::MSG_BLOCK:
    return handleMsgBlock(entry);
  case MessageType::MSG_REQ_CHECK:
    return handleMsgReqCheck(entry);
  default:
    return false;
  }
}

bool BlockProcessor::handleMsgReqBlock(InputMsgEntry &entry) {

  CLOG(INFO, "BPRO") << "called handleMsgReqBlock()";

  size_t req_block_height =
      static_cast<size_t>(stoll(Safe::getString(entry.body["hgt"])));

  if (Safe::getString(entry.body["mCert"]).empty() ||
      Safe::getString(entry.body["mSig"]).empty()) {
    // TODO : check whether the requester is trustworthy or not
  } else {
    BytesBuilder msg_builder;
    msg_builder.appendB64(Safe::getString(entry.body["mID"]));
    msg_builder.appendDec(Safe::getString(entry.body["time"]));
    msg_builder.append(Safe::getString(entry.body["mCert"]));
    msg_builder.append(req_block_height);

    BytesBuilder sig_builder;
    sig_builder.appendB64(Safe::getString(entry.body["mSig"]));

    if (!RSA::doVerify(Safe::getString(entry.body["mCert"]),
                       msg_builder.getString(), sig_builder.getBytes(), true)) {

      CLOG(ERROR, "BPRO") << "Invalid mSig on MSG_REQ_BLOCK";

      return false;
    }
  }

  read_block_type saved_block = m_storage->readBlock(req_block_height);

  id_type recv_id =
      TypeConverter::decodeBase64(Safe::getString(entry.body["mID"]));

  if (saved_block.height <= 0) {

    OutputMsgEntry output_message;
    output_message.type = MessageType::MSG_ERROR;
    output_message.body["sender"] =
        TypeConverter::encodeBase64(m_my_id); // my_id
    output_message.body["time"] = Time::now();
    output_message.body["type"] =
        std::to_string(static_cast<int>(ErrorMsgType::BSYNC_NO_BLOCK));
    output_message.body["info"] = "no block!";
    output_message.receivers = {recv_id};

    CLOG(INFO, "BPRO") << "Send MSG_ERROR (no block)";

    m_msg_proxy.deliverOutputMessage(output_message);

    return false;
  }

  OutputMsgEntry msg_block;
  msg_block.type = MessageType::MSG_BLOCK;
  msg_block.body["mID"] = TypeConverter::encodeBase64(m_my_id);
  msg_block.body["blockraw"] =
      TypeConverter::encodeBase64(saved_block.block_raw);
  msg_block.body["tx"] = saved_block.txs;
  msg_block.receivers = {recv_id};

  CLOG(INFO, "BPRO") << "Send MSG_BLOCK (height=" << req_block_height
                     << ", #TX=" << saved_block.txs.size() << ")";

  m_msg_proxy.deliverOutputMessage(msg_block);

  return true;
}

bool BlockProcessor::handleMsgBlock(InputMsgEntry &entry) {
  std::string block_raw_str = Safe::getString(entry.body["blockraw"]);
  bytes block_raw = TypeConverter::decodeBase64(block_raw_str);

  nlohmann::json block_json = BlockValidator::getBlockHeaderJson(block_raw);

  if (block_json.empty())
    return false;

  std::vector<sha256> full_merkle_tree_nodes;
  std::vector<transaction_id_type> tx_ids;

  if (!BlockValidator::validateAndGetTree(block_json, entry.body["tx"],
                                          full_merkle_tree_nodes, tx_ids))
    return false;

  size_t num_txs = entry.body["tx"].size(); // entry.body["tx"] must be array

  std::vector<std::string> mtree_nodes_b64(num_txs);

  for (size_t i = 0; i < num_txs; ++i) {
    mtree_nodes_b64[i] = TypeConverter::encodeBase64(full_merkle_tree_nodes[i]);
  }

  nlohmann::json block_body;
  block_body["tx"] = entry.body["tx"];
  block_body["txCnt"] = to_string(num_txs);
  block_body["mtree"] = mtree_nodes_b64;

  m_storage->saveBlock(block_raw, block_json, block_body);

  CLOG(INFO, "BPRO") << "Block saved (height="
                     << Safe::getString(block_json["hgt"]) << ")";

  auto &tx_pool = Application::app().getTransactionPool();
  tx_pool.removeDuplicatedTransactions(tx_ids);

  return true;
}

bool BlockProcessor::handleMsgReqCheck(InputMsgEntry &entry) {
  OutputMsgEntry msg_res_check;

  auto setting = Setting::getInstance();
  merger_id_type my_mid = setting->getMyId();
  timestamp_type timestamp = Time::now_int();

  proof_type proof =
      m_storage->findSibling(Safe::getString(entry.body["txid"]));

  json proof_json = json::array();
  for (auto &sibling : proof.siblings) {
    proof_json.push_back(
        nlohmann::json{{"side", sibling.first}, {"val", sibling.second}});
  }

  msg_res_check.type = MessageType::MSG_RES_CHECK;
  msg_res_check.body["mID"] = TypeConverter::encodeBase64(my_mid);
  msg_res_check.body["time"] = to_string(timestamp);
  msg_res_check.body["blockID"] = proof.block_id_b64;
  msg_res_check.body["proof"] = proof_json;

  m_msg_proxy.deliverOutputMessage(msg_res_check);

  return true;
}
} // namespace gruut