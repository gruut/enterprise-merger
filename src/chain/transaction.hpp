#ifndef GRUUT_HANA_MERGER_TRANSACTION_HPP
#define GRUUT_HANA_MERGER_TRANSACTION_HPP

#include "types.hpp"

namespace gruut {
    struct Transaction {
        transaction_id_type txid;
        timestamp sent_time;
        requestor_id_type requestor_id;
        TransactionType transaction_type;
        certification_array_type certifications;
    };
}
#endif
