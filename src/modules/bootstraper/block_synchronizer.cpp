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

bool BlockSynchronizer::pushMsgToBlockList(InputMsgEntry &input_msg_entry) {

  CLOG(INFO, "BSYN") << "called pushMsgToBlockList()";

  std::string sender_id_b64 = input_msg_entry.body["mID"].get<std::string>();

  std::string block_raw_str =
      input_msg_entry.body["blockraw"].get<std::string>();
  bytes block_raw = TypeConverter::decodeBase64(block_raw_str);

  nlohmann::json block_json = BlockValidator::getBlockJson(block_raw);

  if (block_json.empty()) {
    CLOG(ERROR, "BSYN") << "Block dropped (invalid block)";
    return false;
  }

  int block_hgt = stoi(block_json["hgt"].get<std::string>());

  if (block_hgt <= m_my_last_height) {
    CLOG(INFO, "BSYN") << "Block dropped (old block)";
    return false;
  }

  auto it_map = m_recv_block_list.find(block_hgt);

  if (it_map == m_recv_block_list.end()) {
    RcvBlock temp;
    temp.merger_id_b64 = sender_id_b64;
    temp.hash = Sha256::hash(block_raw);
    temp.block_raw = std::move(block_raw);
    temp.block_json = std::move(block_json);
    temp.txs = Safe::parseJson(input_msg_entry.body["tx"].get<std::string>());

    std::lock_guard<std::mutex> lock(m_block_list_mutex);

    m_recv_block_list.insert(make_pair(block_hgt, temp));

    m_block_list_mutex.unlock();
  } else if (it_map->second.state == BlockState::RETRIED) {
    std::lock_guard<std::mutex> lock(m_block_list_mutex);

    it_map->second.merger_id_b64 = sender_id_b64;
    it_map->second.hash = Sha256::hash(block_raw);
    it_map->second.block_raw = std::move(block_raw);
    it_map->second.block_json = std::move(block_json);
    it_map->second.txs =
        Safe::parseJson(input_msg_entry.body["tx"].get<std::string>());
    it_map->second.mtree = {};

    m_block_list_mutex.unlock();
  } else {
    CLOG(ERROR, "BSYN") << "Block dropped (unknown)";
    return false;
  }

  if (m_first_recv_block_height < 0)
    m_first_recv_block_height = block_hgt;

  return true;
}

bool BlockSynchronizer::sendBlockRequest(int height) {

  std::vector<id_type> receivers = {};

  if (height != -1) { // unicast

    if (m_recv_block_list.empty()) { // but, no body
      return false;
    }

    std::vector<std::string> ans_merger_list;

    for (auto &blk_item : m_recv_block_list) {
      ans_merger_list.emplace_back(blk_item.second.merger_id_b64);
    }

    std::string ans_merger_id_b64 =
        ans_merger_list[PRNG::getRange(0, (int)(ans_merger_list.size() - 1))];
    merger_id_type ans_merger_id =
        TypeConverter::decodeBase64(ans_merger_id_b64);
    receivers.emplace_back(ans_merger_id);
  }

  m_last_task_time = static_cast<timestamp_type>(Time::now_int());

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

  return true;
}

bool BlockSynchronizer::validateBlock(int height) {
  auto it_map = m_recv_block_list.find(height);
  if (it_map == m_recv_block_list.end())
    return false;

  std::vector<sha256> mtree;
  std::vector<transaction_id_type> dummy_tx_ids;

  if (BlockValidator::validate(it_map->second.block_json, it_map->second.txs,
                               mtree, dummy_tx_ids)) {
    it_map->second.mtree = mtree;
  }

  return true;
}

