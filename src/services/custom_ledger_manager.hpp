#ifndef GRUUT_ENTERPRISE_MERGER_LEDGER_VALIDATOR_HPP
#define GRUUT_ENTERPRISE_MERGER_LEDGER_VALIDATOR_HPP

#include "../chain/types.hpp"
#include "../ledger/ledger.hpp"
#include "../ledger/digest_ledger.hpp"
#include "../ledger/sms_ledger.hpp"
#include "../utils/safe.hpp"
#include "easy_logging.hpp"
#include "nlohmann/json.hpp"
#include <iostream>
#include <memory>
#include <vector>

namespace gruut {

class CustomLedgerManager {
private:
  std::vector<std::shared_ptr<Ledger>> m_ledgers;
  std::shared_ptr<CertificateLedger> m_certificate_ledger;
  std::shared_ptr<DigestLedger> m_digest_ledger;
  std::shared_ptr<SmsLedger> m_sms_ledger;

public:
  CustomLedgerManager() {
    el::Loggers::getLogger("CLMA");

    m_certificate_ledger = std::make_shared<CertificateLedger>();
    m_digest_ledger = std::make_shared<DigestLedger>();
    m_sms_ledger = std::make_shared<SmsLedger>();

    registerLedger(m_certificate_ledger);
    registerLedger(m_digest_ledger);
    registerLedger(m_sms_ledger);
  }

  bool isValidTransaction(Transaction &tx) {
    if (m_ledgers.empty())
      return false;

    bool is_valid = true;
    for (auto &ledger : m_ledgers) {
      is_valid &= ledger->isValidTx(tx);
    }

    return is_valid;
  }

  void procLedgerBlock(json &block_json) {
    for (auto &ledger : m_ledgers) {
      ledger->procBlock(block_json);
    }
  }

private:
  void registerLedger(std::shared_ptr<Ledger> ledger) {
    m_ledgers.emplace_back(ledger);
  }

  void initLedger() { m_ledgers.clear(); }
};
} // namespace gruut

#endif