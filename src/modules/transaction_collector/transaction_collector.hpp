#ifndef GRUUT_ENTERPRISE_MERGER_TRANSACTION_COLLECTOR_HPP
#define GRUUT_ENTERPRISE_MERGER_TRANSACTION_COLLECTOR_HPP

#include <boost/asio.hpp>
#include <memory>
#include <queue>
#include <thread>

#include "../../modules/module.hpp"
#include "../../services/signature_requester.hpp"

namespace gruut {
const int TRANSACTION_COLLECTION_INTERVAL = 5000;

class TransactionCollector : public Module {
public:
  TransactionCollector();
  void start();

private:
  bool isRunnable();

  void startTimer();

  void startSignatureRequest();

  std::unique_ptr<boost::asio::deadline_timer> m_timer;
  std::shared_ptr<SignatureRequester> m_signature_requester;

  bool m_runnable = false;
  std::thread *m_worker_thread;
};
}
#endif