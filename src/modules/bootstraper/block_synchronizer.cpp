#include "block_synchronizer.hpp"
#include "../../application.hpp"
#include "../../services/message_proxy.hpp"

#include "easy_logging.hpp"

namespace gruut {

BlockSynchronizer::BlockSynchronizer() {

  auto &io_service = Application::app().getIoService();

  m_msg_fetching_loop_timer.reset(new boost::asio::deadline_timer(io_service));
  m_sync_ctrl_loop_timer.reset(new boost::asio::deadline_timer(io_service));
  m_sync_begin_timer.reset(new boost::asio::deadline_timer(io_service));

  m_inputQueue = InputQueueAlt::getInstance();
  auto setting = Setting::getInstance();

  m_my_id = setting->getMyId();

  el::Loggers::getLogger("BSYN");
}

void BlockSynchronizer::syncFinish(ExitCode exit_code) {

  m_is_sync_done = true;

  std::call_once(m_sync_finish_call_flag, [this, &exit_code]() {
    m_msg_fetching_loop_timer->cancel();
    m_sync_ctrl_loop_timer->cancel();

    m_sync_finish_callback(exit_code);
    m_is_sync_done = true;

    CLOG(INFO, "BSYN") << "BLOCK SYNCHRONIZATION ---- END";
  });
}

void BlockSynchronizer::blockSyncControl() {

  if (m_is_sync_done) {
    return;
  }

  auto &io_service = Application::app().getIoService();
  io_service.post([this]() {
    if (!m_is_sync_begin) {
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
  });

  m_sync_ctrl_loop_timer->expires_from_now(
      boost::posix_time::milliseconds(config::SYNC_CONTROL_INTERVAL));
  m_sync_ctrl_loop_timer->async_wait(
      [this](const boost::system::error_code &error) {
        if (!error) {
          blockSyncControl();
        } else {
          CLOG(INFO, "BSYN") << error.message();
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
  if (m_is_sync_done)
    return;

  auto &io_service = Application::app().getIoService();
  io_service.post([this]() {
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
          std::to_string(static_cast<int>(
              ErrorMsgType::BSYNC_NO_BLOCK))) { // Oh! No block!
        syncFinish(ExitCode::NORMAL);
      }
    } break;

    case MessageType::MSG_BLOCK: {
      auto pushed_block_height =
          Application::app().getBlockProcessor().handleMsgBlock(
              input_msg_entry);
      if (pushed_block_height > 0) {

        if (pushed_block_height > m_link_from.height) {
          size_t req_map_size = pushed_block_height - m_link_from.height;
          if (m_sync_flags.size() < req_map_size)
            m_sync_flags.resize(req_map_size, false);

          m_sync_flags[req_map_size - 1] = true; // last or this
        }
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
  });

  m_msg_fetching_loop_timer->expires_from_now(
      boost::posix_time::milliseconds(config::INQUEUE_MSG_FETCHER_INTERVAL));
  m_msg_fetching_loop_timer->async_wait(
      [this](const boost::system::error_code &error) {
        if (!error) {
          messageFetch();
        } else {
          CLOG(INFO, "BSYN") << error.message();
        }
      });
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

  CLOG(INFO, "BSYN") << "BLOCK SYNCHRONIZATION ---- START";

  // TODO : MAX BLOCK HEIGHT 를 가진 Merger들과 연결 될 수 있을때, loop를 빠져
  // 나간다. 사용자로 부터 입력을 받아 처리 할 수 도 있도록 수정 될 수 있음.
  auto conn_manager = ConnManager::getInstance();
  auto max_hgt_merger_list = conn_manager->getMaxBlockHgtMergers();
  uint64_t max_hgt = 0;

  bool conn_check = false;

  if (max_hgt_merger_list.empty())
    conn_check = true;
  else
    max_hgt = max_hgt_merger_list[0].first;

  bool have_max_hgt = false;
  for (auto &check_id : max_hgt_merger_list) {
    if (m_my_id == check_id.second) {
      have_max_hgt = true;
      break;
    }
  }

  if (max_hgt != 0 && !have_max_hgt)
    CLOG(INFO, "BSYN") << "Waiting Mergers that have max height block";
    while (!conn_check) {
      for (auto &merger : max_hgt_merger_list) {
        conn_check |= conn_manager->getMergerStatus(merger.second);
      }
      // TODO: 500ms 임시값.
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

  m_link_from = Application::app().getBlockProcessor().getMostPossibleLink();

  sendRequestStatus();
  messageFetch();
  blockSyncControl();

  m_sync_begin_timer->expires_from_now(
      boost::posix_time::seconds(config::STATUS_COLLECTING_TIMEOUT_SEC));
  m_sync_begin_timer->async_wait(
      [this](const boost::system::error_code &error) {
        if (!error) {
          sendRequestLastBlock();
        } else {
          CLOG(INFO, "BSYN") << error.message();
        }
      });
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

  sendRequestBlock(last_block_height, last_block_hash_b64,
                   TypeConverter::decodeBase64(t_merger_id_b64));

  if (last_block_height > m_link_from.height) {
    size_t req_map_size = last_block_height - m_link_from.height;
    if (m_sync_flags.size() < req_map_size)
      m_sync_flags.resize(req_map_size, false);
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