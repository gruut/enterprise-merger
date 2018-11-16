#ifndef GRUUT_HANA_MERGER_SIGNATURE_REQUESTER_HPP
#define GRUUT_HANA_MERGER_SIGNATURE_REQUESTER_HPP

#include <memory>

#include "../module.hpp"
#include "../../application.hpp"
#include "../signer_pool_manager/signer_pool_manager.hpp"
#include "../../chain/partial_block.hpp"
#include "../../chain/transaction.hpp"

namespace gruut {
    using namespace std;

    class SignatureRequester {
    public:
        SignatureRequester() : m_signer_pool_manager(Application::app().getSignerPoolManager()) {}
        bool requestSignatures() {
            if (auto manager = m_signer_pool_manager.lock()) {
                auto signers = manager->getSigners();
                for_each(signers.begin(), signers.end(), [this](Signer& signer) {
                    auto transaction = makeTransaction(signer);
                    auto temporary_block = makePartialBlock(transaction);
//                    makeMessage(temporary_block);
                });

                return true;
            }
            else {
                return false;
            }
        }
    private:
        Transaction makeTransaction(Signer signer) {
            // TODO: Strategy 패턴으로 생성 (
            auto sent_time = to_string(std::time(0));

            if(signer.isNew()) {
                Transaction new_transaction;
                new_transaction.transaction_type = TransactionType::CHECKSUM;
                new_transaction.requestor_id = sent_time;
                // TODO: signer(new or not)에 따라 signature 가 다름.
//                new_transaction.signature =
                // TODO: 유일하게 tx를 식별할 수 있는 transaction_id 결정
                new_transaction.transaction_id = sent_time;
                new_transaction.sent_time = sent_time;

                return new_transaction;
            }
            else {
                Transaction new_transaction;

                new_transaction.transaction_type = TransactionType::CERTIFICATE;
                new_transaction.requestor_id = sent_time;
                // TODO: signer(new or not)에 따라 signature 가 다름.
//                new_transaction.signature = new_transaction.makeSignature();

                return new_transaction;
            }
        }

        PartialBlock makePartialBlock(Transaction transaction) {
            PartialBlock partial_block;

            return partial_block;
        }

        weak_ptr<SignerPoolManager> m_signer_pool_manager;
    };
}
#endif
