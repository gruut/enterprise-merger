#include "application.hpp"
#include "easy_logging.hpp"

namespace gruut {
boost::asio::io_service &Application::getIoService() { return *m_io_serv; }

BpScheduler &Application::getBpScheduler() { return *m_bp_scheduler; }

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

BlockProcessor &Application::getBlockProcessor() { return *m_block_processor; }

CustomLedgerManager &Application::getCustomLedgerManager() {
  return *m_custom_ledger_manager;
}

void Application::registerModule(shared_ptr<Module> module, int stage,
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

  if (m_modules[m_running_stage].empty()) {
    runNextStage(ExitCode::ERROR_SKIP_STAGE);
    return;
  }

  CLOG(INFO, "_APP") << "STAGE #" << m_running_stage << " ---- START";

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

void Application::runNextStage(ExitCode exit_code) {

  if (exit_code == ExitCode::ERROR_ABORT) {
    quit();
  }

  CLOG(INFO, "_APP") << "STAGE #" << m_running_stage
                     << " ---- END (exit=" << (int)exit_code << ")";
  if (m_running_stage < m_modules.size()) {
    ++m_running_stage;
    start();
  }
}

void Application::quit() { m_io_serv->stop(); }

void Application::setup() {

  // step 1 - making objects
  m_signer_pool_manager = make_shared<SignerPoolManager>();
  m_transaction_pool = make_shared<TransactionPool>();
  m_transaction_collector = make_shared<TransactionCollector>();
  m_signature_pool = make_shared<SignaturePool>();
  m_thread_group = make_shared<std::vector<std::thread>>();

  m_bp_scheduler = make_shared<BpScheduler>();
  m_custom_ledger_manager = make_shared<CustomLedgerManager>();
  m_block_processor = make_shared<BlockProcessor>();
  m_bootstraper = make_shared<Bootstrapper>();
  m_block_health_checker = make_shared<BlockHealthChecker>();
  m_communication = make_shared<Communication>();
  m_out_message_fetcher = make_shared<OutMessageFetcher>();
  m_message_fetcher = make_shared<MessageFetcher>();

  // step 2 - running scenario
  if (Setting::getInstance()->getDBCheck())
    registerModule(m_block_health_checker, 0, true);

  registerModule(m_block_processor, 1);
  registerModule(m_communication, 1, true);

  registerModule(m_out_message_fetcher, 2);
  registerModule(m_bootstraper, 2, true);

  registerModule(m_message_fetcher, 3);
  registerModule(m_bp_scheduler, 3);

  // setp 3 - link services

  m_bootstraper->setCommunication(m_communication);
}

Application::Application() {
  m_io_serv = make_shared<boost::asio::io_service>();
  el::Loggers::getLogger("_APP");
}

} // namespace gruut
