#ifndef GRUUT_ENTERPRISE_MERGER_APPLICATION_HPP
#define GRUUT_ENTERPRISE_MERGER_APPLICATION_HPP

#include "modules/bootstraper/boot_straper.hpp"
#include "modules/bp_scheduler/bp_scheduler.hpp"
#include "modules/communication/communication.hpp"
#include "modules/message_fetcher/message_fetcher.hpp"
#include "modules/message_fetcher/out_message_fetcher.hpp"
#include "modules/module.hpp"
#include "services/setting.hpp"
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
#include "chain/signature.hpp"
#include "chain/transaction.hpp"
#include "services/signature_pool.hpp"

using namespace std;

namespace gruut {
class Application {
public:
  static Application &app() {
    static Application application;
    return application;
  }

  Application(Application const &) = delete;

  Application operator=(Application const &) = delete;

  boost::asio::io_service &getIoService();

  SignerPool &getSignerPool();

  SignerPoolManager &getSignerPoolManager();

  TransactionPool &getTransactionPool();

  TransactionCollector &getTransactionCollector();

  SignaturePool &getSignaturePool();

  PartialBlock &getTemporaryPartialBlock();

  void regModule(shared_ptr<Module> module, int stage,
                 bool runover_flag = false);
  void start();

  BpScheduler &getBpScheduler();

  void exec();
  void quit();

private:
  void runNextStage(ExitCode exit_code);

  shared_ptr<boost::asio::io_service> m_io_serv;
  PartialBlock temporary_partial_block;
  shared_ptr<SignerPool> m_signer_pool;
  shared_ptr<SignerPoolManager> m_signer_pool_manager;
  shared_ptr<TransactionPool> m_transaction_pool;
  shared_ptr<TransactionCollector> m_transaction_collector;
  shared_ptr<SignaturePool> m_signature_pool;

  shared_ptr<std::vector<std::thread>> m_thread_group;

  std::vector<std::vector<shared_ptr<Module>>> m_modules;

  shared_ptr<BpScheduler> m_bp_scheduler;

  int m_running_stage{0};

  Application();

  ~Application() {}
};
} // namespace gruut
#endif
