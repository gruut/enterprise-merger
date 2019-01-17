#ifndef GRUUT_ENTERPRISE_MERGER_SMS_HPP
#define GRUUT_ENTERPRISE_MERGER_SMS_HPP

#include "../services/storage.hpp"
#include "ledger.hpp"

namespace gruut {
class SmsLedger : public Ledger {

private:
  Storage *m_storage;
  std::string m_prefix{"S"};

public:
  SmsLedger() { m_storage = Storage::getInstance(); }

  bool isValidTx(Transaction &tx) override { return true; }

  bool procBlock(json &block_json) override { return true; }
};
} // namespace gruut

#endif