#ifndef GRUUT_ENTERPRISE_MERGER_TRANSACTION_COLLECTOR_HPP
#define GRUUT_ENTERPRISE_MERGER_TRANSACTION_COLLECTOR_HPP

#include "nlohmann/json.hpp"

#include "../chain/transaction.hpp"
#include "../chain/types.hpp"
#include "../modules/module.hpp"
#include "../utils/bytes_builder.hpp"
#include "../utils/periodic_task.hpp"
#include "../utils/rsa.hpp"
#include "../utils/type_converter.hpp"
#include "certificate_pool.hpp"
#include "setting.hpp"
#include "signature_requester.hpp"

#include <boost/asio.hpp>
#include <boost/assert.hpp>
#include <botan-2/botan/base64.h>
#include <botan-2/botan/data_src.h>
#include <botan-2/botan/x509_key.h>

#include <atomic>
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
  void checkBpJob();

  BpStatus m_current_tx_status{BpStatus::IN_BOOT_WAIT};
  BpStatus m_next_tx_status{BpStatus::UNKNOWN};
  TaskOnTime m_update_status_scheduler;
  SignatureRequester m_signature_requester;
  std::deque<BpJobStatus> m_bpjob_sequence;

  Storage *m_storage;
  CertificatePool *m_cert_pool;

  std::map<id_type, std::string> m_cert_map;

  std::once_flag m_timer_once_flag;
};
} // namespace gruut
#endif