#ifndef GRUUT_HANA_MERGER_TYPES_HPP
#define GRUUT_HANA_MERGER_TYPES_HPP

#include <string>
#include <vector>

#include "transaction.hpp"

namespace gruut {
    enum class TransactionType {

    };

    using sha256 = std::string;
    using transaction_id_type = sha256;
    using timestamp = std::string;
    using requestor_id_type = sha256;
    using sender_id_type = sha256;
    using transaction_root_type = sha256;
    using TransactionType = int;
    using chain_id_type = sha256;
    using block_height_type = std::string;

    using certification_array_type = vector<std::string>;
}


#endif //GRUUT_HANA_MERGER_TYPES_HPP
