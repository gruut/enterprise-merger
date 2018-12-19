#include "application.hpp"
#include "chain/transaction.hpp"
#include "config/config.hpp"
#include "modules/message_fetcher/message_fetcher.hpp"
#include "modules/module.hpp"

namespace gruut {
boost::asio::io_service &Application::getIoService() { return *m_io_serv; }

SignerPool &Application::getSignerPool() { return *m_signer_pool; }

BpScheduler &Application::getBpScheduler() { return *m_bp_scheduler;}

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

PartialBlock &Application::getTemporaryPartialBlock() {
  return temporary_partial_block;
}

void Application::regModule(shared_ptr<Module> module, int stage,
                            bool runover_flag) {
  if (runover_flag)
    module->registCallBack(
        std::bind(&Application::runNextStage, this, std::placeholders::_1));

  while (stage + 1 > m_modules.size()) {
    m_modules.emplace_back();
  }

  m_modules[stage].emplace_back(module);
}

void Application::start() {

  cout << "APP: start() - stage " << m_running_stage << endl << flush;

  if (m_modules[m_running_stage].empty()) {
    runNextStage(-3);
    return;
  }

  try {
    for (auto &module : m_modules[m_running_stage]) {
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

void Application::runNextStage(int exit_code) {

  if (m_running_stage < m_modules.size()) {
    cout << "APP: runNextStage(" << exit_code << ")" << endl << flush;
    ++m_running_stage;
    start();
  } else {
    cout << "APP: runNextStage(" << exit_code << ") - No more stage" << endl
         << flush;
  }
}

void Application::quit() { m_io_serv->stop(); }

Application::Application() {

  m_bp_scheduler = make_shared<BpScheduler>();

  m_io_serv = make_shared<boost::asio::io_service>();
  m_signer_pool = make_shared<SignerPool>();
  m_signer_pool_manager = make_shared<SignerPoolManager>();
  m_transaction_pool = make_shared<TransactionPool>();
  m_transaction_collector = make_shared<TransactionCollector>();
  m_signature_pool = make_shared<SignaturePool>();
  m_thread_group = make_shared<std::vector<std::thread>>();
}

} // namespace gruut
