#include "block_processor.hpp"
#include "../../application.hpp"
#include "easy_logging.hpp"

namespace gruut {

BlockProcessor::BlockProcessor() {
  m_storage = Storage::getInstance();
  auto setting = Setting::getInstance();
  m_my_id = setting->getMyId();

  auto last_block_info = m_storage->getNthBlockLinkInfo();

  m_unresolved_block_pool.setPool(last_block_info.id_b64,
                                  last_block_info.hash_b64,
                                  last_block_info.height, last_block_info.time);

  auto &io_service = Application::app().getIoService();
  m_timer.reset(new boost::asio::deadline_timer(io_service));
  m_task_strand.reset(new boost::asio::io_service::strand(io_service));

  el::Loggers::getLogger("BPRO");
}

void BlockProcessor::start() { periodicTask(); }

void BlockProcessor::periodicTask() {
  auto &io_service = Application::app().getIoService();
  io_service.post(m_task_strand->wrap([this]() {
    std::vector<Block> resolved_blocks =
        m_unresolved_block_pool.getResolvedBlocks();
    if (!resolved_blocks.empty()) {
      for (auto &&each_block : resolved_blocks) {

        if (!each_block.isValidLate()) {
          CLOG(ERROR, "BPRO")
              << "Block dropped (invalid - late stage validation)";
          continue;
        }

        json block_header = each_block.getBlockHeaderJson();
        bytes block_raw = each_block.getBlockRaw();
        json block_body = each_block.getBlockBodyJson();

        Application::app().getCustomLedgerManager().procLedgerBlock(block_body);

        m_storage->saveBlock(block_raw, block_header, block_body);
        CLOG(INFO, "BPRO") << "BLOCK SAVED (height=" << each_block.getHeight()
                           << ",#tx=" << each_block.getNumTransactions()
                           << ",#ssig=" << each_block.getNumSSigs() << ")";
      }
    }

    block_height_type unresolved_height =
        m_unresolved_block_pool.getUnresolvedLowestHeight();
    if (unresolved_height > 0) {
      OutputMsgEntry msg_req_block;

      msg_req_block.type = MessageType::MSG_REQ_BLOCK;
      msg_req_block.body["mID"] = TypeConverter::encodeBase64(m_my_id); // my_id
      msg_req_block.body["time"] = Time::now();
      msg_req_block.body["mCert"] = "";
      msg_req_block.body["hgt"] = std::to_string(unresolved_height);
      msg_req_block.body["prevHash"] = "";
      msg_req_block.body["mSig"] = "";
      msg_req_block.receivers = {};

      CLOG(INFO, "BSYN") << "send MSG_REQ_BLOCK (" << unresolved_height << ")";

      m_msg_proxy.deliverOutputMessage(msg_req_block);
    }
  }));

  m_timer->expires_from_now(
      boost::posix_time::milliseconds(config::BROC_PROCESSOR_TASK_PERIOD));
  m_timer->async_wait([this](const boost::system::error_code &ec) {
    if (ec == boost::asio::error::operation_aborted) {
      CLOG(INFO, "BPRO") << "Timer ABORTED";
    } else if (ec.value() == 0) {
      periodicTask();
    } else {
      CLOG(ERROR, "BPRO") << ec.message();
      // throw;
    }
  });
}

nth_link_type BlockProcessor::getMostPossibleLink() {

  return m_unresolved_block_pool.getMostPossibleLink();
}

bool BlockProcessor::hasUnresolvedBlocks() {
  return m_unresolved_block_pool.hasUnresolvedBlocks();
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

  block_height_type req_block_height = Safe::getInt(entry.body, "hgt");
  std::string req_prev_hash = Safe::getString(entry.body, "prevHash");

  if (Safe::getString(entry.body, "mCert").empty() ||
      Safe::getString(entry.body, "mSig").empty()) {

    // TODO : check whether the requester is trustworthy or not

  } else {
    BytesBuilder msg_builder;
    msg_builder.appendB64(Safe::getString(entry.body, "mID"));
    msg_builder.appendDec(Safe::getString(entry.body, "time"));
    msg_builder.append(Safe::getString(entry.body, "mCert"));
    msg_builder.append(req_block_height);
    msg_builder.appendB64(req_prev_hash);

    if (!ECDSA::doVerify(Safe::getString(entry.body, "mCert"),
                         msg_builder.getBytes(),
                         Safe::getBytesFromB64(entry.body, "mSig"))) {

      CLOG(ERROR, "BPRO") << "Invalid mSig on MSG_REQ_BLOCK";
      return false;
    }
  }

  id_type recv_id = Safe::getBytesFromB64<id_type>(entry.body, "mID");

  if(m_unresolved_block_pool.empty() && m_storage->empty()) {
    sendErrorMessage(ErrorMsgType::BSYNC_NO_BLOCK,recv_id);
    return false;
  }

  bool found_block = false;
  Block ret_block;
  if(m_unresolved_block_pool.getBlock(req_block_height,req_prev_hash,ret_block)){
    found_block = true;
  }

  if(!found_block) { // no block in unresolved block pool, then try storage
    storage_block_type saved_block = m_storage->readBlock(req_block_height);
    if (req_prev_hash.empty() || saved_block.prev_hash_b64 == req_prev_hash) {
      found_block = true;
      ret_block.initialize(saved_block);
    }
  }

  if (!found_block) {
    sendErrorMessage(ErrorMsgType::NO_SUCH_BLOCK,recv_id);
    return false;
  }

  OutputMsgEntry msg_block;
  msg_block.type = MessageType::MSG_BLOCK;
  msg_block.body["mID"] = TypeConverter::encodeBase64(m_my_id);
  msg_block.body["blockraw"] = TypeConverter::encodeBase64(ret_block.getBlockRaw());
  msg_block.body["tx"] = ret_block.getBlockBodyJson()["tx"];
  msg_block.receivers = {recv_id};

  CLOG(INFO, "BPRO") << "Send MSG_BLOCK (height=" << ret_block.getHeight() << ",#tx=" << ret_block.getNumTransactions() << ")";

  m_msg_proxy.deliverOutputMessage(msg_block);

  return true;
}

bool BlockProcessor::handleMsgBlock(InputMsgEntry &entry) {

  Block recv_block;
  if (!recv_block.initialize(entry.body)) {
    CLOG(ERROR, "BPRO") << "Block dropped (missing information)";
    return false;
  }

  if (!recv_block.isValidEarly()) {
    CLOG(ERROR, "BPRO") << "Block dropped (invalid - early stage validation)";
    return false;
  }

  if (!m_unresolved_block_pool.push(recv_block)) {
    CLOG(ERROR, "BPRO") << "Block dropped (unlinkable)";
  }

  Application::app().getTransactionPool().removeDuplicatedTransactions(
      recv_block.getTxIds());

  return true;
}

bool BlockProcessor::handleMsgReqCheck(InputMsgEntry &entry) {

  auto setting = Setting::getInstance();

  proof_type proof = m_storage->getProof(Safe::getString(entry.body, "txid"));

  json proof_json = json::array();
  for (auto &sibling : proof.siblings) {
    proof_json.push_back(
        json{{"side", sibling.first}, {"val", sibling.second}});
  }

  OutputMsgEntry msg_res_check;
  msg_res_check.type = MessageType::MSG_RES_CHECK;
  msg_res_check.body["mID"] = TypeConverter::encodeBase64(setting->getMyId());
  msg_res_check.body["time"] = to_string(Time::now_int());
  msg_res_check.body["blockID"] = proof.block_id_b64;
  msg_res_check.body["proof"] = proof_json;

  CLOG(INFO, "BPRO") << "Send MSG_RES_CHECK";

  m_msg_proxy.deliverOutputMessage(msg_res_check);

  return true;
}

void BlockProcessor::sendErrorMessage(ErrorMsgType t_error_type, id_type &recv_id) {

  OutputMsgEntry output_message;
  output_message.type = MessageType::MSG_ERROR;
  output_message.body["sender"] = TypeConverter::encodeBase64(m_my_id); // my_id
  output_message.body["time"] = Time::now();
  output_message.body["type"] = std::to_string(static_cast<int>(t_error_type));
  output_message.body["info"] = "no block!";
  output_message.receivers = {recv_id};

  CLOG(INFO, "BPRO") << "Send MSG_ERROR (" << (int) t_error_type << ")";

  m_msg_proxy.deliverOutputMessage(output_message);
}
} // namespace gruut