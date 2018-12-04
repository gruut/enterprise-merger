#ifndef GRUUT_ENTERPRISE_MERGER_TRANSACTION_COLLECTOR_HPP
#define GRUUT_ENTERPRISE_MERGER_TRANSACTION_COLLECTOR_HPP

#include <boost/asio.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <queue>
#include <thread>

#include "../chain/types.hpp"
#include "../modules/module.hpp"
#include "signature_requester.hpp"

namespace gruut {
const int TRANSACTION_COLLECTION_INTERVAL = 5000;

class TransactionCollector {
public:
  TransactionCollector() = default;
  void handleMessage(nlohmann::json message_body_json);

private:
  bool isRunnable();

  void startTimer();

  void startSignatureRequest();

  std::unique_ptr<boost::asio::deadline_timer> m_timer;
  SignatureRequester m_signature_requester;

  bool m_runnable = false;
  std::thread *m_worker_thread;
};
} // namespace gruut
#endif