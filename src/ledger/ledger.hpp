#ifndef GRUUT_ENTERPRISE_MERGER_LEDGER_HPP
#define GRUUT_ENTERPRISE_MERGER_LEDGER_HPP

#include "easy_logging.hpp"
#include "nlohmann/json.hpp"

#include "../chain/mem_ledger.hpp"
#include "../chain/types.hpp"
#include "../services/layered_storage.hpp"

#include <iostream>

namespace gruut {

class Ledger {
protected:
  LayeredStorage *m_layered_storage;
  std::string m_prefix;

public:
  Ledger() {
    m_layered_storage = LayeredStorage::getInstance();
    el::Loggers::getLogger("LEDG");
  };

  virtual bool isValidTx(const Transaction &tx) = 0;
  virtual bool procBlock(const json &txs_json,
                         const std::string &block_id_b64) = 0;

protected:
  template <typename T = std::string> void setPrefix(T &&prefix) {
    m_prefix = prefix;
  }

  template <typename T = std::string> bool saveLedger(T &&key, T &&value) {
    std::string wrap_key = m_prefix + key;
    // CLOG(INFO, "LEDG") << "Save ledger (key=" << wrap_key << ")";
    m_layered_storage->saveLedger(wrap_key, value);
    return true;
  }

  bool saveLedger(mem_ledger_t &ledger) {
    for (auto &record : ledger) {
      record.key = m_prefix + record.key; // key to wrap key
    }

    bool ret_val = m_layered_storage->saveLedger(ledger);
    if (ret_val)
      m_layered_storage->flushLedger();
    return ret_val;
  }

  template <typename T = std::string, typename V = std::vector<std::string>>
  std::string readLedgerByKey(T &&key, V &&block_layer = {}) {
    std::string wrap_key = m_prefix + key;
    // CLOG(INFO, "LEDG") << "Search ledger (key=" << wrap_key << ")";
    return m_layered_storage->readLedgerByKey(wrap_key, block_layer);
  }
};
} // namespace gruut

#endif