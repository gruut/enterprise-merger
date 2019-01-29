#include "block_synchronizer.hpp"
#include "../../application.hpp"
#include "../../services/message_proxy.hpp"

#include "easy_logging.hpp"

namespace gruut {

BlockSynchronizer::BlockSynchronizer() {

  auto &io_service = Application::app().getIoService();

  m_msg_fetch_scheduler.setIoService(io_service);
  m_sync_control_scheduler.setIoService(io_service);
  m_sync_begin_scheduler.setIoService(io_service);

  m_inputQueue = InputQueueAlt::getInstance();
  auto setting = Setting::getInstance();

  m_my_id = setting->getMyId();

  el::Loggers::getLogger("BSYN");
}

void BlockSynchronizer::syncFinish(ExitCode exit_code) {

  m_is_sync_done = true;

  std::call_once(m_sync_finish_call_flag, [this, &exit_code]() {
    m_msg_fetch_scheduler.stopTask();
    m_sync_control_scheduler.stopTask();

    m_sync_finish_callback(exit_code);
    m_is_sync_done = true;

    CLOG(INFO, "BSYN") << "BLOCK SYNCHRONIZATION ---- END";
  });
}

void BlockSynchronizer::blockSyncControl() {
  if (m_is_sync_done || !m_is_sync_begin) {
    return;
  }

  bool is_done = true;
  for (auto each_flag : m_sync_flags) {
    if (each_flag == false) {
      is_done = false;
      break;
    }
  }

  if (is_done)
    syncFinish(ExitCode::NORMAL);
}

void BlockSynchronizer::sendErrorToSigner(InputMsgEntry &input_msg_entry) {

  signer_id_type signer_id =
      Safe::getBytesFromB64<signer_id_type>(input_msg_entry.body, "sID");
  if (signer_id.empty())
    return;

  OutputMsgEntry output_msg;
  output_msg.type = MessageType::MSG_ERROR;
  output_msg.body["sender"] = TypeConverter::encodeBase64(m_my_id); // my_id
  output_msg.body["time"] = Time::now();
  output_msg.body["type"] =
      std::to_string(static_cast<int>(ErrorMsgType::MERGER_BOOTSTRAP));
  output_msg.body["info"] = "Merger is in bootstrapping. Please, wait.";
  output_msg.receivers = {signer_id};

  CLOG(INFO, "BSYN") << "send MSG_ERROR";

  m_msg_proxy.deliverOutputMessage(output_msg);
}

void BlockSynchronizer::messageFetch() {
  if (m_is_sync_done || m_inputQueue->empty())
    return;

  InputMsgEntry input_msg_entry = m_inputQueue->fetch();

  if (input_msg_entry.type == MessageType::MSG_NULL)
    return;

  CLOG(INFO, "BSYN") << "MSG IN: 0x" << std::hex << (int)input_msg_entry.type;

  if (checkMsgFromSigner(input_msg_entry.type)) {
    sendErrorToSigner(input_msg_entry);
  }

  switch (input_msg_entry.type) {
  case MessageType::MSG_ERROR: {
    if (Safe::getString(input_msg_entry.body, "type") ==
        std::to_string(
            static_cast<int>(ErrorMsgType::BSYNC_NO_BLOCK))) { // Oh! No block!
      syncFinish(ExitCode::NORMAL);
    }
  } break;

  case MessageType::MSG_BLOCK: {
    auto pushed_block_height =
        Application::app().getBlockProcessor().handleMsgBlock(input_msg_entry);
    if (pushed_block_height > 0) {

      std::lock_guard<std::mutex> guard(m_sync_flags_mutex);

      if (pushed_block_height > m_link_from.height) {
        size_t req_map_size = pushed_block_height - m_link_from.height;
        if (m_sync_flags.size() < req_map_size)
          m_sync_flags.resize(req_map_size, false);

        m_sync_flags[req_map_size - 1] = true; // last or this
      }

      m_sync_flags_mutex.unlock();
    }
  } break;

  case MessageType::MSG_RES_STATUS: {
    collectingStatus(input_msg_entry);
  } break;

  case MessageType::MSG_REQ_STATUS: {
    Application::app().getBlockProcessor().handleMessage(input_msg_entry);
  } break;

  default:
    break;
  }
}

void BlockSynchronizer::collectingStatus(InputMsgEntry &entry) {
  if (m_is_sync_begin)
    return;

  std::lock_guard<std::mutex> guard(m_chain_mutex);

  m_chain_status.insert(
      std::make_pair(Safe::getString(entry.body, "mID"),
                     OtherStatusData(Safe::getString(entry.body, "hash"),
                                     Safe::getSize(entry.body, "hgt"))));

  m_chain_mutex.unlock();
}

void BlockSynchronizer::startBlockSync(std::function<void(ExitCode)> callback) {

  m_sync_finish_callback = std::move(callback);

  m_msg_fetch_scheduler.setInterval(config::INQUEUE_MSG_FETCHER_INTERVAL);
  m_msg_fetch_scheduler.setTaskFunction([this]() { messageFetch(); });
  m_msg_fetch_scheduler.runTask();

  m_sync_control_scheduler.setInterval(config::SYNC_CONTROL_INTERVAL);
  m_sync_control_scheduler.setTaskFunction([this]() { blockSyncControl(); });
  m_sync_control_scheduler.runTask();

  CLOG(INFO, "BSYN") << "BLOCK SYNCHRONIZATION ---- START";

  m_link_from = Application::app().getBlockProcessor().getMostPossibleLink();

  sendRequestStatus();

  m_sync_begin_scheduler.runTask(config::STATUS_COLLECTING_TIMEOUT,
                                 [this]() { sendRequestLastBlock(); });
}

void BlockSynchronizer::sendRequestLastBlock() {

  block_height_type last_block_height = 0;
  std::string last_block_hash_b64;
  std::string t_merger_id_b64;

  for (auto &status_dat : m_chain_status) {
    if (status_dat.second.height > last_block_height) {
      last_block_height = status_dat.second.height;
      last_block_hash_b64 = status_dat.second.hash_b64;
      t_merger_id_b64 = status_dat.first;
    }

    if (status_dat.second.height == last_block_height) {
      if (last_block_hash_b64 != status_dat.second.hash_b64) {
        CLOG(INFO, "ERROR")
            << "Chain fork was detected! [" << t_merger_id_b64 << "] and ["
            << status_dat.first << "] have different blocks.";
      }
    }
  }

  if (last_block_height == 0) { // no response at all
    syncFinish(ExitCode::ERROR_SYNC_ALONE);
    return;
  }

  if (last_block_height < m_link_from.height ||
      (m_link_from.height == last_block_height &&
       TypeConverter::encodeBase64(m_link_from.hash) ==
           last_block_hash_b64)) { // no need to sync
    syncFinish(ExitCode::NORMAL);
    return;
  }

  if (m_link_from.height == last_block_height) {
    sendRequestBlock(last_block_height, last_block_hash_b64,
                     TypeConverter::decodeBase64(t_merger_id_b64));
    syncFinish(ExitCode::NORMAL);
    return;
  }

  if (last_block_height > m_link_from.height) {
    sendRequestBlock(last_block_height, last_block_hash_b64,
                     TypeConverter::decodeBase64(t_merger_id_b64));

    std::lock_guard<std::mutex> guard(m_sync_flags_mutex);

    size_t req_map_size = last_block_height - m_link_from.height;
    if (m_sync_flags.size() < req_map_size)
      m_sync_flags.resize(req_map_size, false);

    m_sync_flags_mutex.unlock();
  }

  m_is_sync_begin = true;
}

void BlockSynchronizer::sendRequestBlock(size_t height,
                                         const std::string &block_hash_b64,
                                         const merger_id_type &t_merger) {

  OutputMsgEntry msg_req_block;
  msg_req_block.type = MessageType::MSG_REQ_BLOCK;
  msg_req_block.body["mID"] = TypeConverter::encodeBase64(m_my_id); // my_id
  msg_req_block.body["time"] = Time::now();
  msg_req_block.body["mCert"] = "";
  msg_req_block.body["hgt"] = std::to_string(height);
  msg_req_block.body["prevHash"] = "";
  msg_req_block.body["hash"] = block_hash_b64;
  msg_req_block.body["mSig"] = "";
  msg_req_block.receivers = {t_merger};

  CLOG(INFO, "BSYN") << "send MSG_REQ_BLOCK (height=" << height
                     << ",hash=" << block_hash_b64 << ")";

  m_msg_proxy.deliverOutputMessage(msg_req_block);
}

void BlockSynchronizer::sendRequestStatus() {

  OutputMsgEntry msg_req_status;
  msg_req_status.type = MessageType::MSG_REQ_STATUS;
  msg_req_status.body["mID"] = TypeConverter::encodeBase64(m_my_id); // my_id
  msg_req_status.body["time"] = Time::now();
  msg_req_status.body["mCert"] = "";
  msg_req_status.body["hgt"] = m_link_from.height;
  msg_req_status.body["hash"] = TypeConverter::encodeBase64(m_link_from.hash);
  msg_req_status.body["mSig"] = "";
  msg_req_status.receivers = {};

  CLOG(INFO, "BSYN") << "send MSG_REQ_STATUS";

  m_msg_proxy.deliverOutputMessage(msg_req_status);
}

inline bool BlockSynchronizer::checkMsgFromSigner(MessageType msg_type) {
  return (msg_type == MessageType::MSG_JOIN ||
          msg_type == MessageType::MSG_RESPONSE_1 ||
          msg_type == MessageType::MSG_LEAVE);
}
}; // namespace gruut