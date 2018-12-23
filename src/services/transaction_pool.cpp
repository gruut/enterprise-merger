#include "transaction_pool.hpp"

namespace gruut {
bool TransactionPool::push(Transaction &transaction) {
  std::lock_guard<std::mutex> guard(m_mutex);
  if (!isDuplicated(transaction.transaction_id)) {
    m_transaction_pool.emplace_back(transaction);
    return true;
  }
  return false;
}

bool TransactionPool::isDuplicated(transaction_id_type &tx_id) {
  return (m_transaction_pool.end() !=
          std::find_if(
              m_transaction_pool.begin(), m_transaction_pool.end(),
              [&](Transaction &tx) { return tx.transaction_id == tx_id; }));
}

Transaction TransactionPool::pop() {
  std::lock_guard<std::mutex> guard(m_mutex);
  auto transaction = m_transaction_pool.front();
  m_transaction_pool.pop_front();
  m_mutex.unlock();

  return transaction;
}

void TransactionPool::removeDuplicatedTransactions(
    std::vector<transaction_id_type> &tx_ids) {
  if (tx_ids.empty())
    return;

  std::lock_guard<std::mutex> guard(m_mutex);
  for (auto &tx_id : tx_ids) {
    m_transaction_pool.remove_if(
        [&](Transaction &tx) { return (tx.transaction_id == tx_id); });
  }
  m_mutex.unlock();
}

size_t TransactionPool::size() { return m_transaction_pool.size(); }
} // namespace gruut
