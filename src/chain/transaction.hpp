#ifndef GRUUT_ENTERPRISE_MERGER_TRANSACTION_HPP
#define GRUUT_ENTERPRISE_MERGER_TRANSACTION_HPP

#include "types.hpp"
#include <array>
#include <string>
#include <vector>

namespace gruut {
struct Transaction {
  transaction_id_type transaction_id;
  std::array<uint8_t, 8> sent_time;
  requestor_id_type requestor_id;
  TransactionType transaction_type;
  signature_type signature;
  std::vector<content_type> content_list;
};

struct NullTransaction : public Transaction {
  NullTransaction() { transaction_id = sha256(); }
};
} // namespace gruut
#endif
