#ifndef GRUUT_ENTERPRISE_MERGER_TRANSACTION_COLLECTOR_HPP
#define GRUUT_ENTERPRISE_MERGER_TRANSACTION_COLLECTOR_HPP

#include <boost/asio.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <queue>

#include "../chain/types.hpp"
#include "../modules/module.hpp"
#include "setting.hpp"
#include "signature_requester.hpp"

namespace gruut {

enum class BpJobStatus { DO, DONT, UNKNOWN };

class TransactionCollector {
public:
  TransactionCollector();
  void handleMessage(nlohmann::json &msg_body_json);
  void setTxCollectStatus(BpStatus status);

private:
  bool isRunnable();
  void turnOnTimer();
  void updateStatus();
  void postJob();

  BpStatus m_current_tx_status{BpStatus::IN_BOOT_WAIT};
  BpStatus m_next_tx_status{BpStatus::UNKNOWN};
  std::unique_ptr<boost::asio::deadline_timer> m_timer;
  SignatureRequester m_signature_requester;
  std::deque<BpJobStatus> m_bpjob_sequence;

  bool m_timer_running{false};
};
} // namespace gruut
#endif