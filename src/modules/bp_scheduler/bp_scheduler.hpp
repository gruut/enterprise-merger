#ifndef GRUUT_ENTERPRISE_MERGER_BP_SCHEDULER_HPP
#define GRUUT_ENTERPRISE_MERGER_BP_SCHEDULER_HPP

#include "../../chain/types.hpp"
#include "../../services/input_queue.hpp"
#include "../../services/message_proxy.hpp"
#include "../../services/setting.hpp"
#include "../../utils/periodic_task.hpp"
#include "../../utils/time.hpp"
#include "../../utils/type_converter.hpp"
#include "../communication/manage_connection.hpp"
#include "../module.hpp"

#include "boost/date_time/local_time/local_time.hpp"
#include <algorithm>
#include <atomic>
#include <boost/asio.hpp>
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

using BpRecvStatusInfo = std::tuple<merger_id_type, size_t, BpStatus>;

class BpScheduler : public Module {
public:
  BpScheduler();
  void handleMessage(InputMsgEntry &msg);
  void start() override;
  void setWelcome(bool st);

private:
  void sendPingMessage();
  void lockStatus();

  void updateRecvStatus(const merger_id_type &merger_id, size_t timeslot,
                        BpStatus stat);
  void reschedule();

  std::string statusToString(BpStatus status);
  BpStatus stringToStatus(const std::string &str);

  merger_id_type m_my_mid;
  std::string m_my_mid_b64;
  localchain_id_type m_my_cid;
  std::string m_my_cid_b64;

  size_t m_up_slot{0};
  BpStatus m_current_status{BpStatus::IN_BOOT_WAIT};

  std::atomic<bool> m_is_lock{true};
  std::atomic<bool> m_welcome{true};

  std::vector<BpRecvStatusInfo> m_recv_status;
  std::vector<merger_id_type> m_block_producers;

  std::mutex m_recv_status_mutex;

  TaskOnTime m_lock_scheduler;
  TaskOnTime m_ping_scheduler;

  MessageProxy m_msg_proxy;
  ConnManager *m_conn_manager;
  Setting *m_setting;
};

} // namespace gruut

#endif
