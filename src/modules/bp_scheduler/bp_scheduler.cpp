#include "bp_scheduler.hpp"
#include "../../application.hpp"

#include "easy_logging.hpp"

namespace gruut {

BpScheduler::BpScheduler() {
  m_setting = Setting::getInstance();
  m_conn_manager = ConnManager::getInstance();

  m_my_mid = m_setting->getMyId();
  m_my_cid = m_setting->getLocalChainId();

  m_my_mid_b64 = TypeConverter::encodeBase64(m_my_mid);
  m_my_cid_b64 = TypeConverter::encodeBase64(m_my_cid);

  int rand_ping_time = PRNG::getRange(1, config::BP_PING_PERIOD + 1) * 1000 +
                       PRNG::getRange(0, 999);

  auto &io_service = Application::app().getIoService();

  m_ping_scheduler.setIoService(io_service);
  m_ping_scheduler.setTaskFunction([this]() { sendPingMessage(); });
  m_ping_scheduler.setTime(rand_ping_time, config::BP_INTERVAL * 1000);
  m_ping_scheduler.setStrandMod();

  m_lock_scheduler.setIoService(io_service);
  m_lock_scheduler.setTaskFunction([this]() { lockStatus(); });
  m_lock_scheduler.setTime((config::BP_INTERVAL - 1) * 1000,
                           config::BP_INTERVAL * 1000);
  m_lock_scheduler.setStrandMod();

  el::Loggers::getLogger("BPSC");
}

void BpScheduler::start() {

  m_up_slot = Time::now_int() / config::BP_INTERVAL;
  updateRecvStatus(m_my_mid, m_up_slot, BpStatus::IN_BOOT_WAIT);

  m_lock_scheduler.runTaskOnTime();
  m_ping_scheduler.runTaskOnTime();
}

void BpScheduler::setWelcome(bool st) { m_welcome = st; }

BpStatus BpScheduler::stringToStatus(const std::string &str) {
  BpStatus ret_status = BpStatus::UNKNOWN;
  for (auto &item : STATUS_STRING) {
    if (str == item.second) {
      ret_status = item.first;
      break;
    }
  }
  return ret_status;
}

std::string BpScheduler::statusToString(BpStatus status) {
  auto it_map = STATUS_STRING.find(status);
  std::string ret_str = "UNKNOWN";
  if (it_map != STATUS_STRING.end()) {
    ret_str = it_map->second;
  }
  return ret_str;
}

void BpScheduler::reschedule() {
  if (m_is_lock)
    return;

  if (m_current_status == BpStatus::ERROR_ON_SIGNERS)
    return;

  BpRecvStatusInfo my_bp_status;
  for (BpRecvStatusInfo &item : m_recv_status) {
    if (m_my_mid == std::get<0>(item)) {
      my_bp_status = item;
      break;
    }
  }

  size_t current_time_slot = Time::now_int() / BP_INTERVAL;
  BpStatus my_prev_status = std::get<2>(my_bp_status);

  if (my_prev_status == BpStatus::IN_BOOT_WAIT) {
    m_current_status = BpStatus::IDLE;
    return;
  }

  size_t my_pos = 0;
  std::vector<std::tuple<merger_id_type, BpStatus, BpStatus>>
      possible_merger_list;
  bool is_all_idle = true;
  for (BpRecvStatusInfo &item : m_recv_status) {
    size_t time_slot = std::get<1>(item);
    BpStatus status = std::get<2>(item);
    if (time_slot == current_time_slot &&
        (status == BpStatus::IDLE || status == BpStatus::PRIMARY ||
         status == BpStatus::SECONDARY))
      possible_merger_list.emplace_back(
          std::make_tuple(std::get<0>(item), status, BpStatus::UNKNOWN));

    if (status != BpStatus::IDLE) {
      is_all_idle = false;
    }

    if (m_my_mid == std::get<0>(item)) {
      my_pos = possible_merger_list.size() - 1;
    }
  }

  size_t num_possible_mergers = possible_merger_list.size();

  if (num_possible_mergers == 0) {
    // in this case, we cannot determine current status
    return;
  }

  m_block_producers.clear();

  if (num_possible_mergers == 1) { // I am the only merger
    m_current_status = BpStatus::PRIMARY;
    m_block_producers.emplace_back(m_my_mid);
    return;
  }

  if (is_all_idle) { // All mergers were IDLE

    if (my_pos == 0)
      m_current_status = BpStatus::PRIMARY;
    else if (my_pos == 1)
      m_current_status = BpStatus::SECONDARY;
    else
      m_current_status = BpStatus::IDLE;

    m_block_producers.emplace_back(std::get<0>(possible_merger_list[0]));
    if (num_possible_mergers >= 2) {
      m_block_producers.emplace_back(std::get<0>(possible_merger_list[1]));
    }

    return;
  }

  // step 1) find PRIMARY

  size_t primary_pos = 0;

  bool found_primary = false;

  for (size_t i = 0; i < num_possible_mergers; ++i) {
    if (std::get<1>(possible_merger_list[i]) == BpStatus::SECONDARY) {
      found_primary = true;
      std::get<2>(possible_merger_list[i]) = BpStatus::PRIMARY;
      primary_pos = i;
      break;
    }
  }

  // no secondary, in this case, primary becomes primary again
  if (!found_primary) {
    for (size_t i = 0; i < num_possible_mergers; ++i) {
      if (std::get<1>(possible_merger_list[i]) == BpStatus::PRIMARY) {
        std::get<2>(possible_merger_list[i]) = BpStatus::PRIMARY;
        primary_pos = i;
        break;
      }
    }
  }

  // step 2) find SECONDARY

  size_t secondary_pos = (primary_pos + 1) % num_possible_mergers;
  std::get<2>(possible_merger_list[secondary_pos]) = BpStatus::SECONDARY;

  m_block_producers.emplace_back(
      std::get<0>(possible_merger_list[primary_pos]));
  m_block_producers.emplace_back(
      std::get<0>(possible_merger_list[secondary_pos]));

  // step 3) all others will be IDLE

  for (auto &possible_bp : possible_merger_list) {
    if (std::get<2>(possible_bp) == BpStatus::PRIMARY ||
        std::get<2>(possible_bp) == BpStatus::SECONDARY) {
      continue;
    }
    std::get<2>(possible_bp) = BpStatus::IDLE;
  }

  // setp 4) update my status
  m_current_status = std::get<2>(possible_merger_list[my_pos]);
}

void BpScheduler::lockStatus() {
  m_is_lock = true; // lock to update status
  Application::app().getTransactionCollector().setTxCollectStatus(
      m_current_status, m_block_producers);
}

void BpScheduler::sendPingMessage() {
  timestamp_t current_time = Time::now_int();
  size_t current_slot = current_time / config::BP_INTERVAL;

  size_t num_signers = SignerPool::getInstance()->getNumSignerBy();

  if (m_current_status != BpStatus::IN_BOOT_WAIT &&
      num_signers < config::MIN_SIGNATURE_COLLECT_SIZE) {
    m_current_status = BpStatus::ERROR_ON_SIGNERS;
  }

  if (m_current_status == BpStatus::ERROR_ON_SIGNERS &&
      num_signers >= config::MIN_SIGNATURE_COLLECT_SIZE) {
    // It was IDLE, even I said ERROR_ON_SIGNERS.
    m_current_status = BpStatus::IDLE;
  }

  OutputMsgEntry output_msg;
  output_msg.type = MessageType::MSG_PING;
  output_msg.body["mID"] = m_my_mid_b64;
  output_msg.body["cID"] = m_my_cid_b64;
  output_msg.body["time"] = to_string(current_time);
  output_msg.body["sCnt"] = to_string(num_signers);
  output_msg.body["stat"] = statusToString(m_current_status);

  m_msg_proxy.deliverOutputMessage(output_msg);

  CLOG(INFO, "BPSC") << "Send MSG_PING (" << m_my_mid_b64 << "," << num_signers
                     << "," << statusToString(m_current_status) << ")";

  m_is_lock = false; // unlock to update status
  updateRecvStatus(m_my_mid, current_slot, m_current_status);
}

void BpScheduler::updateRecvStatus(const merger_id_type &merger_id,
                                   size_t timeslot, BpStatus stat) {

  // NO-BAKGGU table!

  bool is_updatable = false;
  for (BpRecvStatusInfo &item : m_recv_status) {
    if (std::get<0>(item) == merger_id) {
      std::lock_guard<std::mutex> lock(m_recv_status_mutex);
      is_updatable = true;
      std::get<1>(item) = timeslot;
      std::get<2>(item) = stat;
      m_recv_status_mutex.unlock();
      break;
    }
  }

  if (!is_updatable) {
    std::lock_guard<std::mutex> lock(m_recv_status_mutex);
    m_recv_status.emplace_back(make_tuple(merger_id, timeslot, stat));
    std::sort(m_recv_status.begin(), m_recv_status.end(),
              [](const BpRecvStatusInfo &a, const BpRecvStatusInfo &b) {
                return get<0>(a) < get<0>(b);
              });
    m_recv_status_mutex.unlock();
  }
  reschedule();
}

void BpScheduler::handleMessage(InputMsgEntry &msg) {

  if (Safe::getString(msg.body, "cID") != m_my_cid_b64)
    return; // wrong chain

  std::string merger_id_b64 = Safe::getString(msg.body, "mID");
  merger_id_type merger_id =
      Safe::getBytesFromB64<merger_id_type>(msg.body, "mID");
  timestamp_t merger_time = Safe::getTime(msg.body, "time");
  size_t timeslot = merger_time / config::BP_INTERVAL;

  if (abs((int64_t)merger_time - (int64_t)Time::now_int()) >
      config::BP_INTERVAL / 2) {
    return;
  }

  switch (msg.type) {
  case MessageType::MSG_PING: {

    auto num_singers = Safe::getInt(msg.body, "sCnt");

    BpStatus status = stringToStatus(Safe::getString(msg.body, "stat"));
    if (num_singers < config::MIN_SIGNATURE_COLLECT_SIZE)
      status = BpStatus::ERROR_ON_SIGNERS;
    updateRecvStatus(merger_id, timeslot, status);

  } break;
  case MessageType::MSG_UP: {

    MergerInfo merger_info;
    merger_info.id = merger_id;
    merger_info.address = Safe::getString(msg.body, "ip");
    merger_info.port = Safe::getString(msg.body, "port");
    merger_info.cert = Safe::getString(msg.body, "mCert");
    m_conn_manager->setMergerInfo(merger_info, true);

    msg.body.erase("time");
    msg.body.erase("ver");
    msg.body.erase("cID");
    m_conn_manager->writeMergerInfo(msg.body);

    updateRecvStatus(merger_id, timeslot, BpStatus::IN_BOOT_WAIT);

    OutputMsgEntry output_msg;
    output_msg.type = MessageType::MSG_WELCOME;
    output_msg.body["mID"] = m_my_mid_b64;
    output_msg.body["cID"] = m_my_cid_b64;
    output_msg.body["time"] = Time::now();
    output_msg.body["val"] = true;
    output_msg.body["merger"] = m_conn_manager->getAllMergerInfoJson();
    output_msg.body["se"] = m_conn_manager->getAllSeInfoJson();

    output_msg.receivers = {merger_id};

    MessageProxy msg_proxy;
    msg_proxy.deliverOutputMessage(output_msg);

  } break;

  case MessageType::MSG_WELCOME: {
    bool val = Safe::getBoolean(msg.body, "val");
    if (!val) {
      CLOG(INFO, "BPSC") << "COULD NOT JOIN MERGER NET [" << merger_id_b64
                         << "]";
      break;
    }

    CLOG(INFO, "BPSC") << "WELCOME! [" << merger_id_b64 << "]";
    m_welcome = true;

    if (msg.body.find("merger") != msg.body.end() &&
        msg.body.find("se") != msg.body.end()) {

      m_conn_manager->setMultiMergerInfo(msg.body["merger"]);
      for (auto &merger_info : msg.body["merger"]) {
        std::string merger_info_id_b64 = Safe::getString(merger_info, "mID");
        if (m_my_mid_b64 == merger_info_id_b64)
          continue;

        m_conn_manager->writeMergerInfo(merger_info);
      }
      m_conn_manager->setMultiSeInfo(msg.body["se"]);
    }

  } break;

  default:
    break;
  }
}

} // namespace gruut