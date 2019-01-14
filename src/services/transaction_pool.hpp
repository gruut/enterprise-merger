#ifndef GRUUT_ENTERPRISE_MERGER_TRANSACTION_POOL_HPP
#define GRUUT_ENTERPRISE_MERGER_TRANSACTION_POOL_HPP

#include "../chain/transaction.hpp"

#include <algorithm>
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

  std::mutex m_push_mutex;
  std::mutex m_check_mutex;
};
} // namespace gruut
#endif
