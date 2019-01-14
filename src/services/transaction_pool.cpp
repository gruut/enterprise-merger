#include "transaction_pool.hpp"

namespace gruut {
bool TransactionPool::push(Transaction &transaction) {
  std::lock_guard<std::mutex> guard(m_push_mutex);
  if (!isDuplicated(transaction.getId())) {
    m_transaction_pool.emplace_back(transaction);
    m_txid_pool.set(TypeConverter::encodeBase64(transaction.getId()), 0);
    return true;
  }
  return false;
}

void TransactionPool::clear() {
  m_transaction_pool.clear();
  m_txid_pool.clear();
}

bool TransactionPool::isDuplicated(transaction_id_type &&tx_id) {
  return isDuplicated(tx_id);
}
bool TransactionPool::isDuplicated(transaction_id_type &tx_id) {
  return m_txid_pool.has(TypeConverter::encodeBase64(tx_id));
}

bool TransactionPool::pop(Transaction &transaction) {
  if (m_txid_pool.get_n_keys() == 0)
    return false;

  std::lock_guard<std::mutex> guard(m_push_mutex);
  transaction = m_transaction_pool.back();
  m_transaction_pool.pop_back();
  m_push_mutex.unlock();

  m_txid_pool.unset(TypeConverter::encodeBase64(transaction.getId()));

  return true;
}

std::vector<Transaction> TransactionPool::fetchLastN(size_t n) {
  std::vector<Transaction> transactions;
  std::lock_guard<std::mutex> guard(m_push_mutex);
  size_t len = std::min(n, m_txid_pool.get_n_keys());
  for (size_t i = 0; i < len; ++i) {
    Transaction transaction = m_transaction_pool.back();
    transactions.emplace_back(transaction);
    m_transaction_pool.pop_back();
    m_txid_pool.unset(TypeConverter::encodeBase64(transaction.getId()));
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
    m_txid_pool.unset(TypeConverter::encodeBase64(tx_id));
    m_transaction_pool.remove_if(
        [&](Transaction &tx) { return (tx.getId() == tx_id); });
  }
  m_push_mutex.unlock();
}

size_t TransactionPool::size() { return m_txid_pool.get_n_keys(); }
} // namespace gruut
