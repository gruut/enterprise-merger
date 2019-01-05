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

  m_storage = Storage::getInstance();
  m_inputQueue = InputQueueAlt::getInstance();
  auto setting = Setting::getInstance();

  m_my_id = setting->getMyId();

  el::Loggers::getLogger("BSYN");
}

bool BlockSynchronizer::pushMsgToBlockList(InputMsgEntry &msg_block) {

  updateTaskTime();

  std::string sender_id_b64 = Safe::getString(msg_block.body, "mID");

  Block new_block;
  if (!new_block.initialze(msg_block.body)) {
    CLOG(ERROR, "BSYN") << "Block dropped (missing information)";
    return false;
  }

  size_t block_height = new_block.getHeight();

  auto it_map = m_recv_block_list.find(new_block.getHeight());

  bool is_new_block = false;

  if (it_map == m_recv_block_list.end())
    is_new_block = true;

  if (m_first_recv_block_height == 0) {
    m_first_recv_block_height = new_block.getHeight();
    reserveBlockList(m_my_last_height + 1, m_first_recv_block_height);
  }

  RcvBlockMapItem recv_block_item;
  recv_block_item.merger_id_b64 = sender_id_b64;
  recv_block_item.block = std::move(new_block);
  recv_block_item.state = BlockState::RECEIVED;

  if (is_new_block) {

    std::lock_guard<std::mutex> lock(m_block_list_mutex);
    m_recv_block_list.insert(make_pair(block_height, recv_block_item));
    m_block_list_mutex.unlock();

  } else if (it_map->second.state == BlockState::RETRIED) {

    std::lock_guard<std::mutex> lock(m_block_list_mutex);
    m_recv_block_list.erase(block_height);
    m_recv_block_list.insert(std::make_pair(block_height, recv_block_item));
    m_block_list_mutex.unlock();

  } else {
    CLOG(ERROR, "BSYN") << "Block dropped (unknown)";
    return false;
  }

  CLOG(INFO, "BSYN") << "Block received (height=" << block_height << ")";

  return true;
}

void BlockSynchronizer::updateTaskTime() {
  m_last_task_time = static_cast<timestamp_type>(Time::now_int());
}

void BlockSynchronizer::reserveBlockList(size_t begin, size_t end) {

  RcvBlockMapItem temp;
  temp.state = BlockState::RESERVED;

  std::lock_guard<std::mutex> lock(m_block_list_mutex);

  for (size_t i = begin; i < end; ++i) {
    m_recv_block_list.insert(make_pair(i, temp));
  }

  m_block_list_mutex.unlock();

  CLOG(INFO, "BSYN") << "Block slots reserved (from=" << begin
                     << ",before=" << end << ")";
}

bool BlockSynchronizer::sendBlockRequest(size_t height) {

  std::vector<id_type> receivers = {};

  if (height != 0) { // unicast

    if (m_recv_block_list.empty()) { // but, no body
      return false;
    }

    std::vector<std::string> ans_merger_list;

    for (auto &blk_item : m_recv_block_list) {
      if (!blk_item.second.merger_id_b64.empty())
        ans_merger_list.emplace_back(blk_item.second.merger_id_b64);
    }

    std::string ans_merger_id_b64 =
        ans_merger_list[PRNG::getRange(0, (int)(ans_merger_list.size() - 1))];
    merger_id_type ans_merger_id =
        TypeConverter::decodeBase64(ans_merger_id_b64);
    receivers.emplace_back(ans_merger_id);
  }

  OutputMsgEntry msg_req_block;

  msg_req_block.type = MessageType::MSG_REQ_BLOCK;
  msg_req_block.body["mID"] = TypeConverter::encodeBase64(m_my_id); // my_id
  msg_req_block.body["time"] = Time::now();
  msg_req_block.body["mCert"] = "";
  msg_req_block.body["hgt"] = std::to_string(height);
  msg_req_block.body["mSig"] = "";
  msg_req_block.receivers = receivers;

  CLOG(INFO, "BSYN") << "send MSG_REQ_BLOCK (" << height << ")";

  m_msg_proxy.deliverOutputMessage(msg_req_block);

  updateTaskTime();

  return true;
}

