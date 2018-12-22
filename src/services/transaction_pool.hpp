#ifndef GRUUT_ENTERPRISE_MERGER_TRANSACTION_POOL_HPP
#define GRUUT_ENTERPRISE_MERGER_TRANSACTION_POOL_HPP

#include <list>
#include <mutex>

#include "../chain/transaction.hpp"

namespace gruut {
class TransactionPool {
public:
  void push(Transaction &transaction);
  Transaction pop();
  size_t size();
  void removeDuplicatedTransactions(std::vector<transaction_id_type> &tx_ids);

private:
  std::list<Transaction> m_transaction_pool;

  std::mutex m_mutex;
};
} // namespace gruut
#endif
