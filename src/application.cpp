#include "application.hpp"
#include "chain/transaction.hpp"
#include "config/config.hpp"
#include "modules/module.hpp"

namespace gruut {
boost::asio::io_service &Application::getIoService() { return *m_io_serv; }

InputQueue &Application::getInputQueue() { return m_input_queue; }

OutputQueue &Application::getOutputQueue() { return m_output_queue; }

SignerPool &Application::getSignerPool() { return *m_signer_pool; }

SignerPoolManager &Application::getSignerPoolManager() {
  return *m_signer_pool_manager;
}

TransactionPool &Application::getTransactionPool() {
  return *m_transaction_pool;
}

TransactionCollector &Application::getTransactionCollector() {
  return *m_transaction_collector;
}

SignaturePool &Application::getSignaturePool() { return *m_signature_pool; }

void Application::start(const vector<shared_ptr<Module>> &modules) {
  try {
    for (auto module : modules) {
      module->start();
    }
  } catch (...) {
    quit();
    throw;
  }
}

void Application::exec() {
  for (auto i = 0; i < config::MAX_THREAD; i++) {
    m_thread_group->emplace_back([this]() { m_io_serv->run(); });
  }

  for (auto &th : *m_thread_group) {
    if (th.joinable())
      th.join();
  }
}

void Application::quit() { m_io_serv->stop(); }

Application::Application() {
  m_io_serv = make_shared<boost::asio::io_service>();
  m_input_queue = make_shared<queue<InputMessage>>();
  m_output_queue = make_shared<queue<OutputMessage>>();
  m_signer_pool = make_shared<SignerPool>();
  m_signer_pool_manager = make_shared<SignerPoolManager>();
  m_transaction_pool = make_shared<TransactionPool>();
  m_transaction_collector = make_shared<TransactionCollector>();
  m_signature_pool = make_shared<SignaturePool>();

  m_thread_group = make_shared<std::vector<std::thread>>();
}

PartialBlock &Application::getTemporaryPartialBlock() {
  return temporary_partial_block;
}
} // namespace gruut
