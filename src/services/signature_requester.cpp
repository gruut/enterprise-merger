#include <algorithm>
#include <boost/system/error_code.hpp>
#include <iostream>
#include <random>

#include "../application.hpp"
#include "../chain/signer.hpp"
#include "../chain/types.hpp"
#include "block_generator.hpp"
#include "message_factory.hpp"
#include "message_proxy.hpp"
#include "signature_requester.hpp"
#include "signer_pool.hpp"

namespace gruut {
constexpr unsigned int REQUEST_NUM_OF_SIGNER = 5;
constexpr unsigned int MAX_COLLECT_TRANSACTION_SIZE = 4096;

void SignatureRequester::requestSignatures() {
  auto signers = selectSigners();
  auto transactions = std::move(fetchTransactions());
  // TODO: 임시 블럭 생성할 수 있을때 주석 해제
  //    auto partial_block = makePartialBlock(transactions);
  //    auto message = makeMessage(partial_block);
  //
  //  MessageProxy proxy;
  //  proxy.deliverOutputMessage(message);
  //  startSignatureCollectTimer(transactions);
}

void SignatureRequester::startSignatureCollectTimer(
    Transactions &transactions) {
  m_timer.reset(
      new boost::asio::deadline_timer(Application::app().getIoService()));
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
  auto &transaction_pool = Application::app().getTransactionPool();

  const unsigned int transactions_size =
      static_cast<const int>(transaction_pool.size());
  auto t_size = min(transactions_size, MAX_COLLECT_TRANSACTION_SIZE);

  Transactions transactions_list;
  for (unsigned int i = 0; i < t_size; i++) {
    // TODO: Lock guard.
    auto &transaction = transaction_pool.front();
    transactions_list.emplace_back(transaction);
    transaction_pool.pop_front();
  }

  return transactions_list;
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

OutputMessage SignatureRequester::makeMessage(PartialBlock &block) {
  auto message = MessageFactory::createSigRequestMessage(block);

  return message;
}

RandomSignerIndices
SignatureRequester::generateRandomNumbers(const unsigned int size) {
  // Generate random number in range(0, size)
  mt19937 mt;
  mt.seed(random_device()());

  RandomSignerIndices number_set;
  while (number_set.size() < size) {
    uniform_int_distribution<mt19937::result_type> dist(0, size - 1);
    int random_number = static_cast<int>(dist(mt));
    number_set.insert(random_number);
  }

  return number_set;
}

Signers SignatureRequester::selectSigners() {
  Signers selected_signers;

  auto &signer_pool = Application::app().getSignerPool();
  auto signer_pool_size = signer_pool.size();
  if (signer_pool_size > 0) {
    auto chosen_signers_index_set =
        generateRandomNumbers(static_cast<unsigned int>(signer_pool.size()));

    for (auto index : chosen_signers_index_set) {
      auto signer = signer_pool.getSigner(index);
      selected_signers.emplace_back(signer);
    }
  }

  return selected_signers;
}
} // namespace gruut