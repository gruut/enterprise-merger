#ifndef GRUUT_ENTERPRISE_MERGER_LEDGER_HPP
#define GRUUT_ENTERPRISE_MERGER_LEDGER_HPP

#include "../chain/types.hpp"
#include "nlohmann/json.hpp"
#include <iostream>

namespace gruut {

class Block;

class Ledger {
private:
public:
  Ledger() = default;

  virtual bool isValidTx(Transaction &tx) = 0;
  virtual bool procBlock(json &block_json) = 0;
};
} // namespace gruut

#endif