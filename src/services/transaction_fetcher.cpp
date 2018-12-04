#include <algorithm>

#include "../../include/nlohmann/json.hpp"
#include "../application.hpp"
#include "../utils/sha256.hpp"
#include "transaction_fetcher.hpp"

namespace gruut {
using json = nlohmann::json;

TransactionFetcher::TransactionFetcher(Signers &&signers) {
  m_signers = signers;
}

Transactions TransactionFetcher::fetchAll() {
  auto &transaction_pool = Application::app().getTransactionPool();

  const int transacitons_size = static_cast<const int>(transaction_pool.size());
  auto t_size = std::min(transacitons_size, MAX_COLLECT_TRANSACTION_SIZE);

  Transactions transactions;
  for (unsigned int i = 0; i < t_size; i++) {
    transactions.emplace_back();
  }
  //  for_each(m_signers.begin(), m_signers.end(),
  //           [this, &transactions](Signer &signer) {
  //             auto transaction = fetch(signer);
  //             if (!(transaction.sent_time == "")) {
  //               transactions.push_back(transaction);
  //             }
  //           });

  return transactions;
}

Transaction TransactionFetcher::fetch(Signer &signer) { return Transaction(); }
} // namespace gruut