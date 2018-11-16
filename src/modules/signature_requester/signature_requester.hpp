#ifndef GRUUT_HANA_MERGER_SIGNATURE_REQUESTER_HPP
#define GRUUT_HANA_MERGER_SIGNATURE_REQUESTER_HPP

#include <memory>

#include "../module.hpp"
#include "../../application.hpp"
#include "../signer_pool_manager/signer_pool_manager.hpp"
#include "../../chain/partial_block.hpp"
#include "../../chain/transaction.hpp"
#include "../../services/transaction_fetcher.hpp"
#include "../../chain/signer.hpp"

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
            TransactionFetcher transaction_fetcher {signer};
            auto transaction = transaction_fetcher.fetch();

            return transaction;
        }

        PartialBlock makePartialBlock(Transaction transaction) {
            PartialBlock partial_block;

            return partial_block;
        }
    private:
        weak_ptr<SignerPoolManager> m_signer_pool_manager;
    };
}
#endif
