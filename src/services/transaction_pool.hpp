#ifndef GRUUT_ENTERPRISE_MERGER_TRANSACTION_POOL_HPP
#define GRUUT_ENTERPRISE_MERGER_TRANSACTION_POOL_HPP

#include "../chain/transaction.hpp"
#include "../config/config.hpp"
#include "omp_hash_map.h"

#include <algorithm>
#include <atomic>
#include <list>
#include <mutex>

namespace gruut {
class TransactionPool {
public:
  TransactionPool();
  bool push(Transaction &transaction);
  bool isDuplicated(tx_id_type &&tx_id);
  bool isDuplicated(tx_id_type &tx_id);
  bool pop(Transaction &transaction);
  size_t size();
  void removeDuplicatedTransactions(std::vector<tx_id_type> &tx_ids);
  void removeDuplicatedTransactions(std::vector<tx_id_type> &&tx_ids);
  std::vector<Transaction> fetchLastN(size_t n);
  void clear();

private:
  std::vector<Transaction> m_transaction_pool;
  omp_hash_map<std::string, bool> m_txid_pool;

  std::mutex m_push_mutex;
};
} // namespace gruut
#endif
