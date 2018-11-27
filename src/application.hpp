#ifndef GRUUT_ENTERPRISE_MERGER_APPLICATION_HPP
#define GRUUT_ENTERPRISE_MERGER_APPLICATION_HPP

#include "modules/module.hpp"
#include "services/signer_pool_manager.hpp"
#include <boost/asio.hpp>
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
using TransactionPool = vector<Transaction>;
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

  SignerPoolManager &getSignerPoolManager();

  TransactionPool &getTransactionPool();

  SignaturePool &getSignaturePool();

  void start(const vector<shared_ptr<Module>> &modules);

  void exec();

  void quit();

private:
  shared_ptr<boost::asio::io_service> m_io_serv;
  InputQueue m_input_queue;
  OutputQueue m_output_queue;
  shared_ptr<gruut::SignerPoolManager> m_signer_pool_manager;
  shared_ptr<TransactionPool> m_transaction_pool;
  shared_ptr<SignaturePool> m_signature_pool;

  shared_ptr<std::vector<std::thread>> m_thread_group;
  Application();

  ~Application() {}
};
} // namespace gruut
#endif
