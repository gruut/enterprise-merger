#ifndef GRUUT_ENTERPRISE_MERGER_TRANSACTION_COLLECTOR_HPP
#define GRUUT_ENTERPRISE_MERGER_TRANSACTION_COLLECTOR_HPP

#include <boost/asio.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <queue>

#include "../chain/types.hpp"
#include "../modules/module.hpp"
#include "signature_requester.hpp"

namespace gruut {
const int TRANSACTION_COLLECTION_INTERVAL = 5000;

class TransactionCollector {
public:
  TransactionCollector();
  void handleMessage(nlohmann::json message_body_json);

private:
  bool isRunnable();

  void startTimer();

  void startSignatureRequest();

  std::unique_ptr<boost::asio::deadline_timer> m_timer;
  std::shared_ptr<SignatureRequester> m_signature_requester;

  bool m_timer_running = false;
};
} // namespace gruut
#endif