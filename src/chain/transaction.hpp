#ifndef GRUUT_ENTERPRISE_MERGER_TRANSACTION_HPP
#define GRUUT_ENTERPRISE_MERGER_TRANSACTION_HPP

#include "types.hpp"
#include <array>
#include <string>
#include <vector>

namespace gruut {
struct Transaction {
  transaction_id_type transaction_id;
  std::vector<uint8_t> sent_time;
  requestor_id_type requestor_id;
  TransactionType transaction_type;
  signature_type signature;
  std::vector<content_type> content_list;

  virtual bool isNull() { return false; }
};

struct NullTransaction : public Transaction {
  virtual bool isNull() { return true; }
};
} // namespace gruut
#endif
