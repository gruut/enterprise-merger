#include <iostream>
#include <boost/system/error_code.hpp>
#include "../chain/signer.hpp"
#include "signature_requester.hpp"
#include "../application.hpp"
#include "transaction_fetcher.hpp"

namespace gruut {
    SignatureRequester::SignatureRequester() {
        m_timer.reset(new boost::asio::steady_timer(Application::app().getIoService()));
    }

    bool SignatureRequester::requestSignatures() {
        auto transactions = std::move(fetchTransactions());
//                auto temporary_block = makePartialBlock(transactions);
//                    makeMessage(temporary_block);

        startSignatureCollectTimer();
        return true;
    }

    void SignatureRequester::startSignatureCollectTimer() {
        auto waits = m_timer->expires_after(chrono::milliseconds(SIGNATURE_COLLECTION_INTERVAL));
        std::cout << waits << std::endl;
        m_timer->async_wait([this](const boost::system::error_code &ec) {
            if (ec == boost::asio::error::operation_aborted) {
                std::cout << "startSignatureCollectTimer: Timer was cancelled or retriggered." << std::endl;
            } else if(ec.value() == 0){
                // Sig -> Block generator
                std::cout << "RUN" << std::endl;
            } else {
                std::cout << "ERROR: " << ec.message() << std::endl;
                throw;
            }
        });
    }

    Transactions SignatureRequester::fetchTransactions() {
        vector<Signer> &&signers = Application::app().getSignerPoolManager().getSigners();
        TransactionFetcher transaction_fetcher{move(signers)};

        return transaction_fetcher.fetchAll();
    }
}