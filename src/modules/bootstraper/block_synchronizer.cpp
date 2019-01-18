#include "block_synchronizer.hpp"
#include "../../application.hpp"
#include "../../services/message_proxy.hpp"

#include "easy_logging.hpp"

namespace gruut {

BlockSynchronizer::BlockSynchronizer() {

  auto &io_service = Application::app().getIoService();

  m_block_sync_strand.reset(new boost::asio::io_service::strand(io_service));
  m_msg_fetching_timer.reset(new boost::asio::deadline_timer(io_service));
  m_sync_ctrl_timer.reset(new boost::asio::deadline_timer(io_service));

  m_inputQueue = InputQueueAlt::getInstance();
  auto setting = Setting::getInstance();

  m_my_id = setting->getMyId();

  el::Loggers::getLogger("BSYN");
}

void BlockSynchronizer::syncFinish() {

  std::call_once(m_end_sync_call_flag, [this]() {
    m_msg_fetching_timer->cancel();
    m_sync_ctrl_timer->cancel();

    if (m_sync_fail) {
      if (m_sync_alone)
        m_finish_callback(ExitCode::ERROR_SYNC_ALONE);
      else
        m_finish_callback(ExitCode::ERROR_SYNC_FAIL);
    } else
      m_finish_callback(ExitCode::NORMAL);

    CLOG(INFO, "BSYN") << "BLOCK SYNCHRONIZATION ---- END";
  });

}

void BlockSynchronizer::blockSyncControl() {

  if (m_sync_done) {
    return;
  }

  auto &io_service = Application::app().getIoService();
  io_service.post(m_block_sync_strand->wrap([this]() {



  }));

  m_sync_ctrl_timer->expires_from_now(
      boost::posix_time::milliseconds(config::SYNC_CONTROL_INTERVAL));
  m_sync_ctrl_timer->async_wait([this](const boost::system::error_code &ec) {
    if (ec == boost::asio::error::operation_aborted) {
      CLOG(INFO, "BSYN") << "CtrlTimer ABORTED";
    } else if (ec.value() == 0) {
      blockSyncControl();
    } else {
      CLOG(ERROR, "BSYN") << ec.message();
    }
  });
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
  if (m_sync_done)
    return;

  auto &io_service = Application::app().getIoService();
  io_service.post([this]() {
    if (m_sync_done)
      return;

    if (m_inputQueue->empty())
      return;

    InputMsgEntry input_msg_entry = m_inputQueue->fetch();

    if (input_msg_entry.type == MessageType::MSG_NULL)
      return;

    CLOG(INFO, "BSYN") << "MSG IN: 0x" << std::hex << (int) input_msg_entry.type;

    if (checkMsgFromOtherMerger(input_msg_entry.type)) {
      m_sync_alone = false; // Wow! I am not alone!
    } else if (checkMsgFromSigner(input_msg_entry.type)) {
      sendErrorToSigner(input_msg_entry);
    }

    switch (input_msg_entry.type) {
    case MessageType::MSG_ERROR : {
      if (Safe::getString(input_msg_entry.body, "type")
        == std::to_string(static_cast<int>(ErrorMsgType::BSYNC_NO_BLOCK))) { // Oh! No block!
        m_sync_done = true;
        m_sync_alone = false;
        m_sync_fail = false;
        syncFinish();
      }
    }
      break;

    case MessageType::MSG_BLOCK : {
      if (Application::app().getBlockProcessor().handleMessage(input_msg_entry)) {

        if (pushed_block_height > m_link_from.height) {
          size_t req_map_size = pushed_block_height - m_link_from.height;
          if (m_sync_map.size() < req_map_size)
            m_sync_map.resize(req_map_size, false);

          m_sync_map[req_map_size - 1] = true;
        }

      }

    }
      break;

    case MessageType::MSG_RES_STATUS : {

    }
      break;

    case MessageType::MSG_REQ_STATUS : {
      Application::app().getBlockProcessor().handleMessage(input_msg_entry);
    }
      break;

    default:break;
    }
  });

  m_msg_fetching_timer->expires_from_now(
      boost::posix_time::milliseconds(config::INQUEUE_MSG_FETCHER_INTERVAL));
  m_msg_fetching_timer->async_wait([this](const boost::system::error_code &ec) {
    if (ec == boost::asio::error::operation_aborted) {
      CLOG(INFO, "BSYN") << "FetchingTimer ABORTED";
    } else if (ec.value() == 0) {
      messageFetch();
    } else {
      CLOG(ERROR, "BSYN") << ec.message();
      // throw;
    }
  });
}

void BlockSynchronizer::startBlockSync(std::function<void(ExitCode)> callback) {

  m_finish_callback = std::move(callback);

  CLOG(INFO, "BSYN") << "BLOCK SYNCHRONIZATION ---- START";

  m_link_from = Application::app().getBlockProcessor().getMostPossibleLink();

  sendRequestStatus();
  messageFetch();
  blockSyncControl();
}

void BlockSynchronizer::sendRequestBlock(size_t height){

  OutputMsgEntry msg_req_block;
  msg_req_block.type = MessageType::MSG_REQ_BLOCK;
  msg_req_block.body["mID"] = TypeConverter::encodeBase64(m_my_id); // my_id
  msg_req_block.body["time"] = Time::now();
  msg_req_block.body["mCert"] = "";
  msg_req_block.body["hgt"] = std::to_string(height);
  msg_req_block.body["prevHash"] = "";
  msg_req_block.body["mSig"] = "";
  msg_req_block.receivers = {};

  CLOG(INFO, "BSYN") << "send MSG_REQ_BLOCK (" << height << ")";

  m_msg_proxy.deliverOutputMessage(msg_req_block);
}

void BlockSynchronizer::sendRequestStatus(){

  OutputMsgEntry msg_req_status;
  msg_req_status.type = MessageType::MSG_REQ_STATUS ;
  msg_req_status.body["mID"] = TypeConverter::encodeBase64(m_my_id); // my_id
  msg_req_status.body["time"] = Time::now();
  msg_req_status.body["mCert"] = "";
  msg_req_status.body["hgt"] = m_link_from.height;
  msg_req_status.body["hash"] = m_link_from.hash_b64;
  msg_req_status.body["mSig"] = "";
  msg_req_status.receivers = {};

  CLOG(INFO, "BSYN") << "send MSG_REQ_STATUS";

  m_msg_proxy.deliverOutputMessage(msg_req_status);
}

inline bool BlockSynchronizer::checkMsgFromOtherMerger(MessageType msg_type) {
  return (
      msg_type == MessageType::MSG_UP || msg_type == MessageType::MSG_PING ||
      msg_type == MessageType::MSG_REQ_BLOCK ||
      msg_type == MessageType::MSG_BLOCK || msg_type == MessageType::MSG_ERROR);
}

inline bool BlockSynchronizer::checkMsgFromSigner(MessageType msg_type) {
  return (msg_type == MessageType::MSG_JOIN ||
          msg_type == MessageType::MSG_RESPONSE_1 ||
          msg_type == MessageType::MSG_LEAVE);
}
}; // namespace gruut