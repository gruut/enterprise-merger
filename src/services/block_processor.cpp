#include "block_processor.hpp"
#include "../application.hpp"

namespace gruut {

BlockProcessor::BlockProcessor() {
  m_storage = Storage::getInstance();
  auto setting = Setting::getInstance();
  m_my_id = setting->getMyId();
  el::Loggers::getLogger("BPU");
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
  if (entry.body["mCert"].get<std::string>().empty() ||
      entry.body["mSig"].get<std::string>().empty()) {
    // TODO : check whether the requester is trustworthy or not
  } else {
    BytesBuilder msg_builder;
    msg_builder.appendB64(entry.body["mID"].get<std::string>());
    msg_builder.appendDec(entry.body["time"].get<std::string>());
    msg_builder.append(entry.body["mCert"].get<std::string>());
    msg_builder.appendDec(entry.body["hgt"].get<std::string>());

    BytesBuilder sig_builder;
    sig_builder.appendB64(entry.body["mSig"].get<std::string>());

    if (!RSA::doVerify(entry.body["mCert"].get<std::string>(),
                       msg_builder.getString(), sig_builder.getBytes(), true)) {
      CLOG(ERROR, "BPU") << "invalid sig MSG_REQ_BLOCK";

      return false;
    }
  }

  auto saved_block =
      m_storage->readBlock(stoi(entry.body["hgt"].get<std::string>()));

  id_type recv_id =
      TypeConverter::decodeBase64(entry.body["mID"].get<std::string>());

  if (std::get<0>(saved_block) < 0) {

    OutputMsgEntry output_message;
    output_message.type = MessageType::MSG_ERROR;
    output_message.body["sender"] =
        TypeConverter::toBase64Str(m_my_id); // my_id
    output_message.body["time"] = Time::now();
    output_message.body["type"] =
        std::to_string(static_cast<int>(ErrorMsgType::BSYNC_NO_BLOCK));
    output_message.body["info"] = "no block!";
    output_message.receivers = {recv_id};

    m_msg_proxy.deliverOutputMessage(output_message);

    return false;
  }

  OutputMsgEntry msg_block;
  msg_block.type = MessageType::MSG_BLOCK;
  msg_block.body["mID"] = TypeConverter::toBase64Str(m_my_id);
  msg_block.body["blockraw"] =
      TypeConverter::toBase64Str(std::get<1>(saved_block));
  msg_block.body["tx"] = std::get<2>(saved_block);
  msg_block.receivers = {recv_id};

  m_msg_proxy.deliverOutputMessage(msg_block);

  return true;
}

bool BlockProcessor::handleMsgBlock(InputMsgEntry &entry) {
  std::string block_raw_str = entry.body["blockraw"].get<std::string>();
  bytes block_raw = TypeConverter::decodeBase64(block_raw_str);

  nlohmann::json block_json = BlockValidator::getBlockJson(block_raw);

  if (block_json.empty())
    return false;

  std::vector<sha256> mtree_nodes;
  std::vector<transaction_id_type> tx_ids;

  if (!BlockValidator::validate(block_json, entry.body["tx"], mtree_nodes,
                                tx_ids))
    return false;

  size_t num_txs = entry.body["tx"].size();

  std::vector<std::string> mtree_nodes_b64(num_txs);

  for (size_t i = 0; i < num_txs; ++i) {
    mtree_nodes_b64[i] = TypeConverter::toBase64Str(mtree_nodes[i]);
  }

  nlohmann::json block_body;
  block_body["tx"] = entry.body["tx"];
  block_body["txCnt"] = to_string(num_txs);
  block_body["mtree"] = mtree_nodes_b64;

  m_storage->saveBlock(block_raw, block_json, block_body);

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
      m_storage->findSibling(entry.body["txid"].get<std::string>());

  json proof_json = json::array();
  for (auto &sibling : proof.siblings) {
    proof_json.push_back(
        nlohmann::json{{"side", sibling.first}, {"val", sibling.second}});
  }

  msg_res_check.type = MessageType::MSG_RES_CHECK;
  msg_res_check.body["mID"] = TypeConverter::toBase64Str(my_mid);
  msg_res_check.body["time"] = to_string(timestamp);
  msg_res_check.body["blockID"] = proof.block_id;
  msg_res_check.body["proof"] = proof_json;

  m_msg_proxy.deliverOutputMessage(msg_res_check);

  return true;
}
} // namespace gruut