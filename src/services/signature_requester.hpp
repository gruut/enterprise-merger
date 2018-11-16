#ifndef GRUUT_HANA_MERGER_SIGNATURE_REQUESTER_HPP
#define GRUUT_HANA_MERGER_SIGNATURE_REQUESTER_HPP

#include <memory>

#include "../application.hpp"
#include "../modules/signer_pool_manager/signer_pool_manager.hpp"
#include "../chain/partial_block.hpp"
#include "../chain/transaction.hpp"
#include "transaction_fetcher.hpp"
#include "../chain/signer.hpp"

namespace gruut {
    using namespace std;
    constexpr int SIGNATURE_COLLECTION_INTERVAL = 10000;

    class SignatureRequester {
    public:
        SignatureRequester() : m_signer_pool_manager(Application::app().getSignerPoolManager()),
                               m_timer(Application::app().getIoService()) {}

        bool requestSignatures() {
            if (auto manager = m_signer_pool_manager.lock()) {
                auto signers = manager->getSigners();
                for_each(signers.begin(), signers.end(), [this](Signer &signer) {
                    auto transaction = makeTransaction(signer);
                    auto temporary_block = makePartialBlock(transaction);
//                    makeMessage(temporary_block);
                });

                startSignatureCollectTimer();
                return true;
            } else {
                return false;
            }
        }

    private:
        void startSignatureCollectTimer() {
            m_timer.expires_from_now(boost::posix_time::milliseconds(SIGNATURE_COLLECTION_INTERVAL));

            m_timer.async_wait([this](const boost::system::error_code &ec) {
                if (ec == boost::asio::error::operation_aborted) {
                    cout << "Timer was cancelled or retriggered." << endl;
                } else if (ec.value() == 0) {
                }
            });
        }

        Transaction makeTransaction(Signer signer) {
            TransactionFetcher transaction_fetcher{signer};
            auto transaction = transaction_fetcher.fetch();

            return transaction;
        }

        PartialBlock makePartialBlock(Transaction transaction) {
            PartialBlock partial_block;

            return partial_block;
        }

        boost::asio::deadline_timer m_timer;
        weak_ptr<SignerPoolManager> m_signer_pool_manager;
    };
}
#endif
