#ifndef GRUUT_ENTERPRISE_MERGER_TRANSACTION_FETCHER_HPP
#define GRUUT_ENTERPRISE_MERGER_TRANSACTION_FETCHER_HPP

#include <vector>
#include "../chain/signer.hpp"
#include "../chain/transaction.hpp"

namespace gruut {
    using Transactions = std::vector<Transaction>;
    using Signers = std::vector<Signer>;

    constexpr int MAX_COLLECT_TRANSACTION_SIZE = 4096;

    class TransactionFetcher {
    public:
        TransactionFetcher(Signers &&signers);
        TransactionFetcher(Signer &signers) = delete;
        Transactions fetchAll();
    private:
        Transaction fetch(Signer &signer);

        Transactions m_selected_transaction_list;
        Signers m_signers;
    };
}
#endif
