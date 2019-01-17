#ifndef GRUUT_ENTERPRISE_MERGER_DIGEST_HPP
#define GRUUT_ENTERPRISE_MERGER_DIGEST_HPP

#include "../services/storage.hpp"
#include "ledger.hpp"

namespace gruut {
class DigestLedger : public Ledger {
public:
  DigestLedger() { setPrefix("D"); }

  bool isValidTx(Transaction &tx) override { return true; }

  bool procBlock(json &block_json) override { return true; }
};
} // namespace gruut

#endif