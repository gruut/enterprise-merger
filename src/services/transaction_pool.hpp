#ifndef GRUUT_ENTERPRISE_MERGER_TRANSACTION_POOL_HPP
#define GRUUT_ENTERPRISE_MERGER_TRANSACTION_POOL_HPP

#include "../chain/transaction.hpp"
#include "omp_hash_map.h"

#include <algorithm>
#include <atomic>
#include <list>
#include <mutex>

namespace gruut {
class TransactionPool {
public:
  bool push(Transaction &transaction);
  bool isDuplicated(transaction_id_type &&tx_id);
  bool isDuplicated(transaction_id_type &tx_id);
  bool pop(Transaction &transaction);
  size_t size();
  void removeDuplicatedTransactions(std::vector<transaction_id_type> &tx_ids);
  void removeDuplicatedTransactions(std::vector<transaction_id_type> &&tx_ids);
  std::vector<Transaction> fetchLastN(size_t n);
  void clear();

private:
  std::list<Transaction> m_transaction_pool;
  omp_hash_map<std::string, int> m_txid_pool;

  std::mutex m_push_mutex;
};
} // namespace gruut
#endif
