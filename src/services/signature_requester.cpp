#include <boost/system/error_code.hpp>
#include <iostream>

#include "../application.hpp"
#include "../chain/signer.hpp"
#include "../chain/types.hpp"
#include "block_generator.hpp"
#include "message_factory.hpp"
#include "signature_requester.hpp"
#include "transaction_fetcher.hpp"

namespace gruut {
SignatureRequester::SignatureRequester() {
  m_timer.reset(
      new boost::asio::deadline_timer(Application::app().getIoService()));
}

bool SignatureRequester::requestSignatures() {
  auto transactions = std::move(fetchTransactions());
  auto partial_block = makePartialBlock(transactions);
  auto message = makeMessage(partial_block);

  Application::app().getOutputQueue()->push(message);
  startSignatureCollectTimer(transactions);
  return true;
}

void SignatureRequester::startSignatureCollectTimer(
    Transactions &transactions) {
  m_timer->expires_from_now(
      boost::posix_time::milliseconds(SIGNATURE_COLLECTION_INTERVAL));
  m_timer->async_wait([this](const boost::system::error_code &ec) {
    if (ec == boost::asio::error::operation_aborted) {
      m_runnable = false;
      std::cout
          << "startSignatureCollectTimer: Timer was cancelled or retriggered."
          << std::endl;
    } else if (ec.value() == 0) {
      m_runnable = false;
      auto &signature_pool = Application::app().getSignaturePool();
      if (!signature_pool.empty() &&
          signature_pool.size() == SIGNATURE_COLLECT_SIZE) {
        std::cout << "CREATE BLOCK!" << std::endl;
        BlockGenerator generator;

        //                    generator.generateBlock();
      }
    } else {
      m_runnable = false;
      std::cout << "ERROR: " << ec.message() << std::endl;
      this->m_signature_check_thread->join();
      throw;
    }

    this->m_signature_check_thread->join();
  });

  m_runnable = true;
  m_signature_check_thread = new thread([this]() {
    while (this->m_runnable) {
      auto &signature_pool = Application::app().getSignaturePool();
      // TODO: SignaturePool에 sig가 들어올때 주석 해제할 것
      //                if(signature_pool.size() > SIGNATURE_COLLECT_SIZE)
      m_runnable = false;
    }
  });
}

Transactions SignatureRequester::fetchTransactions() {
  vector<Signer> &&signers =
      Application::app().getSignerPoolManager().getSigners();
  TransactionFetcher transaction_fetcher{move(signers)};

  return transaction_fetcher.fetchAll();
}

PartialBlock SignatureRequester::makePartialBlock(Transactions &transactions) {
  BlockGenerator block_generator;
  vector<sha256> transaction_ids;

  transaction_ids.reserve(transactions.size());
  for (auto &transaction : transactions)
    transaction_ids.emplace_back(transaction.transaction_id);

  auto root_id = sha256();
  auto &&block = block_generator.generatePartialBlock(root_id);

  return block;
}

Message SignatureRequester::makeMessage(PartialBlock &block) {
  auto message = MessageFactory::createSigRequestMessage(block);

  return message;
}
} // namespace gruut