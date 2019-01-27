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
  virtual bool procBlock(const json &txs_json, const std::string &block_id_b64,
                         const block_layer_t &block_layer) = 0;

protected:
  void setPrefix(std::string prefix) { m_prefix = std::move(prefix); }

  bool saveLedger(const std::string &key, const std::string &value,
                  const std::string &block_id_b64 = "") {
    std::string wrap_key = m_prefix + key;
    m_layered_storage->saveLedger(wrap_key, value, block_id_b64);
    return true;
  }

  bool saveLedger(mem_ledger_t &ledger) {
    bool ret_val = m_layered_storage->saveLedger(ledger, m_prefix);
    if (ret_val)
      m_layered_storage->flushLedger();
    return ret_val;
  }

  std::string readLedgerByKey(const std::string &key) {
    std::string wrap_key = m_prefix + key;
    return m_layered_storage->readLedgerByKey(wrap_key);
  }

  std::string readLedgerByKeyOnLayer(const std::string &key,
                                     const block_layer_t &bloc_layer = {}) {
    std::string wrap_key = m_prefix + key;
    return m_layered_storage->readLedgerByKey(wrap_key, bloc_layer);
  }
};
} // namespace gruut

#endif