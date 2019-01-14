#include "transaction_pool.hpp"

namespace gruut {
bool TransactionPool::push(Transaction &transaction) {
  std::lock_guard<std::mutex> guard(m_push_mutex);
  if (!isDuplicated(transaction.getId())) {
    m_transaction_pool.emplace_back(transaction);
    return true;
  }
  return false;
}

void TransactionPool::clear() {
  std::lock_guard<std::mutex> guard(m_check_mutex);
  m_transaction_pool.clear();
}

bool TransactionPool::isDuplicated(transaction_id_type &&tx_id) {
  return isDuplicated(tx_id);
}
bool TransactionPool::isDuplicated(transaction_id_type &tx_id) {
  std::lock_guard<std::mutex> guard(m_check_mutex);
  auto start_pos = m_transaction_pool.begin();
  auto end_pos = m_transaction_pool.end();
  auto find_pos = std::find_if(
      start_pos, end_pos, [&](Transaction &tx) { return tx.getId() == tx_id; });

  return (find_pos != end_pos);
}

bool TransactionPool::pop(Transaction &transaction) {
  std::lock_guard<std::mutex> guard(m_push_mutex);
  if (m_transaction_pool.empty())
    return false;

  transaction = m_transaction_pool.back();
  m_transaction_pool.pop_back();
  m_push_mutex.unlock();

  return true;
}

std::vector<Transaction> TransactionPool::fetchLastN(size_t n) {
  std::vector<Transaction> transactions;
  std::lock_guard<std::mutex> guard(m_push_mutex);
  size_t len = std::min(n, m_transaction_pool.size());
  for (size_t i = 0; i < len; ++i) {
    transactions.emplace_back(m_transaction_pool.back());
    m_transaction_pool.pop_back();
  }
  m_push_mutex.unlock();

  return transactions;
}

void TransactionPool::removeDuplicatedTransactions(
    std::vector<transaction_id_type> &&tx_ids) {
  removeDuplicatedTransactions(tx_ids);
}

void TransactionPool::removeDuplicatedTransactions(
    std::vector<transaction_id_type> &tx_ids) {
  if (tx_ids.empty())
    return;

  std::lock_guard<std::mutex> guard(m_push_mutex);
  for (auto &tx_id : tx_ids) {
    m_transaction_pool.remove_if(
        [&](Transaction &tx) { return (tx.getId() == tx_id); });
  }
  m_push_mutex.unlock();
}

size_t TransactionPool::size() { return m_transaction_pool.size(); }
} // namespace gruut