bool BlockSynchronizer::validateBlock(size_t height) {
  auto it_map = m_recv_block_list.find(height);
  if (it_map == m_recv_block_list.end())
    return false;

  return it_map->second.block.isValid();
}

void BlockSynchronizer::saveBlock(size_t height) {

  // CLOG(INFO, "BSYN") << "called saveBlock()";

  auto it_map = m_recv_block_list.find(height);
  if (it_map == m_recv_block_list.end()) {
    return;
  }

  json block_header = it_map->second.block.getBlockHeaderJson();
  bytes block_raw = it_map->second.block.getBlockRaw();
  json block_body = it_map->second.block.getBlockBodyJson();

  m_storage->saveBlock(block_raw, block_header, block_body);

  CLOG(INFO, "BSYN") << "Block saved (height=" << height << ")";
}

void BlockSynchronizer::syncFinish() {

  CLOG(INFO, "BSYN") << "END BLOCK SYNCHRONIZATION "
                        "=================================================";

  m_msg_fetching_timer->cancel();
  m_sync_ctrl_timer->cancel();

  if (m_sync_fail) {
    if (m_sync_alone)
      m_finish_callback(ExitCode::ERROR_SYNC_ALONE);
    else
      m_finish_callback(ExitCode::ERROR_SYNC_FAIL);
  } else
    m_finish_callback(ExitCode::NORMAL);
}

