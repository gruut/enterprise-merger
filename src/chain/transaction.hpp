#ifndef GRUUT_ENTERPRISE_MERGER_TRANSACTION_HPP
#define GRUUT_ENTERPRISE_MERGER_TRANSACTION_HPP

#include "types.hpp"
#include <array>
#include <string>
#include <vector>

#include <iostream>

namespace gruut {
class Transaction {
public:
  Transaction() {}
  transaction_id_type transaction_id;
  timestamp_type sent_time;
  requestor_id_type requestor_id;
  TransactionType transaction_type;
  signature_type signature;
  std::vector<content_type> content_list;

  virtual bool isNull() { return false; }
};

class NullTransaction : public Transaction {
  virtual bool isNull() { return true; }
};
} // namespace gruut
#endif
