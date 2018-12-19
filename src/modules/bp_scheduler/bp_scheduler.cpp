#include "bp_scheduler.hpp"
#include "../../application.hpp"

namespace gruut {

BpScheduler::BpScheduler() { m_setting = Setting::getInstance(); }

void BpScheduler::start() {
  m_timer.reset(
      new boost::asio::deadline_timer(Application::app().getIoService()));
  m_lock_timer.reset(
      new boost::asio::deadline_timer(Application::app().getIoService()));
  setMyIds();
  m_up_slot = Time::now_int() / config::BP_INTERVAL;
  updateRecvStatus(m_my_mid_b64, m_up_slot, BpStatus::IN_BOOT_WAIT);


  lockStatus();
  sendPing();
}

void BpScheduler::setMyIds() {
  id_type my_mid = m_setting->getMyId();
  local_chain_id_type my_cid = m_setting->getLocalChainId();

  m_my_mid_b64 = TypeConverter::toBase64Str(my_mid);
  m_my_cid_b64 = TypeConverter::toBase64Str(my_cid);
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

  if (m_current_status != BpStatus::ERROR_ON_SIGNERS) {
    BpRecvStatusInfo my_bp_status;
    for (BpRecvStatusInfo &item : m_recv_status) {
      if (m_my_mid_b64 == get<0>(item)) {
        my_bp_status = item;
        break;
      }
    }

    size_t current_time_slot = Time::now_int() / BP_INTERVAL;
    BpStatus my_prev_status = get<2>(my_bp_status);

    if (my_prev_status == BpStatus::SECONDARY) {
      m_current_status = BpStatus::PRIMARY;
    } else if (my_prev_status == BpStatus::IN_BOOT_WAIT) {
      m_current_status = BpStatus::IDLE;
    } else {
      size_t my_pos = 0;
      std::vector<BpRecvStatusInfo> possible_bp_list;
      for (BpRecvStatusInfo &item : m_recv_status) {
        size_t time_slot = get<1>(item);
        BpStatus status = get<2>(item);
        if (time_slot == current_time_slot &&
            (status == BpStatus::IDLE || status == BpStatus::PRIMARY ||
             status == BpStatus::SECONDARY))
          possible_bp_list.emplace_back(item);

        if (m_my_mid_b64 == get<0>(item)) {
          my_pos = possible_bp_list.size() - 1;
        }
      }

      size_t list_size = possible_bp_list.size();

      if (my_prev_status == BpStatus::PRIMARY) {
        if (list_size > 2)
          m_current_status = BpStatus::IDLE;
        else if (list_size == 2) {
          size_t others_pos = (my_pos + 1) % 2;
          BpStatus others_status = get<2>(possible_bp_list[others_pos]);
          if (others_status == BpStatus::IDLE)
            m_current_status = BpStatus::PRIMARY;
          else
            m_current_status = BpStatus::SECONDARY;
        } else
          m_current_status = BpStatus::PRIMARY;
      } else {
        if (list_size == 1)
          m_current_status = BpStatus::PRIMARY;
        else {
          size_t prev_pos = (list_size + my_pos - 1) % list_size;
          BpStatus others_status = get<2>(possible_bp_list[prev_pos]);
          if (others_status == BpStatus::SECONDARY ||
              others_status == BpStatus::PRIMARY)
            m_current_status = BpStatus::SECONDARY;
          else
            m_current_status = BpStatus::IDLE;
        }
      }
    }
  }
}

void BpScheduler::lockStatus() {

  timestamp_type current_time = Time::now_int();

  size_t current_slot = current_time / config::BP_INTERVAL;
  time_t next_slot_begin = (current_slot + 1) * config::BP_INTERVAL;
  time_t next_lock_time = (current_time >= next_slot_begin - 1)
                              ? (next_slot_begin - 1 + config::BP_INTERVAL)
                              : (next_slot_begin - 1);
  boost::posix_time::ptime lock_time =
      boost::posix_time::from_time_t(next_lock_time);

  cout << "BPS: lockStatus(" << next_lock_time << ")" << endl << flush;

  m_lock_timer->expires_at(lock_time);
  m_lock_timer->async_wait([this](const boost::system::error_code &ec) {
    if (ec == boost::asio::error::operation_aborted) {
    } else if (ec.value() == 0) {
      postLockJob();
      lockStatus();
    } else {
      throw;
    }
  });
}

void BpScheduler::postLockJob() {
  auto &io_service = Application::app().getIoService();
  io_service.post([this]() {
    Application::app().getTransactionCollector().setTxCollectStatus(
        m_current_status);
    m_is_lock = true;
  });
}

void BpScheduler::sendPing() {
  auto &io_service = Application::app().getIoService();

  io_service.post([this]() {
    timestamp_type current_time = Time::now_int();
    size_t current_slot = current_time / config::BP_INTERVAL;

    if (m_up_slot == current_slot) {
      return;
    }

    auto &signer_pool = Application::app().getSignerPool();

    size_t num_signers = signer_pool.size();
    if (num_signers < config::MIN_SIGNATURE_COLLECT_SIZE) {
      m_current_status = BpStatus::ERROR_ON_SIGNERS;
    }

    cout << "BPS: sendPing(" << m_my_mid_b64 << "," << num_signers << ","
         << statusToString(m_current_status) << ")" << endl;

    nlohmann::json ping_msg;
    ping_msg["mID"] = m_my_mid_b64;
    ping_msg["time"] = to_string(current_time);
    ping_msg["sCnt"] = to_string(num_signers);
    ping_msg["stat"] = statusToString(m_current_status);

    OutputMsgEntry output_msg(MessageType::MSG_PING, ping_msg);
    m_msg_proxy.deliverOutputMessage(output_msg);

    m_is_lock = false;
    updateRecvStatus(m_my_mid_b64, current_slot, m_current_status);
  });

  size_t current_slot = Time::now_int() / config::BP_INTERVAL;
  time_t next_slot_begin = (current_slot + 1) * config::BP_INTERVAL;
  // time_t next_slot_end = (current_slot + 2) * BP_INTERVAL - 1;

  boost::posix_time::ptime ping_time = boost::posix_time::from_time_t(
      PRNG::getRange(0, config::BP_PING_PERIOD) + next_slot_begin);
  ping_time += boost::posix_time::milliseconds(PRNG::getRange(0, 999));

  m_timer->expires_at(ping_time);
  m_timer->async_wait([this](const boost::system::error_code &ec) {
    if (ec == boost::asio::error::operation_aborted) {
    } else if (ec.value() == 0) {
      sendPing();
    } else {
      throw;
    }
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

  std::string merger_id_b64 = msg.body["mID"].get<std::string>();
  timestamp_type merger_time =
      (timestamp_type)std::stoll(msg.body["time"].get<std::string>());
  size_t timeslot = merger_time / config::BP_INTERVAL;

  if (abs((int64_t)merger_time - (int64_t)Time::now_int()) >
      config::BP_INTERVAL / 2) {
    return;
  }

  switch (msg.type) {
  case MessageType::MSG_PING: {

    std::string num_signers_str = msg.body["sCnt"].get<std::string>();
    auto num_singers = static_cast<size_t>(std::stoll(num_signers_str));

    BpStatus status = stringToStatus(msg.body["stat"].get<std::string>());
    if (num_singers < config::MIN_SIGNATURE_COLLECT_SIZE)
      status = BpStatus::ERROR_ON_SIGNERS;
    updateRecvStatus(merger_id_b64, timeslot, status);

  } break;
  case MessageType::MSG_UP: {
    // std::string ver = msg.body["ver"].get<std::string>();
    if (msg.body["cID"].get<std::string>() != m_my_cid_b64) {
      break;
    }
    updateRecvStatus(merger_id_b64, timeslot, BpStatus::IN_BOOT_WAIT);
  } break;

  default:
    break;
  }
}

} // namespace gruut