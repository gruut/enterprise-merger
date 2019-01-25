#ifndef GRUUT_ENTERPRISE_MERGER_DIGEST_HPP
#define GRUUT_ENTERPRISE_MERGER_DIGEST_HPP

#include "../services/storage.hpp"
#include "ledger.hpp"

namespace gruut {
class DigestLedger : public Ledger {
public:
  DigestLedger() { setPrefix("D"); }

  bool isValidTx(const Transaction &tx) override { return true; }

  bool procBlock(const json &txs_json, const std::string &block_id_b64,
                 const block_layer_t &block_layer) override {
    return true;
  }
};
} // namespace gruut

#endif