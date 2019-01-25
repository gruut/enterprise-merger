#include "transaction_pool.hpp"

namespace gruut {
TransactionPool::TransactionPool() {
  m_transaction_pool.reserve(config::MAX_COLLECT_TRANSACTION_SIZE * 2);
}

bool TransactionPool::push(Transaction transaction) {

  std::string tx_id_str = transaction.getIdStr();
  std::lock_guard<std::mutex> lock(m_push_mutex);
  if (!m_txid_pool.has(tx_id_str)) {
    m_txid_pool.set(tx_id_str, true);
    m_transaction_pool.emplace_back(transaction);
    return true;
  }
  return false;
}

void TransactionPool::clear() {
  std::lock_guard<std::mutex> lock(m_push_mutex);
  m_txid_pool.clear();
  m_transaction_pool.clear();
  m_push_mutex.unlock();
}

bool TransactionPool::isDuplicated(tx_id_type &&tx_id) {
  return isDuplicated(tx_id);
}
bool TransactionPool::isDuplicated(tx_id_type &tx_id) {
  return m_txid_pool.has(TypeConverter::arrayToString<TRANSACTION_ID_TYPE_SIZE>(tx_id));
}

bool TransactionPool::pop(Transaction &transaction) {
  if (m_txid_pool.get_n_keys() == 0)
    return false;

  bool success = false;
  for (int i = (int)m_transaction_pool.size() - 1; i >= 0; --i) {
    std::string tx_id_str = m_transaction_pool[i].getIdStr();
    if (m_txid_pool.get_copy_or_default(tx_id_str, false)) {
      transaction = m_transaction_pool[i];
      m_txid_pool.unset(tx_id_str);
      success = true;
      break;
    }
  }

  return success;
}

std::vector<Transaction> TransactionPool::fetchLastN(size_t n) {

  std::vector<Transaction> transactions;

  for (int i = (int)m_transaction_pool.size() - 1; i >= 0; --i) {
    std::string tx_id_str = m_transaction_pool[i].getIdStr();
    if (m_txid_pool.get_copy_or_default(tx_id_str, false)) {
      transactions.emplace_back(m_transaction_pool[i]);
      m_txid_pool.unset(tx_id_str);
      if (transactions.size() >= n)
        break;
    }
  }

  return transactions;
}

void TransactionPool::removeDuplicatedTransactions(
    std::vector<tx_id_type> &&tx_ids) {
  removeDuplicatedTransactions(tx_ids);
}

void TransactionPool::removeDuplicatedTransactions(
    std::vector<tx_id_type> &tx_ids) {
  if (tx_ids.empty())
    return;

  for (auto &tx_id : tx_ids) {
    m_txid_pool.unset(
        TypeConverter::arrayToString<TRANSACTION_ID_TYPE_SIZE>(tx_id));
  }
}

size_t TransactionPool::size() { return m_txid_pool.get_n_keys(); }
} // namespace gruut
