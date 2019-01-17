#ifndef GRUUT_ENTERPRISE_MERGER_LEDGER_HPP
#define GRUUT_ENTERPRISE_MERGER_LEDGER_HPP

#include "../chain/types.hpp"
#include "nlohmann/json.hpp"
#include <iostream>

namespace gruut {

class Ledger {
protected:
  Storage *m_storage;
  std::string m_prefix;

public:
  Ledger() { m_storage = Storage::getInstance(); };

  virtual bool isValidTx(Transaction &tx) = 0;
  virtual bool procBlock(json &block_json) = 0;

protected:
  template <typename T = std::string> void setPrefix(T &&prefix) {
    m_prefix = prefix;
  }

  template <typename T = std::string> bool saveLedger(T &&key, T &&value) {
    std::string wrap_key = m_prefix + key;
    m_storage->saveLedger(wrap_key, value);
    return true;
  }

  template <typename T = std::string> std::string searchLedger(T &&key) {
    std::string wrap_key = m_prefix + key;
    return m_storage->readLedger(wrap_key);
  }
};
} // namespace gruut

#endif