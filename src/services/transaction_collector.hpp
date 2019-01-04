#ifndef GRUUT_ENTERPRISE_MERGER_TRANSACTION_COLLECTOR_HPP
#define GRUUT_ENTERPRISE_MERGER_TRANSACTION_COLLECTOR_HPP

#include "nlohmann/json.hpp"

#include "../chain/transaction.hpp"
#include "../chain/types.hpp"
#include "../modules/module.hpp"
#include "../utils/bytes_builder.hpp"
#include "../utils/rsa.hpp"
#include "../utils/type_converter.hpp"
#include "setting.hpp"
#include "signature_requester.hpp"

#include <boost/asio.hpp>
#include <boost/assert.hpp>
#include <botan-2/botan/base64.h>
#include <botan-2/botan/data_src.h>
#include <botan-2/botan/x509_key.h>

#include <iostream>
#include <memory>
#include <queue>

namespace gruut {

enum class BpJobStatus { DO, DONT, UNKNOWN };

class TransactionCollector {
public:
  TransactionCollector();
  void handleMessage(json &msg_body_json);
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