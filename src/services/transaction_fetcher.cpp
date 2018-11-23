#include <algorithm>
#include <boost/random/random_device.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#include "../../include/nlohmann/json.hpp"
#include "../application.hpp"
#include "../utils/sha256.hpp"
#include "transaction_fetcher.hpp"

namespace gruut {
using json = nlohmann::json;

TransactionFetcher::TransactionFetcher(Signers &&signers) {
  m_signers = signers;
  auto &transaction_pool = Application::app().getTransactionPool();

  const int transaciton_size = transaction_pool.size();
  auto t_size = std::min(transaciton_size, MAX_COLLECT_TRANSACTION_SIZE);

  m_selected_transaction_list = Transactions(
      transaction_pool.cbegin(), transaction_pool.cbegin() + t_size);
}

Transactions TransactionFetcher::fetchAll() {
  Transactions transactions;

  for_each(m_signers.begin(), m_signers.end(),
           [this, &transactions](Signer &signer) {
             auto transaction = fetch(signer);
             if (!(transaction.transaction_id == "")) {
               transactions.push_back(transaction);
             }
           });

  return transactions;
}

Transaction TransactionFetcher::fetch(Signer &signer) {
  auto sent_time = to_string(time(0));

  if (signer.isNew()) {
    Transaction new_transaction;

    new_transaction.transaction_id = generateTransactionId();
    new_transaction.transaction_type = TransactionType::CERTIFICATE;

    // TODO: requestor_id <- Merger Id, 임시로 sent_time
    new_transaction.requestor_id = sent_time;

    // TODO: Merger의 signature, 임시로 sent_time
    new_transaction.signature = Sha256::hash(sent_time);

    new_transaction.sent_time = sent_time;

    json j;
    // TOOD: 공증요청한 client의 id를 넣어야 함, 임시로 트랜잭션 requestor_id
    j["requestor_id"] = new_transaction.requestor_id;
    j["sent_time"] = new_transaction.sent_time;
    // TOOD: 임시로 sent_time
    j["data_id"] = sent_time;
    // TODO: 공증서에 대한 내용이 들어가야 하지만, 임시로 sent_time 해싱
    j["digest"] = Sha256::hash(sent_time);
    new_transaction.content = j.dump();

    return new_transaction;
  } else {
    if (!m_selected_transaction_list.empty()) {
      auto last_transaction = *(m_selected_transaction_list.end());
      m_selected_transaction_list.pop_back();

      return last_transaction;
    } else {
      NullTransaction null_transaction;

      return null_transaction;
    }
  }
}

transaction_id_type TransactionFetcher::generateTransactionId() {
  std::string chars("abcdefghijklmnopqrstuvwxyz"
                    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                    "1234567890"
                    "!@#$%^&*()"
                    "`~-_=+[{]}\\|;:'\",<.>/? ");
  boost::random::random_device rng;
  boost::random::uniform_int_distribution<> index_dist(0, chars.size() - 1);

  stringstream s;
  for (int i = 0; i < 32; ++i) {
    s << chars[index_dist(rng)];
  }

  return s.str();
}
}