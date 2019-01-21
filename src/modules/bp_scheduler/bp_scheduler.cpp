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

  auto &io_service = Application::app().getIoService();
  m_timer.reset(new boost::asio::deadline_timer(io_service));
  m_lock_timer.reset(new boost::asio::deadline_timer(io_service));

  el::Loggers::getLogger("BPSC");
}

void BpScheduler::start() {

  m_up_slot = Time::now_int() / config::BP_INTERVAL;
  updateRecvStatus(m_my_mid_b64, m_up_slot, BpStatus::IN_BOOT_WAIT);

  lockStatusloop();
  sendPingloop();
}

void BpScheduler::setWelcome(bool st) {
  m_welcome = st;
}

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
    if (m_my_mid_b64 == std::get<0>(item)) {
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
  std::vector<std::tuple<std::string, BpStatus, BpStatus>> possible_merger_list;
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

    if (m_my_mid_b64 == std::get<0>(item)) {
      my_pos = possible_merger_list.size() - 1;
    }
  }

  size_t num_possible_mergers = possible_merger_list.size();

  if (num_possible_mergers ==
      0) { // in this case, we cannot determine current status
    return;
  }

  if (num_possible_mergers == 1) { // I am the only merger
    m_current_status = BpStatus::PRIMARY;
    return;
  }

  if (is_all_idle) { // All mergers were IDLE

    if (my_pos == 0)
      m_current_status = BpStatus::PRIMARY;
    else if (my_pos == 1)
      m_current_status = BpStatus::SECONDARY;
    else
      m_current_status = BpStatus::IDLE;

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

  if (!found_primary) { // no secondary, in this case, primary becomes primary
                        // again
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

void BpScheduler::lockStatusloop() {

  timestamp_t current_time = Time::now_int();

  size_t current_slot = current_time / config::BP_INTERVAL;
  time_t next_slot_begin = (current_slot + 1) * config::BP_INTERVAL;
  time_t next_lock_time = (current_time >= next_slot_begin - 1)
                              ? (next_slot_begin - 1 + config::BP_INTERVAL)
                              : (next_slot_begin - 1);
  boost::posix_time::ptime lock_time =
      boost::posix_time::from_time_t(next_lock_time);

  // CLOG(INFO, "BPSC") << "called lockStatus (time=" << next_lock_time << ")";

  m_lock_timer->expires_at(lock_time);
  m_lock_timer->async_wait([this](const boost::system::error_code &ec) {
    if (ec == boost::asio::error::operation_aborted) {
      CLOG(INFO, "BPSC") << "LockTimer ABORTED";
    } else if (ec.value() == 0) {
      postLockJob();
      lockStatusloop();
    } else {
      CLOG(ERROR, "BPSC") << ec.message();
      // throw;
    }
  });
}

void BpScheduler::postLockJob() {
  auto &io_service = Application::app().getIoService();
  io_service.post([this]() {
    Application::app().getTransactionCollector().setTxCollectStatus(
        m_current_status);
    m_is_lock = true; // lock to update status
  });
}

void BpScheduler::sendPingloop() {
  size_t current_slot = Time::now_int() / config::BP_INTERVAL;
  time_t next_slot_begin = (current_slot + 1) * config::BP_INTERVAL;

  boost::posix_time::ptime ping_time = boost::posix_time::from_time_t(
      PRNG::getRange(0, config::BP_PING_PERIOD) + next_slot_begin);
  ping_time += boost::posix_time::milliseconds(PRNG::getRange(0, 999));

  m_timer->expires_at(ping_time);
  m_timer->async_wait([this](const boost::system::error_code &ec) {
    if (ec == boost::asio::error::operation_aborted) {
      CLOG(INFO, "BPSC") << "PingTimer ABORTED";
    } else if (ec.value() == 0) {
      if(m_welcome)
        postSendPingJob();
      sendPingloop();
    } else {
      CLOG(ERROR, "BPSC") << ec.message();
      // throw;
    }
  });
}

void BpScheduler::postSendPingJob() {
  auto &io_service = Application::app().getIoService();

  io_service.post([this]() {
    timestamp_t current_time = Time::now_int();
    size_t current_slot = current_time / config::BP_INTERVAL;

    auto signer_pool = SignerPool::getInstance();

    size_t num_signers = signer_pool->getNumSignerBy();
    if (m_current_status != BpStatus::IN_BOOT_WAIT &&
        num_signers < config::MIN_SIGNATURE_COLLECT_SIZE) {
      m_current_status = BpStatus::ERROR_ON_SIGNERS;
    }

    if (m_current_status == BpStatus::ERROR_ON_SIGNERS &&
        num_signers >= config::MIN_SIGNATURE_COLLECT_SIZE) {
      m_current_status =
          BpStatus::IDLE; // It was IDLE, even I said ERROR_ON_SIGNERS.
    }

    CLOG(INFO, "BPSC") << "Send MSG_PING (" << m_my_mid_b64 << ","
                       << num_signers << "," << statusToString(m_current_status)
                       << ")";

    OutputMsgEntry output_msg;
    output_msg.type = MessageType::MSG_PING;
    output_msg.body["mID"] = m_my_mid_b64;
    output_msg.body["time"] = to_string(current_time);
    output_msg.body["sCnt"] = to_string(num_signers);
    output_msg.body["stat"] = statusToString(m_current_status);

    m_msg_proxy.deliverOutputMessage(output_msg);

    m_is_lock = false; // unlock to update status
    updateRecvStatus(m_my_mid_b64, current_slot, m_current_status);
  });
}

void BpScheduler::updateRecvStatus(const std::string &id_b64, size_t timeslot,
                                   BpStatus stat) {

  // NO-BAKGGU table!

  bool is_updatable = false;
  for (BpRecvStatusInfo &item : m_recv_status) {
    if (std::get<0>(item) == id_b64) {
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
    m_recv_status.emplace_back(make_tuple(id_b64, timeslot, stat));
    std::sort(m_recv_status.begin(), m_recv_status.end(),
              [](const BpRecvStatusInfo &a, const BpRecvStatusInfo &b) {
                return get<0>(a) < get<0>(b);
              });
    m_recv_status_mutex.unlock();
  }
  reschedule();
}

void BpScheduler::handleMessage(InputMsgEntry &msg) {

  std::string merger_id_b64 = Safe::getString(msg.body, "mID");
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
    updateRecvStatus(merger_id_b64, timeslot, status);

  } break;
  case MessageType::MSG_UP: {
    MessageProxy msg_proxy;
    OutputMsgEntry output_message;
    std::vector<merger_id_type> receivers{TypeConverter::decodeBase64(merger_id_b64)};
    output_message.receivers = receivers;
    output_message.type = MessageType::MSG_WELCOME;
    output_message.body["mID"] = m_my_mid_b64;
    output_message.body["time"] = Time::now();

    // TODO : MSG_UP에 대해 검증 할 요소가 추가 될 수 있습니다.
    if (Safe::getString(msg.body, "cID") != m_my_cid_b64) {
      output_message.body["val"] = false;
    } else {
      std::string ip = Safe::getString(msg.body, "ip");
      std::string port = Safe::getString(msg.body, "port");
      std::string mCert = Safe::getString(msg.body, "mCert");

      MergerInfo merger_info;
      merger_info.id = TypeConverter::decodeBase64(merger_id_b64);
      merger_info.address = ip;
      merger_info.port = port;
      merger_info.cert = mCert;
      m_conn_manager->setMergerInfo(merger_info, true);

      output_message.body["val"] = true;
      updateRecvStatus(merger_id_b64, timeslot, BpStatus::IN_BOOT_WAIT);
    }
    msg_proxy.deliverOutputMessage(output_message);
  } break;

  case MessageType::MSG_WELCOME: {
    bool val = Safe::getBoolean(msg.body, "val");
    if (val){
      CLOG(INFO, "BPSC") << "WELCOME! From(" << merger_id_b64 << ")";
      m_welcome = true;
    }

    // TODO: Merger network에 등록되지 못 할시 처리 필요.
  } break;

  default:
    break;
  }
}

} // namespace gruut