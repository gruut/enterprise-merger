#include "application.hpp"
#include "chain/transaction.hpp"
#include "config/config.hpp"
#include "modules/module.hpp"
#include "modules/message_fetcher/message_fetcher.hpp"

namespace gruut {
boost::asio::io_service &Application::getIoService() { return *m_io_serv; }

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

//void Application::start(){
//
//  cout << "APP: start()" << endl << flush;
//
//  //regModule(m_moudle_communication);
//  //regModule(m_module_message_fetcher);
//  //regModule(m_module_out_message_fetcher);
//}

void Application::regModule(shared_ptr<Module> module){
  cout << "APP: regModule()" << endl << flush;
  m_modules.emplace_back(module);
}

void Application::regBootstraper(shared_ptr<BootStraper> bootstraper){
  cout << "APP: regBootstraper()" << endl << flush;
  m_bootstraper = bootstraper;
}

void Application::regMessageFetcher(shared_ptr<MessageFetcher> message_fetcher) {
  cout << "APP: regMessageFetcher()" << endl << flush;
  m_message_fetcher = message_fetcher;
}

void Application::start(){
  try {
    for(auto &module : m_modules) {
      module->start();
    }
    m_modules.clear();
  }
  catch (...){
    quit();
    throw ;
  }
}

void Application::exec() {

  cout << "APP: exec()" << endl << flush;

  for (auto i = 0; i < config::MAX_THREAD; i++) {
    m_thread_group->emplace_back([this]() { m_io_serv->run(); });
  }

  m_bootstraper->startSync(std::bind(&Application::runScheduler, this, std::placeholders::_1));

  for (auto &th : *m_thread_group) {
    if (th.joinable())
      th.join();
  }
}

void Application::runScheduler(int exit_code){

  cout << "APP: runScheduler()" << endl << flush;

  // TODO :: running BP scheduler and Message Fetcher
  m_message_fetcher->start();
}

void Application::quit() { m_io_serv->stop(); }

Application::Application() {

  cout << "APP: Application()" << endl << flush;

  m_io_serv = make_shared<boost::asio::io_service>();
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
