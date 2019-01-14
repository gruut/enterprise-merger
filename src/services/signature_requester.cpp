#include "signature_requester.hpp"
#include "../application.hpp"
#include "easy_logging.hpp"

namespace gruut {

SignatureRequester::SignatureRequester() {
  auto &io_service = Application::app().getIoService();
  m_check_timer.reset(new boost::asio::deadline_timer(io_service));
  m_collect_timer.reset(new boost::asio::deadline_timer(io_service));
  m_block_gen_strand.reset(new boost::asio::io_service::strand(io_service));
  el::Loggers::getLogger("SIGR");
}

void SignatureRequester::waitCollectDone() {
  if (!m_is_collect_timer_running)
    return;

  if (Application::app().getSignaturePool().size() >= m_max_signers) {
    doCreateBlock();
  }

  m_check_timer->expires_from_now(boost::posix_time::milliseconds(
      config::SIGNATURE_COLLECTION_CHECK_INTERVAL));
  m_check_timer->async_wait([this](const boost::system::error_code &ec) {
    if (ec == boost::asio::error::operation_aborted) {
      CLOG(INFO, "SIGR") << "CheckTimer ABORTED";
    } else if (ec.value() == 0) {
      waitCollectDone();
    } else {
      CLOG(ERROR, "SIGR") << ec.message();
      // throw;
    }
  });
}
void SignatureRequester::requestSignatures() {

  // step 1 - chooosing suitable signers

  auto target_signers = selectSigners();

  if (target_signers.empty()) {
    CLOG(ERROR, "SIGR") << "No signer";
    return;
  }

  std::vector<Signer> new_signers;
  for (auto &signer : target_signers) {
    if (signer.isNew()) {
      new_signers.emplace_back(signer);
    }
  }

  auto &transaction_pool = Application::app().getTransactionPool();

  if (!new_signers.empty()) {
    Transaction new_transaction = generateCertificateTransaction(new_signers);
    transaction_pool.push(new_transaction);
  }

  m_max_signers = target_signers.size();

  // step 2 - fetching transactions and making partial block

  Transactions transactions = transaction_pool.fetchLastN(
      std::min(transaction_pool.size(), config::MAX_COLLECT_TRANSACTION_SIZE));
  transaction_pool.clear();

  m_merkle_tree.generate(transactions);

  m_basic_block_info = generateBasicBlockInfo();
  m_basic_block_info.transaction_root = m_merkle_tree.getMerkleTree().back();
  m_basic_block_info.transactions = std::move(transactions);

  // step 3 - setup SignaturePool

  Application::app().getSignaturePool().setupSigPool(
      m_basic_block_info.height, m_basic_block_info.time,
      m_basic_block_info.transaction_root); // auto enable pool

  // step 4 - collect signatures

  sendRequestMessage(target_signers);
  startSignatureCollectTimer();
  waitCollectDone();
}

void SignatureRequester::doCreateBlock() {

  CLOG(INFO, "SIGR") << "START MAKING BLOCK";

  m_is_collect_timer_running = false;

  m_collect_timer->cancel();
  m_check_timer->cancel();

  Application::app().getSignaturePool().disablePool(); // disable pool now!

  auto &io_service = Application::app().getIoService();

  io_service.post(m_block_gen_strand->wrap([this]() { // should work one-by-one
    auto &signature_pool = Application::app().getSignaturePool();

    if (signature_pool.size() >= config::MIN_SIGNATURE_COLLECT_SIZE &&
        signature_pool.size() <= config::MAX_SIGNATURE_COLLECT_SIZE) {

      auto signatures_size =
          min(signature_pool.size(), config::MAX_SIGNATURE_COLLECT_SIZE);
      auto signatures =
          signature_pool.fetchN(signatures_size, m_basic_block_info.height);
      signature_pool.clear(); // last signatures are useless.

      BlockGenerator generator;
      generator.generateBlock(m_basic_block_info, signatures, m_merkle_tree);
    } else {
      CLOG(ERROR, "SIGR") << "CANCEL MAKING BLOCK";
      signature_pool.clear();
    }
  }));
}

void SignatureRequester::startSignatureCollectTimer() {
  m_is_collect_timer_running = true;

  m_collect_timer->expires_from_now(
      boost::posix_time::milliseconds(config::SIGNATURE_COLLECTION_INTERVAL));
  m_collect_timer->async_wait([this](const boost::system::error_code &ec) {
    if (ec == boost::asio::error::operation_aborted) {
      CLOG(INFO, "SIGR") << "SigCollectTimer ABORTED";
    } else if (ec.value() == 0) {
      if (m_is_collect_timer_running)
        doCreateBlock();
    } else {
      CLOG(ERROR, "SIGR") << ec.message();
      // throw;
    }
  });
}

void SignatureRequester::sendRequestMessage(Signers &signers) {
  if (signers.empty()) {
    CLOG(ERROR, "SIGR") << "No signer";
    return;
  }

  vector<id_type> receivers_list;
  for_each(signers.begin(), signers.end(), [&receivers_list](Signer signer) {
    receivers_list.emplace_back(signer.user_id);
  });

  OutputMsgEntry output_message;
  output_message.type = MessageType::MSG_REQ_SSIG;
  output_message.body["time"] = Time::now();
  output_message.body["mID"] =
      TypeConverter::encodeBase64(m_basic_block_info.merger_id);
  output_message.body["cID"] =
      TypeConverter::encodeBase64(m_basic_block_info.chain_id);
  output_message.body["txrt"] =
      TypeConverter::encodeBase64(m_basic_block_info.transaction_root);
  output_message.body["hgt"] = m_basic_block_info.height;
  output_message.receivers = receivers_list;

  MessageProxy proxy;
  proxy.deliverOutputMessage(output_message);
}

Signers SignatureRequester::selectSigners() {
  Signers selected_signers;

  auto signer_pool = SignerPool::getInstance();
  size_t num_available_signers =
      signer_pool->getNumSignerBy(SignerStatus::GOOD);
  if (num_available_signers >= config::MIN_SIGNATURE_COLLECT_SIZE) {
    selected_signers = signer_pool->getRandomSigners(
        std::min(config::MAX_SIGNATURE_COLLECT_SIZE, num_available_signers));
  }

  return selected_signers;
}

Transaction
SignatureRequester::generateCertificateTransaction(vector<Signer> &signers) {
  Transaction new_transaction;

  if (!signers.empty()) {

    auto setting = Setting::getInstance();

    //    new_transaction.setId(generateTxId());
    new_transaction.setTime(static_cast<timestamp_type>(Time::now_int()));
    new_transaction.setRequestorId(setting->getMyId());
    new_transaction.setTransactionType(TransactionType::CERTIFICATES);

    std::vector<content_type> content_list;
    for (auto &signer : signers) {
      if (signer.isNew()) {
        auto user_id_str = TypeConverter::encodeBase64(signer.user_id);
        content_list.emplace_back(user_id_str);
        content_list.emplace_back(signer.pk_cert);
      }
    }

    new_transaction.setContents(content_list);

    new_transaction.genNewTxId();

    new_transaction.refreshSignature(setting->getMySK(), setting->getMyPass());
  }

  return new_transaction;
}

BasicBlockInfo SignatureRequester::generateBasicBlockInfo() {

  auto setting = Setting::getInstance();

  auto storage = Storage::getInstance();
  auto latest_block_info = storage->getNthBlockLinkInfo();

  BasicBlockInfo partial_block;

  partial_block.time = Time::now_int();
  partial_block.merger_id = setting->getMyId();
  partial_block.chain_id = setting->getLocalChainId();

  if (latest_block_info.height == 0)
    partial_block.height = 1; // this is genesis block
  else
    partial_block.height = latest_block_info.height + 1;

  return partial_block;
}
} // namespace gruut
