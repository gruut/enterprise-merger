#ifndef GRUUT_ENTERPRISE_MERGER_APPLICATION_HPP
#define GRUUT_ENTERPRISE_MERGER_APPLICATION_HPP

#include "modules/bootstraper/bootstrapper.hpp"
#include "modules/bp_scheduler/bp_scheduler.hpp"
#include "modules/communication/communication.hpp"
#include "modules/health_checker/block_health_checker.hpp"
#include "modules/message_fetcher/message_fetcher.hpp"
#include "modules/message_fetcher/out_message_fetcher.hpp"
#include "modules/module.hpp"

#include "config/config.hpp"

#include "chain/message.hpp"
#include "chain/signature.hpp"
#include "chain/transaction.hpp"

#include "ledger/certificate_ledger.hpp"
#include "ledger/digest_ledger.hpp"
#include "ledger/ledger.hpp"
#include "ledger/sms_ledger.hpp"
#include "services/block_processor.hpp"
#include "services/custom_ledger_manager.hpp"
#include "services/setting.hpp"
#include "services/signature_pool.hpp"
#include "services/signer_pool_manager.hpp"
#include "services/transaction_collector.hpp"
#include "services/transaction_pool.hpp"

#include <boost/asio.hpp>

#include <list>
#include <queue>
#include <thread>
#include <tuple>
#include <vector>

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

  SignerPoolManager &getSignerPoolManager();

  TransactionPool &getTransactionPool();

  TransactionCollector &getTransactionCollector();

  SignaturePool &getSignaturePool();

  BlockProcessor &getBlockProcessor();

  BpScheduler &getBpScheduler();

  CustomLedgerManager &getCustomLedgerManager();

  CertificateLedger &getCertificateLedger();

  void setup();
  void start();
  void exec();
  void quit();

private:
  void registerModule(shared_ptr<Module> module, int stage,
                      bool runover_flag = false);
  void runNextStage(ExitCode exit_code);

  shared_ptr<boost::asio::io_service> m_io_serv;
  shared_ptr<SignerPoolManager> m_signer_pool_manager;
  shared_ptr<TransactionPool> m_transaction_pool;
  shared_ptr<TransactionCollector> m_transaction_collector;
  shared_ptr<SignaturePool> m_signature_pool;

  shared_ptr<std::vector<std::thread>> m_thread_group;

  std::vector<std::vector<shared_ptr<Module>>> m_modules;

  shared_ptr<BpScheduler> m_bp_scheduler;
  shared_ptr<BlockProcessor> m_block_processor;
  shared_ptr<Communication> m_communication;
  shared_ptr<OutMessageFetcher> m_out_message_fetcher;
  shared_ptr<Bootstrapper> m_bootstraper;
  shared_ptr<BlockHealthChecker> m_block_health_checker;
  shared_ptr<MessageFetcher> m_message_fetcher;

  shared_ptr<CustomLedgerManager> m_custom_ledger_manager;
  shared_ptr<CertificateLedger> m_certificate_ledger;
  shared_ptr<DigestLedger> m_digest_ledger;
  shared_ptr<SmsLedger> m_sms_ledger;

  int m_running_stage{0};

  Application();

  ~Application() {}
};
} // namespace gruut
#endif
