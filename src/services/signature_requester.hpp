#ifndef GRUUT_HANA_MERGER_SIGNATURE_REQUESTER_HPP
#define GRUUT_HANA_MERGER_SIGNATURE_REQUESTER_HPP

#include <vector>
#include <memory>
#include <boost/asio.hpp>

namespace gruut {
    class PartialBlock;

    class Transaction;

    const int SIGNATURE_COLLECTION_INTERVAL = 10000;
    using Transactions = std::vector<Transaction>;

    class SignatureRequester {
    public:
        SignatureRequester();

        bool requestSignatures();

    private:
        void startSignatureCollectTimer();

        Transactions fetchTransactions();

//        PartialBlock makePartialBlock(Transaction transaction);
        std::unique_ptr<boost::asio::steady_timer> m_timer;
    };
}
#endif
