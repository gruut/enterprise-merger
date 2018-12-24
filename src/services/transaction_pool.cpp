#include "transaction_pool.hpp"

namespace gruut {
bool TransactionPool::push(Transaction &transaction) {
  std::lock_guard<std::mutex> guard(m_mutex);
  if (!isDuplicated(transaction.getId())) {
    m_transaction_pool.emplace_back(transaction);
    return true;
  }
  return false;
}

bool TransactionPool::isDuplicated(transaction_id_type &&tx_id) {
  return isDuplicated(tx_id);
}
bool TransactionPool::isDuplicated(transaction_id_type &tx_id) {
  //  return (m_transaction_pool.end() !=
  //          std::find_if(m_transaction_pool.begin(), m_transaction_pool.end(),
  //                       [&](Transaction &tx) { return tx.getId() == tx_id;
  //                       }));

  return false;
}

bool TransactionPool::pop(Transaction &transaction) {
  std::lock_guard<std::mutex> guard(m_mutex);
  if (m_transaction_pool.empty())
    return false;

  transaction = m_transaction_pool.back();
  m_transaction_pool.pop_back();
  m_mutex.unlock();

  return true;
}

std::vector<Transaction> TransactionPool::fetchLastN(size_t n) {
  std::vector<Transaction> transactions;
  std::lock_guard<std::mutex> guard(m_mutex);
  size_t len = std::min(n, m_transaction_pool.size());
  for (size_t i = 0; i < len; ++i) {
    transactions.emplace_back(m_transaction_pool.back());
    m_transaction_pool.pop_back();
  }
  m_mutex.unlock();

  return transactions;
}

void TransactionPool::removeDuplicatedTransactions(
    std::vector<transaction_id_type> &tx_ids) {
  if (tx_ids.empty())
    return;

  std::lock_guard<std::mutex> guard(m_mutex);
  for (auto &tx_id : tx_ids) {
    m_transaction_pool.remove_if(
        [&](Transaction &tx) { return (tx.getId() == tx_id); });
  }
  m_mutex.unlock();
}

size_t TransactionPool::size() { return m_transaction_pool.size(); }
} // namespace gruut
