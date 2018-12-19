#pragma once
#include "../module.hpp"
#include "../../chain/types.hpp"
#include "../../services/input_queue.hpp"
#include "../../services/output_queue.hpp"
#include "../../utils/time.hpp"
#include "../../utils/type_converter.hpp"
#include "../../services/setting.hpp"

#include "boost/date_time/local_time/local_time.hpp"
#include <boost/asio.hpp>
#include <algorithm>
#include <chrono>
#include <memory>
#include <random>
#include <vector>

namespace gruut {

const std::map<BpStatus, std::string> STATUS_STRING = {
    {BpStatus::IN_BOOT_WAIT, "IN_BOOT_WAIT"},
    {BpStatus::IDLE, "IDLE"},
    {BpStatus::PRIMARY, "PRIMARY"},
    {BpStatus::SECONDARY, "SECONDARY"},
    {BpStatus::ERROR_ON_SIGNERS, "ERROR_ON_SIGNERS"},
    {BpStatus::UNKNOWN, "UNKNOWN"},
};

using BpRecvStatusInfo = std::tuple<std::string, size_t, BpStatus>;

class BpScheduler : public Module {
public:
  BpScheduler();
  void handleMessage(InputMsgEntry &msg);
  void start() override;

private:
  void setMyIds();
  void sendPingMsg();
  void lockStatus();
  void updateRecvStatus(const std::string &id_b64, size_t timeslot,
                        BpStatus stat);
  void reschedule();

  void postLockJob();

  std::string statusToString(BpStatus status);
  BpStatus stringToStatus(const std::string &str);

  merger_id_type m_my_mid;
  std::string m_my_mid_b64;
  local_chain_id_type m_my_cid;
  std::string m_my_cid_b64;

  timestamp_type m_up_time{0};
  BpStatus m_current_status;
  bool m_is_lock{true};
  std::vector<BpRecvStatusInfo> m_recv_status;

  std::mutex m_recv_status_mutex;

  std::unique_ptr<boost::asio::deadline_timer> m_timer;
  std::unique_ptr<boost::asio::deadline_timer> m_lock_timer;
  OutputQueueAlt *m_output_queue;
  Setting *m_setting;
};

} // namespace gruut
