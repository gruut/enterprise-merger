#ifndef GRUUT_HANA_MERGER_TRANSACTION_FETCHER_HPP
#define GRUUT_HANA_MERGER_TRANSACTION_FETCHER_HPP

#include "../chain/signer.hpp"
#include "../chain/transaction.hpp"
#include "../utils/sha_256.hpp"
#include "../../include/nlohmann/json.hpp"

namespace gruut {
    using namespace std;

    class TransactionFetcher {
        using json = nlohmann::json;
    public:
        TransactionFetcher(Signer& signer): m_signer(signer) {}

        Transaction fetch() {
            auto sent_time = to_string(time(0));

            if(m_signer.isNew()) {
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
            }
            else {
                Transaction new_transaction;

                // TODO: 유일하게 tx를 식별할 수 있는 transaction_id 결정
                new_transaction.transaction_id = sent_time;
                new_transaction.transaction_type = TransactionType::CHECKSUM;

                // TODO: requestor_id <- Service EndPoint Id, 임시로 sent_time
                new_transaction.requestor_id = sent_time;

                // TODO: Service Endpoint의 signature, 임시로 sent_time
                new_transaction.signature = Sha256::encrypt(sent_time);

                new_transaction.sent_time = sent_time;

                json j;
                j["signer_id"] = m_signer.address;
                j["signer_certificate"] = m_signer.cert;
                new_transaction.content = j.dump();

                return new_transaction;
            }
        }

    private:
        Signer& m_signer;
    };
}
#endif
