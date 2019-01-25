#include "block_processor.hpp"
#include "../../application.hpp"
#include "easy_logging.hpp"

namespace gruut {

BlockProcessor::BlockProcessor() {
  m_storage = Storage::getInstance();
  m_layered_storage = LayeredStorage::getInstance();

  auto setting = Setting::getInstance();
  m_my_id_b64 = TypeConverter::encodeBase64(setting->getMyId());

  auto last_block_info = m_storage->getNthBlockLinkInfo();

  m_unresolved_block_pool.setPool(last_block_info.id, last_block_info.hash,
                                  last_block_info.height, last_block_info.time);

  m_unresolved_block_pool.restorePool();

  auto &io_service = Application::app().getIoService();
  m_task_scheduler.setIoService(io_service);

  el::Loggers::getLogger("BPRO");
}

void BlockProcessor::start() {
  m_task_scheduler.setTaskFunction([this]() { periodicTask(); });
  m_task_scheduler.setInterval(config::BROC_PROCESSOR_TASK_INTERVAL);
  m_task_scheduler.setStrandMod();
  m_task_scheduler.runTask();
}

void BlockProcessor::periodicTask() {
  timestamp_t current_time = Time::now_int();
  std::vector<BlockRequestRecord> request_this_time;

  std::lock_guard<std::recursive_mutex> guard(m_request_mutex);

  for (auto &each_request : m_request_list) {
    if (current_time > each_request.request_time &&
        current_time >
            config::BROC_PROCESSOR_REQ_WAIT + each_request.request_time) {
      request_this_time.emplace_back(each_request);
      each_request.request_time = current_time;
    }
  }

  m_request_mutex.unlock();

  for (auto &each_request : request_this_time) {

    OutputMsgEntry msg_req_block;

    msg_req_block.type = MessageType::MSG_REQ_BLOCK;
    msg_req_block.body["mID"] = m_my_id_b64; // my_id
    msg_req_block.body["time"] = to_string(current_time);
    msg_req_block.body["mCert"] = "";
    msg_req_block.body["hgt"] = to_string(each_request.height);
    msg_req_block.body["prevHash"] = each_request.prev_hash_b64;
    msg_req_block.body["hash"] = each_request.hash_b64;
    msg_req_block.body["mSig"] = "";

    if (each_request.recv_id.empty())
      msg_req_block.receivers = {};
    else
      msg_req_block.receivers = {each_request.recv_id};

    CLOG(INFO, "BPRO") << "send MSG_REQ_BLOCK (height=" << each_request.height
                       << ",prevHash=" << each_request.prev_hash_b64 << ")";

    m_msg_proxy.deliverOutputMessage(msg_req_block);
  }
}

block_layer_t BlockProcessor::getBlockLayer(const std::string &block_id_b64){
 return m_unresolved_block_pool.getBlockLayer(block_id_b64);
}

nth_link_type BlockProcessor::getMostPossibleLink() {
  return m_unresolved_block_pool.getMostPossibleLink();
}

bool BlockProcessor::hasUnresolvedBlocks() {
  return m_unresolved_block_pool.hasUnresolvedBlocks();
}

void BlockProcessor::handleMessage(InputMsgEntry &entry) {
  switch (entry.type) {
  case MessageType::MSG_REQ_BLOCK:
    handleMsgReqBlock(entry);
    break;
  case MessageType::MSG_BLOCK:
    handleMsgBlock(entry);
    break;
  case MessageType::MSG_REQ_CHECK:
    handleMsgReqCheck(entry);
    break;
  case MessageType::MSG_REQ_STATUS:
    handleMsgReqStatus(entry);
    break;
  default:
    break;
  }
}

void BlockProcessor::handleMsgReqStatus(InputMsgEntry &entry) {

  id_type sender_id = Safe::getBytesFromB64<id_type>(entry.body, "mID");

  if (m_unresolved_block_pool.empty() && m_storage->empty()) {
    sendErrorMessage(ErrorMsgType::BSYNC_NO_BLOCK, sender_id);
    return;
  }

  auto possible_link = getMostPossibleLink();

  OutputMsgEntry msg_res_status;
  msg_res_status.type = MessageType::MSG_RES_STATUS;
  msg_res_status.body["mID"] = m_my_id_b64;
  msg_res_status.body["time"] = Time::now();
  msg_res_status.body["mCert"] = "";
  msg_res_status.body["hgt"] = to_string(possible_link.height);
  msg_res_status.body["hash"] = TypeConverter::encodeBase64(possible_link.hash);
  msg_res_status.body["mSig"] = "";
  msg_res_status.receivers = {sender_id};

  CLOG(INFO, "BPRO") << "Send MSG_RES_STATUS (height=" << possible_link.height
                     << ")";

  m_msg_proxy.deliverOutputMessage(msg_res_status);
}

void BlockProcessor::handleMsgReqBlock(InputMsgEntry &entry) {

  block_height_type req_block_height = Safe::getInt(entry.body, "hgt");
  hash_t req_prev_hash = Safe::getBytesFromB64<hash_t>(entry.body, "prevHash");
  hash_t req_hash = Safe::getBytesFromB64<hash_t>(entry.body, "hash");

  // TODO : check whether the requester is trustworthy or not

  id_type sender_id = Safe::getBytesFromB64<id_type>(entry.body, "mID");

  if (m_unresolved_block_pool.empty() && m_storage->empty()) {
    sendErrorMessage(ErrorMsgType::BSYNC_NO_BLOCK, sender_id);
    return;
  }

  bool found_block = false;
  Block ret_block;
  if (m_unresolved_block_pool.getBlock(req_block_height, req_prev_hash,
                                       req_hash, ret_block)) {
    found_block = true;
  }

  if (!found_block) { // no block in unresolved block pool, then try storage
    storage_block_type saved_block = m_storage->readBlock(req_block_height);
    if (saved_block.height > 0 &&
        (req_prev_hash.empty() || saved_block.prev_hash == req_prev_hash) &&
        (req_hash.empty() || saved_block.hash == req_hash)) {
      found_block = true;
      ret_block.initialize(saved_block);
    }

    if (!found_block) {
      CLOG(ERROR, "BPRO") << "No such block (height=" << req_block_height
                          << ",hash="
                          << TypeConverter::encodeBase64(saved_block.hash)
                          << ",prevhash="
                          << TypeConverter::encodeBase64(saved_block.prev_hash)
                          << ")";
      sendErrorMessage(ErrorMsgType::NO_SUCH_BLOCK, sender_id);
      return;
    }
  }

  OutputMsgEntry msg_block;
  msg_block.type = MessageType::MSG_BLOCK;
  msg_block.body["mID"] = m_my_id_b64;
  msg_block.body["blockraw"] =
      TypeConverter::encodeBase64(ret_block.getBlockRaw());
  msg_block.body["tx"] = ret_block.getBlockBodyJson()["tx"];
  msg_block.receivers = {sender_id};

  CLOG(INFO, "BPRO") << "Send MSG_BLOCK (height=" << ret_block.getHeight()
                     << ",#tx=" << ret_block.getNumTransactions() << ")";

  m_msg_proxy.deliverOutputMessage(msg_block);
}

block_height_type BlockProcessor::handleMsgBlock(InputMsgEntry &entry) {

  Block recv_block;
  if (!recv_block.initialize(entry.body)) {
    CLOG(ERROR, "BPRO") << "Block dropped (missing information)";
    return 0;
  }

  if (!recv_block.isValidEarly()) {
    CLOG(ERROR, "BPRO") << "Block dropped (invalid - early stage validation)";
    return 0;
  }

  auto block_push_result = m_unresolved_block_pool.push(recv_block);

  if (block_push_result.height == 0) {
    CLOG(ERROR, "BPRO") << "Block dropped (unlinkable or duplicated)";
    return 0;
  }

  std::lock_guard<std::recursive_mutex> guard(m_request_mutex);

  auto it = m_request_list.begin();
  while (it != m_request_list.end()) {
    if (it->height == recv_block.getHeight() &&
        (it->hash_b64.empty() || it->hash_b64 == recv_block.getHashB64()) &&
        (it->prev_hash_b64.empty() ||
         it->prev_hash_b64 == recv_block.getPrevHashB64())) {
      m_request_list.erase(it++);
    } else {
      it++;
    }
  }

  m_request_mutex.unlock();

  // if not linked initially, we cannot interpret this block because of previous
  // missing blocks!
  if (block_push_result.linked) {
    auto possible_link = getMostPossibleLink();

    Application::app().getCustomLedgerManager().procLedgerBlock(
        entry.body["tx"], recv_block.getBlockIdB64(), block_push_result.block_layer);

    invalidateBlockLayer();

    OutputMsgEntry msg_chain_info;
    msg_chain_info.type = MessageType::MSG_CHAIN_INFO; // MSG_CHAIN_INFO = 0xB7
    msg_chain_info.body["mID"] = Safe::getString(entry.body, "mID");
    msg_chain_info.body["time"] = to_string(possible_link.time);
    msg_chain_info.body["hgt"] = to_string(possible_link.height);
    msg_chain_info.body["bID"] = TypeConverter::encodeBase64(possible_link.id);
    msg_chain_info.body["prevbID"] =
        TypeConverter::encodeBase64(possible_link.prev_id);
    msg_chain_info.body["hash"] =
        TypeConverter::encodeBase64(possible_link.hash);
    msg_chain_info.body["prevHash"] =
        TypeConverter::encodeBase64(possible_link.prev_hash);
    // TODO : mSig 는 임시.
    msg_chain_info.body["mSig"] = "";

    m_msg_proxy.deliverOutputMessage(msg_chain_info);
  }

  Application::app().getTransactionPool().removeDuplicatedTransactions(
      recv_block.getTxIds());

  resolveBlocks();

  return block_push_result.height;
}

void BlockProcessor::invalidateBlockLayer(){
  LayeredStorage::getInstance()->setBlockLayer(m_unresolved_block_pool.getMostPossibleBlockLayer());
}

void BlockProcessor::resolveBlocks() {
  std::vector<UnresolvedBlock> resolved_blocks;
  std::vector<std::string> drop_blocks;

  m_unresolved_block_pool.getResolvedBlocks(resolved_blocks, drop_blocks);

  for (auto &block_id_b64 : drop_blocks)
    m_layered_storage->dropLedger(block_id_b64);

  if (!resolved_blocks.empty()) {
    CLOG(INFO, "BPRO") << "Resolved block(s) received ("
                       << resolved_blocks.size() << ")";

    for (auto &each_block : resolved_blocks) {

      if (!each_block.block.isValidLate()) {
        CLOG(ERROR, "BPRO")
            << "Block dropped (invalid - late stage validation)";
        continue;
      }

      json block_header = each_block.block.getBlockHeaderJson();
      bytes block_raw = each_block.block.getBlockRaw();
      json block_body = each_block.block.getBlockBodyJson();

      if (each_block.linked) {
        // this block was not interpreted into the ledger because of link failure
        Application::app().getCustomLedgerManager().procLedgerBlock(
            block_body["tx"], each_block.block.getBlockIdB64(), each_block.block_layer);

        invalidateBlockLayer();
      }

      m_storage->saveBlock(block_raw, block_header, block_body);
      m_layered_storage->moveToDiskLedger(each_block.block.getBlockIdB64());

      CLOG(INFO, "BPRO") << "BLOCK SAVED (height="
                         << each_block.block.getHeight()
                         << ",#tx=" << each_block.block.getNumTransactions()
                         << ",#ssig=" << each_block.block.getNumSSigs() << ")";
    }
  }

  nth_link_type unresolved_block =
      m_unresolved_block_pool.getUnresolvedLowestLink();
  if (unresolved_block.height > 0) {

    // TODO : use tracker's information, broadcast can cause a lot of block
    // droppings

    BlockRequestRecord new_request;
    new_request.height = unresolved_block.height;
    new_request.recv_id = id_type();
    new_request.hash_b64 = "";
    new_request.prev_hash_b64 =
        TypeConverter::encodeBase64(unresolved_block.prev_hash);
    new_request.request_time = 0;

    std::lock_guard<std::recursive_mutex> guard(m_request_mutex);
    m_request_list.emplace_back(new_request);
    m_request_mutex.unlock();
  }
}

void BlockProcessor::handleMsgReqCheck(InputMsgEntry &entry) {

  proof_type proof = m_storage->getProof(Safe::getString(entry.body, "txid"));

  json proof_json = json::array();
  for (auto &sibling : proof.siblings) {
    proof_json.push_back(
        json{{"side", sibling.first}, {"val", sibling.second}});
  }

  OutputMsgEntry msg_res_check;
  msg_res_check.type = MessageType::MSG_RES_CHECK;
  msg_res_check.body["mID"] = m_my_id_b64;
  msg_res_check.body["time"] = to_string(Time::now_int());
  msg_res_check.body["blockID"] = proof.block_id_b64;
  msg_res_check.body["proof"] = proof_json;

  CLOG(INFO, "BPRO") << "Send MSG_RES_CHECK";

  m_msg_proxy.deliverOutputMessage(msg_res_check);
}

void BlockProcessor::sendErrorMessage(ErrorMsgType t_error_type,
                                      id_type &recv_id) {

  OutputMsgEntry output_message;
  output_message.type = MessageType::MSG_ERROR;
  output_message.body["sender"] = m_my_id_b64; // my_id
  output_message.body["time"] = Time::now();
  output_message.body["type"] = std::to_string(static_cast<int>(t_error_type));
  output_message.body["info"] = "no block!";
  output_message.receivers = {recv_id};

  CLOG(INFO, "BPRO") << "Send MSG_ERROR (" << (int)t_error_type << ")";

  m_msg_proxy.deliverOutputMessage(output_message);
}
} // namespace gruut