void BlockSynchronizer::blockSyncControl() {

  if (m_sync_done) {
    return;
  }

  auto &io_service = Application::app().getIoService();
  io_service.post(m_block_sync_strand->wrap([this]() {
    // step 0 - check whether this is done
    if (Time::now_int() - m_last_task_time >
        config::BOOTSTRAP_MAX_TASK_WAIT_TIME) {
      m_sync_done = true;
      m_sync_fail = true;

      syncFinish();

      return;
    }

    if (m_recv_block_list.empty()) { // not over, but empty
      return;
    }

    // step 1 - validate min height block
    size_t lowest_block_height = std::numeric_limits<size_t>::max();
    for (auto &block_item : m_recv_block_list) {
      if (lowest_block_height > block_item.first)
        lowest_block_height = block_item.first;
    }

    if (lowest_block_height == m_my_last_height + 1) {
      auto it_map = m_recv_block_list.find(lowest_block_height);
      if (it_map->second.state == BlockState::RECEIVED) {
        if (it_map->second.block.getPrevHashB64() == m_my_last_blk_hash_b64) {
          if (validateBlock(it_map->first)) {

            std::lock_guard<std::mutex> lock(m_block_list_mutex);
            it_map->second.state = BlockState::TOSAVE;
            m_block_list_mutex.unlock();

            m_my_last_blk_hash_b64 = it_map->second.block.getHashB64();
            m_my_last_height = it_map->first;

            updateTaskTime();

          } else {
            CLOG(ERROR, "BSYN") << "Invalid block";
            std::lock_guard<std::mutex> lock(m_block_list_mutex);
            it_map->second.state = BlockState::RETRIED;
            m_block_list_mutex.unlock();
          }
        } else {
          CLOG(ERROR, "BSYN") << "Chain is not match";
        }
      }
    }

    // step 2 - save block
    for (auto &block_item : m_recv_block_list) {
      if (block_item.second.state == BlockState::TOSAVE) {
        saveBlock(block_item.first);
        block_item.second.state = BlockState::TODELETE;
      }
    }

    // step 3 - delete block list
    for (auto it_map = m_recv_block_list.begin();
         it_map != m_recv_block_list.end();) {
      if (it_map->second.state == BlockState::TODELETE) {
        std::lock_guard<std::mutex> lock(m_block_list_mutex);
        m_recv_block_list.erase(it_map++);
        m_block_list_mutex.unlock();
      } else {
        ++it_map;
      }
    }

    // step 4 - retry min block
    bool is_retry_block = false;
    size_t retry_block = std::numeric_limits<size_t>::max();
    for (auto &block_item : m_recv_block_list) {
      if (block_item.second.num_retry > config::BOOTSTRAP_MAX_REQ_BLOCK_RETRY) {
        m_sync_done = true;
        m_sync_fail = true;
        break;
      }

      if (block_item.second.state == BlockState::RETRIED) {
        is_retry_block = true;
        if (retry_block > block_item.first)
          retry_block = block_item.first;
      }
    }

    if (!m_sync_done) {

      if (is_retry_block) {

        auto it_map = m_recv_block_list.find(retry_block);
        std::lock_guard<std::mutex> lock(m_block_list_mutex);
        it_map->second.num_retry += 1;
        m_block_list_mutex.unlock();
        sendBlockRequest(retry_block);

      } else {

        if (!m_recv_block_list.empty()) { // no retry block and not empty list

          bool is_reserve_block = false;
          size_t reserve_block = std::numeric_limits<size_t>::max();
          for (auto &block_item : m_recv_block_list) {
            if (block_item.second.state == BlockState::RESERVED) {
              is_reserve_block = true;
              if (reserve_block > block_item.first)
                reserve_block = block_item.first;
            }
          }

          if (is_reserve_block) {
            auto it_map = m_recv_block_list.find(reserve_block);
            std::lock_guard<std::mutex> lock(m_block_list_mutex);
            it_map->second.num_retry += 1;
            it_map->second.state = BlockState::RETRIED;
            m_block_list_mutex.unlock();
            sendBlockRequest(reserve_block);
          }
        }
      }
    }

    // step 5 - finishing
    if (m_first_recv_block_height <= m_my_last_height &&
        m_recv_block_list.empty()) {
      m_sync_done = true;
    }

    if (m_sync_done) { // ok! block sync was done

      syncFinish();

      return;
    }
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
      // throw;
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

  m_msg_proxy.deliverOutputMessage(output_msg);
}

void BlockSynchronizer::messageFetch() {
  if (m_sync_done)
    return;

  auto &io_service = Application::app().getIoService();
  io_service.post([this]() {
    if (m_inputQueue->empty())
      return;

    InputMsgEntry input_msg_entry = m_inputQueue->fetch();

    if (input_msg_entry.type == MessageType::MSG_NULL)
      return;

    CLOG(INFO, "BSYN") << "MSG IN: " << (int)input_msg_entry.type;

    if (checkMsgFromOtherMerger(input_msg_entry.type)) {
      m_sync_alone = false; // Wow! I am not alone!
    } else if (checkMsgFromSigner(input_msg_entry.type)) {
      sendErrorToSigner(input_msg_entry);
    }

    if (input_msg_entry.type == MessageType::MSG_ERROR &&
        Safe::getString(input_msg_entry.body, "type") ==
            std::to_string(static_cast<int>(
                ErrorMsgType::BSYNC_NO_BLOCK))) { // Oh! No block!
      m_sync_done = true;
      m_sync_alone = false;
      m_sync_fail = false;

      syncFinish();
    } else if (input_msg_entry.type == MessageType::MSG_BLOCK) {
      pushMsgToBlockList(input_msg_entry);
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

  CLOG(INFO, "BSYN") << "START BLOCK SYNCHRONIZATION "
                        "===============================================";

  m_finish_callback = std::move(callback);

  std::pair<std::string, size_t> hash_and_height =
      m_storage->findLatestHashAndHeight();
  m_my_last_height = hash_and_height.second; // if 0, no block in DB

  if (m_my_last_height == 0) {
    m_my_last_blk_hash_b64 = config::GENESIS_BLOCK_PREV_HASH_B64;
  } else {
    m_my_last_blk_hash_b64 = TypeConverter::encodeBase64(hash_and_height.first);
  }

  m_recv_block_list.clear();

  sendBlockRequest(0);

  messageFetch();
  blockSyncControl();
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
          msg_type == MessageType::MSG_ECHO ||
          msg_type == MessageType::MSG_LEAVE);
}
}; // namespace gruut