void BlockSynchronizer::saveBlock(int height) {

  auto it_map = m_recv_block_list.find(height);
  if (it_map == m_recv_block_list.end()) {
    return;
  }

  size_t num_txs = it_map->second.txs.size();

  std::vector<std::string> mtree_nodes_b64(num_txs);

  for (size_t i = 0; i < num_txs; ++i) { // to save data, we need only digests
    mtree_nodes_b64[i] = TypeConverter::encodeBase64(it_map->second.mtree[i]);
  }

  nlohmann::json block_body;
  block_body["tx"] = it_map->second.txs;
  block_body["txCnt"] = to_string(num_txs);
  block_body["mtree"] = mtree_nodes_b64;

  m_storage->saveBlock(std::string(it_map->second.block_raw.begin(),
                                   it_map->second.block_raw.end()),
                       it_map->second.block_json, block_body);
}

void BlockSynchronizer::syncFinish() {

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
    if (Time::now_int() - m_last_task_time > config::MAX_WAIT_TIME) {
      m_sync_done = true;
      m_sync_fail = true;

      syncFinish();

      return;
    }

    if (m_recv_block_list.empty()) { // not over, but empty
      return;
    }

    // step 1 - validate min height block
    for (auto &blk_item : m_recv_block_list) {
      if (blk_item.first == m_my_last_height + 1) {
        if (blk_item.second.block_json["prevH"].get<std::string>() ==
            m_my_last_blk_hash_b64) {
          if (validateBlock((int)blk_item.first)) {

            std::lock_guard<std::mutex> lock(m_block_list_mutex);
            blk_item.second.state = BlockState::TOSAVE;
            m_block_list_mutex.unlock();

            m_my_last_blk_hash_b64 =
                TypeConverter::encodeBase64(blk_item.second.hash);
            m_my_last_height = blk_item.first;

            m_last_task_time = Time::now_int();

          } else {
            std::lock_guard<std::mutex> lock(m_block_list_mutex);
            blk_item.second.state = BlockState::RETRIED;
            m_block_list_mutex.unlock();
          }
        }
      }
    }

    // step 2 - save block
    for (auto &blk_item : m_recv_block_list) {
      if (blk_item.second.state == BlockState::TOSAVE) {
        saveBlock((int)blk_item.first);
        blk_item.second.state = BlockState::TODELETE;
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

    // step 4 - retry block
    for (auto &blk_item : m_recv_block_list) {

      if (blk_item.second.num_retry > config::MAX_REQ_BLOCK_RETRY) {
        m_sync_done = true;
        m_sync_fail = true;
        break;
      }

      if (blk_item.second.state == BlockState::RETRIED) {
        std::lock_guard<std::mutex> lock(m_block_list_mutex);
        blk_item.second.num_retry += 1;
        m_block_list_mutex.unlock();
        sendBlockRequest((int)blk_item.first);
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

  if (input_msg_entry.body["sID"].empty())
    return;

  std::string signer_id_b64 = input_msg_entry.body["sID"];
  signer_id_type signer_id = TypeConverter::decodeBase64(signer_id_b64);

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
        input_msg_entry.body["type"].get<std::string>() ==
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

  CLOG(INFO, "BSYN") << "called startBlockSync()";

  m_finish_callback = std::move(callback);

  std::pair<std::string, size_t> hash_and_height =
      m_storage->findLatestHashAndHeight();
  m_my_last_height = hash_and_height.second;
  m_my_last_blk_hash_b64 = hash_and_height.first;

  m_recv_block_list.clear();

  sendBlockRequest(-1);

  messageFetch();
  blockSyncControl();
}

bool BlockSynchronizer::checkMsgFromOtherMerger(MessageType msg_type) {
  return (msg_type == MessageType::MSG_UP ||
          msg_type == MessageType::MSG_PING ||
          msg_type == MessageType::MSG_REQ_BLOCK ||
          msg_type == MessageType::MSG_ERROR);
}

bool BlockSynchronizer::checkMsgFromSigner(MessageType msg_type) {
  return (msg_type == MessageType::MSG_JOIN ||
          msg_type == MessageType::MSG_RESPONSE_1 ||
          msg_type == MessageType::MSG_ECHO ||
          msg_type == MessageType::MSG_LEAVE);
}
}; // namespace gruut