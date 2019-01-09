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

  block_height_type req_block_height = Safe::getInt(entry.body, "hgt");

  if (Safe::getString(entry.body, "mCert").empty() ||
      Safe::getString(entry.body, "mSig").empty()) {
    // TODO : check whether the requester is trustworthy or not
  } else {
    BytesBuilder msg_builder;
    msg_builder.appendB64(Safe::getString(entry.body, "mID"));
    msg_builder.appendDec(Safe::getString(entry.body, "time"));
    msg_builder.append(Safe::getString(entry.body, "mCert"));
    msg_builder.append(req_block_height);

    if (!ECDSA::doVerify(Safe::getString(entry.body, "mCert"),
                         msg_builder.getBytes(),
                         Safe::getBytesFromB64(entry.body, "mSig"))) {

      CLOG(ERROR, "BPRO") << "Invalid mSig on MSG_REQ_BLOCK";

      return false;
    }
  }

  read_block_type saved_block = m_storage->readBlock(req_block_height);

  id_type recv_id = Safe::getBytesFromB64<id_type>(entry.body, "mID");

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

  Block recv_block;
  if (!recv_block.initialize(entry.body)) {
    CLOG(ERROR, "BPRO") << "Block dropped (missing information)";
    return false;
  }

  if (!recv_block.isValid()) {
    CLOG(ERROR, "BPRO") << "Block dropped (invalid block)";
    return false;
  }

  auto latest_block_link = m_storage->getNthBlockLinkInfo();

  if (recv_block.getHeight() <= latest_block_link.height) {
    CLOG(ERROR, "BPRO") << "Block dropped (duplicated block)";
    return false;
  }

  if (recv_block.getHeight() == latest_block_link.height + 1) {

    if (recv_block.getPrevBlockIdB64() != latest_block_link.id_b64 &&
        recv_block.getPrevHashB64() != latest_block_link.hash_b64) {
      CLOG(ERROR, "BPRO") << "Block dropped (unlinkable)";
      return false;
    }

    json block_header = recv_block.getBlockHeaderJson();
    bytes block_raw = recv_block.getBlockRaw();
    json block_body = recv_block.getBlockBodyJson();

    m_storage->saveBlock(block_raw, block_header, block_body);

    CLOG(INFO, "BPRO") << "Block saved (height=" << recv_block.getHeight()
                       << ")";

    auto &tx_pool = Application::app().getTransactionPool();
    tx_pool.removeDuplicatedTransactions(recv_block.getTxIds());
  } else {
    // TODO : push this block to unresolved block pool
    CLOG(ERROR, "BPRO")
        << "Unable to handle this block due to implementation issue";
    return false;
  }

  return true;
}

bool BlockProcessor::handleMsgReqCheck(InputMsgEntry &entry) {
  OutputMsgEntry msg_res_check;

  auto setting = Setting::getInstance();

  proof_type proof = m_storage->getProof(Safe::getString(entry.body, "txid"));

  json proof_json = json::array();
  for (auto &sibling : proof.siblings) {
    proof_json.push_back(
        json{{"side", sibling.first}, {"val", sibling.second}});
  }

  msg_res_check.type = MessageType::MSG_RES_CHECK;
  msg_res_check.body["mID"] = TypeConverter::encodeBase64(setting->getMyId());
  msg_res_check.body["time"] = to_string(Time::now_int());
  msg_res_check.body["blockID"] = proof.block_id_b64;
  msg_res_check.body["proof"] = proof_json;

  m_msg_proxy.deliverOutputMessage(msg_res_check);

  return true;
}
} // namespace gruut