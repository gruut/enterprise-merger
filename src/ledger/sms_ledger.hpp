#ifndef GRUUT_ENTERPRISE_MERGER_SMS_HPP
#define GRUUT_ENTERPRISE_MERGER_SMS_HPP

#include "../services/storage.hpp"
#include "ledger.hpp"

namespace gruut {
class SmsLedger : public Ledger {
public:
  SmsLedger() { setPrefix("S"); }

  bool isValidTx(const Transaction &tx) override { return true; }

  bool procBlock(const json &txs_json,
                 const std::string &block_id_b64, const block_layer_t &block_layer) override {
    return true;
  }
};
} // namespace gruut

#endif