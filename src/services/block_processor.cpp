#include "block_processor.hpp"
#include "../application.hpp"

namespace gruut {

BlockProcessor::BlockProcessor() { m_storage = Storage::getInstance(); }

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
      return false;
    }
  }

  auto saved_block =
      m_storage->readBlock(stoi(entry.body["hgt"].get<std::string>()));

  if (std::get<0>(saved_block) < 0) {
    return false;
  }

  std::string recv_id_b64 = entry.body["mID"].get<std::string>();
  id_type recv_id = TypeConverter::decodeBase64(recv_id_b64);

  OutputMsgEntry msg_block;
  msg_block.type = MessageType::MSG_BLOCK;
  msg_block.body["blockraw"] = std::get<1>(saved_block);
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

  std::vector<std::string> mtree_nodes_b64;

  for (size_t i = 0; i < mtree_nodes.size(); ++i) {
    mtree_nodes_b64[i] = TypeConverter::toBase64Str(mtree_nodes[i]);
  }

  nlohmann::json block_secret;
  block_secret["tx"] = entry.body["tx"];
  block_secret["txCnt"] = entry.body["tx"].size();
  block_secret["mtree"] = mtree_nodes_b64;

  m_storage->saveBlock(entry.body["blockraw"].get<std::string>(), block_json,
                       block_secret);

  auto &tx_pool = Application::app().getTransactionPool();
  tx_pool.removeDuplicatedTransactions(tx_ids);

  return true;
}

bool BlockProcessor::handleMsgReqCheck(InputMsgEntry &entry) {
  OutputMsgEntry msg_res_check;

  auto setting = Setting::getInstance();
  merger_id_type my_mid = setting->getMyId();
  timestamp_type timestamp = Time::now_int();

  msg_res_check.type = MessageType::MSG_RES_CHECK;
  msg_res_check.body["mID"] = TypeConverter::toBase64Str(my_mid);
  msg_res_check.body["time"] = to_string(timestamp);
  msg_res_check.body["proof"] = m_storage->findSibling(
      entry.body["txid"].get<std::string>()); // even if sibilings is empty, do
                                              // not send error message

  m_msg_proxy.deliverOutputMessage(msg_res_check);

  return true;
}
} // namespace gruut