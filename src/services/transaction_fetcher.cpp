#include <algorithm>

#include "../utils/sha_256.hpp"
#include "transaction_fetcher.hpp"
#include "../application.hpp"
#include "../../include/nlohmann/json.hpp"

namespace gruut {
    using json = nlohmann::json;

    TransactionFetcher::TransactionFetcher(Signers &&signers) {
        m_signers = signers;
        auto& transaction_pool = Application::app().getTransactionPool();

        const int transaciton_size = transaction_pool.size();
        auto t_size = std::min(transaciton_size, MAX_COLLECT_TRANSACTION_SIZE);

        m_selected_transaction_list = Transactions(transaction_pool.cbegin(), transaction_pool.cbegin() + t_size);
    }

    Transactions TransactionFetcher::fetchAll() {
        Transactions transactions;

        for_each(m_signers.begin(), m_signers.end(), [this, &transactions](Signer &signer) {
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

            // TODO: 유일하게 tx를 식별할 수 있는 transaction_id 결정
            new_transaction.transaction_id = sent_time;
            new_transaction.transaction_type = TransactionType::CERTIFICATE;

            // TODO: requestor_id <- Merger Id, 임시로 sent_time
            new_transaction.requestor_id = sent_time;

            // TODO: Merger의 signature, 임시로 sent_time
            new_transaction.signature = Sha256::encrypt(sent_time);

            new_transaction.sent_time = sent_time;

            json j;
            // TOOD: 공증요청한 client의 id를 넣어야 함, 임시로 트랜잭션 requestor_id
            j["requestor_id"] = new_transaction.requestor_id;
            j["sent_time"] = new_transaction.sent_time;
            // TOOD: 임시로 sent_time
            j["data_id"] = sent_time;
            // TODO: 공증서에 대한 내용이 들어가야 하지만, 임시로 sent_time 해싱
            j["digest"] = Sha256::encrypt(sent_time);
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
}