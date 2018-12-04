#ifndef GRUUT_ENTERPRISE_MERGER_APPLICATION_HPP
#define GRUUT_ENTERPRISE_MERGER_APPLICATION_HPP

#include "modules/module.hpp"
#include "services/signer_pool_manager.hpp"
#include "services/transaction_collector.hpp"
#include "services/transaction_pool.hpp"

#include <boost/asio.hpp>
#include <list>
#include <queue>
#include <thread>
#include <tuple>
#include <vector>

#include "chain/message.hpp"
#include "chain/signature.cpp"
#include "chain/transaction.hpp"

using namespace std;

namespace gruut {
using InputQueue = shared_ptr<queue<InputMessage>>;
using OutputQueue = shared_ptr<queue<OutputMessage>>;
using SignaturePool = vector<Signature>;

class Application {
public:
  static Application &app() {
    static Application application;
    return application;
  }

  Application(Application const &) = delete;

  Application operator=(Application const &) = delete;

  boost::asio::io_service &getIoService();

  InputQueue &getInputQueue();

  OutputQueue &getOutputQueue();

  SignerPool &getSignerPool();

  SignerPoolManager &getSignerPoolManager();

  TransactionPool &getTransactionPool();

  TransactionCollector &getTransactionCollector();

  SignaturePool &getSignaturePool();

  void start(const vector<shared_ptr<Module>> &modules);

  void exec();

  void quit();

private:
  shared_ptr<boost::asio::io_service> m_io_serv;
  InputQueue m_input_queue;
  OutputQueue m_output_queue;
  shared_ptr<SignerPool> m_signer_pool;
  shared_ptr<SignerPoolManager> m_signer_pool_manager;
  shared_ptr<TransactionPool> m_transaction_pool;
  shared_ptr<TransactionCollector> m_transaction_collector;
  shared_ptr<SignaturePool> m_signature_pool;

  shared_ptr<std::vector<std::thread>> m_thread_group;
  Application();

  ~Application() {}
};
} // namespace gruut
#endif
