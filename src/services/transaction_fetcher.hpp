#ifndef GRUUT_ENTERPRISE_MERGER_TRANSACTION_FETCHER_HPP
#define GRUUT_ENTERPRISE_MERGER_TRANSACTION_FETCHER_HPP

#include "../chain/signer.hpp"
#include "../chain/transaction.hpp"
#include "../chain/types.hpp"
#include "../services/signer_pool.hpp"
#include <vector>

namespace gruut {
using Transactions = std::vector<Transaction>;
using Signers = std::vector<Signer>;

constexpr int MAX_COLLECT_TRANSACTION_SIZE = 4096;

class TransactionFetcher {
public:
  TransactionFetcher(Signers &&signers);
  TransactionFetcher(Signers &signers) = delete;
  Transactions fetchAll();

private:
  Transaction fetch(Signer &signer);
  transaction_id_type generateTransactionId();

  Transactions m_selected_transaction_list;
  Signers m_signers;
};
} // namespace gruut
#endif
