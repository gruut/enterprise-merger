#ifndef GRUUT_ENTERPRISE_MERGER_TRANSACTION_HPP
#define GRUUT_ENTERPRISE_MERGER_TRANSACTION_HPP

#include "types.hpp"

namespace gruut {
struct Transaction {
  transaction_id_type transaction_id;
  timestamp sent_time;
  requestor_id_type requestor_id;
  TransactionType transaction_type;
  signature_type signature;
  content_type content;
};

struct NullTransaction : public Transaction {
  NullTransaction() { transaction_id = sha256(); }
};
} // namespace gruut
#endif
