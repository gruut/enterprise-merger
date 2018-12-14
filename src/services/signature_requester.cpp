#include <boost/system/error_code.hpp>
#include <iostream>
#include <random>

#include "../application.hpp"
#include "../chain/signer.hpp"
#include "../chain/types.hpp"
#include "../config/config.hpp"
#include "block_generator.hpp"
#include "message_factory.hpp"
#include "message_proxy.hpp"
#include "signature_requester.hpp"
#include "signer_pool.hpp"

using namespace gruut::config;

namespace gruut {
void SignatureRequester::requestSignatures() {
  auto signers = selectSigners();

  auto transactions = std::move(fetchTransactions());

  auto partial_block = makePartialBlock(transactions);
  Application::app().getTemporaryPartialBlock() = partial_block;

  requestSignature(partial_block, signers);
  startSignatureCollectTimer(transactions);
}

void SignatureRequester::startSignatureCollectTimer(
    Transactions &transactions) {
  m_timer.reset(
      new boost::asio::deadline_timer(Application::app().getIoService()));
  m_timer->expires_from_now(
      boost::posix_time::milliseconds(SIGNATURE_COLLECTION_INTERVAL));
  m_timer->async_wait([&](const boost::system::error_code &ec) {
    if (ec == boost::asio::error::operation_aborted) {
      std::cout
          << "startSignatureCollectTimer: Timer was cancelled or retriggered."
          << std::endl;
    } else if (ec.value() == 0) {
      auto &signature_pool = Application::app().getSignaturePool();
      if (signature_pool.size() >= MIN_SIGNATURE_COLLECT_SIZE &&
          signature_pool.size() <= MAX_SIGNATURE_COLLECT_SIZE) {
        cout << "SIG POOL SIZE: " << signature_pool.size() << endl;
        std::cout << "CREATE BLOCK!" << std::endl;

        auto temp_partial_block = Application::app().getTemporaryPartialBlock();

        auto signatures_size =
            min(signature_pool.size(), MAX_SIGNATURE_COLLECT_SIZE);
        auto signatures = signature_pool.fetchN(signatures_size);

        BlockGenerator generator;
        Block block = generator.generateBlock(temp_partial_block, signatures,
                                              m_merkle_tree);
      }
    } else {
      std::cout << "ERROR: " << ec.message() << std::endl;
      throw;
    }
  });
}

Transactions SignatureRequester::fetchTransactions() {
  auto &transaction_pool = Application::app().getTransactionPool();

  const size_t transactions_size = transaction_pool.size();
  auto t_size = std::min(transactions_size, MAX_COLLECT_TRANSACTION_SIZE);

  Transactions transactions_list;
  for (unsigned int i = 0; i < t_size; i++) {
    auto transaction = transaction_pool.pop();
    transactions_list.emplace_back(transaction);
  }

  return transactions_list;
}

PartialBlock SignatureRequester::makePartialBlock(Transactions &transactions) {
  BlockGenerator block_generator;

  m_merkle_tree.generate(transactions);
  vector<sha256> merkle_tree_vector = m_merkle_tree.getMerkleTree();
  auto &&block =
      block_generator.generatePartialBlock(merkle_tree_vector, transactions);

  return block;
}

void SignatureRequester::requestSignature(PartialBlock &block,
                                          Signers &signers) {
  if (signers.size() > 0) {
    auto message = MessageFactory::createSigRequestMessage(block, signers);
    MessageProxy proxy;
    proxy.deliverOutputMessage(message);
  }
}

Signers SignatureRequester::selectSigners() {
  Signers selected_signers;

  auto &signer_pool = Application::app().getSignerPool();
  auto signer_pool_size = signer_pool.size();

  if (signer_pool_size > 0) {
    auto req_signers_num =
        std::min(config::REQ_SSIG_SIGNERS_NUM, signer_pool_size);
    selected_signers = signer_pool.getRandomSigners(req_signers_num);
  }

  return selected_signers;
}
} // namespace gruut