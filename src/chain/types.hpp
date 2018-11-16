#ifndef GRUUT_HANA_MERGER_TYPES_HPP
#define GRUUT_HANA_MERGER_TYPES_HPP

#include <string>
#include <vector>

namespace gruut {
    enum class TransactionType {
        CHECKSUM,
        CERTIFICATE
    };

    using sha256 = std::string;
    using timestamp = std::string;
    using block_height_type = std::string;

    using transaction_id_type = sha256;
    using requestor_id_type = sha256;
    using sender_id_type = sha256;
    using transaction_root_type = sha256;
    using chain_id_type = sha256;
    using signature_type = sha256;

    using content_type = std::string;
}
#endif
