#include <iostream>
#include <boost/system/error_code.hpp>

#include "../chain/types.hpp"
#include "../chain/signer.hpp"
#include "signature_requester.hpp"
#include "../application.hpp"
#include "transaction_fetcher.hpp"
#include "block_generator.hpp"
#include "message_factory.hpp"

namespace gruut {
    SignatureRequester::SignatureRequester() {
        m_timer.reset(new boost::asio::deadline_timer(Application::app().getIoService()));
    }

    bool SignatureRequester::requestSignatures() {
        auto transactions = std::move(fetchTransactions());
        auto partial_block = makePartialBlock(transactions);
        auto message = makeMessage(partial_block);

        Application::app().getOutputQueue()->push(message);
        startSignatureCollectTimer(transactions);
        return true;
    }

    void SignatureRequester::startSignatureCollectTimer(Transactions &transactions) {
        m_timer->expires_from_now(boost::posix_time::milliseconds(SIGNATURE_COLLECTION_INTERVAL));
        m_timer->async_wait([this](const boost::system::error_code &ec) {
            if (ec == boost::asio::error::operation_aborted) {
                std::cout << "startSignatureCollectTimer: Timer was cancelled or retriggered." << std::endl;
            } else if (ec.value() == 0) {
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

    PartialBlock SignatureRequester::makePartialBlock(Transactions &transactions) {
        BlockGenerator block_generator;
        vector<sha256> transaction_ids;

        transaction_ids.reserve(transactions.size());
        for (auto &transaction : transactions)
            transaction_ids.emplace_back(transaction.transaction_id);

        auto root_id = m_merkle_tree.generate(transaction_ids);
        auto &&block = block_generator.generatePartialBlock(root_id);

        return block;
    }

    Message SignatureRequester::makeMessage(PartialBlock &block) {
        auto message = MessageFactory::createSigRequestMessage(block);

        return message;
    }
}