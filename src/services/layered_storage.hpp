#ifndef GRUUT_ENTERPRISE_MERGER_LAYERED_STORAGE_HPP
#define GRUUT_ENTERPRISE_MERGER_LAYERED_STORAGE_HPP

#include "../chain/mem_ledger.hpp"
#include "../utils/template_singleton.hpp"
#include "easy_logging.hpp"
#include "storage.hpp"

namespace gruut {

class LayeredStorage : public TemplateSingleton<LayeredStorage> {
private:
  Storage *m_storage;
  mem_ledger_t m_mem_ledger;
  std::vector<std::string> m_block_layer;

public:
  LayeredStorage() {
    el::Loggers::getLogger("LAYS");
    m_storage = Storage::getInstance();
  }

  bool saveLedger(mem_ledger_t &mem_ledger, const std::string &prefix = "") {
    m_mem_ledger.append(mem_ledger, prefix);
    return true;
  }

  template <typename T = std::string>
  bool saveLedger(T &&key, T &&value, T &&block_id_b64 = "") {

    if (block_id_b64.empty())
      return m_storage->saveLedger(key, value);

    m_mem_ledger.push(key, value, block_id_b64);

    return true;
  }
  template <typename V = std::vector<std::string>>
  void setBlockLayer(V &&block_layer = {}) {
    m_block_layer = block_layer;
  }

  template <typename T = std::string, typename V = block_layer_t>
  std::string readLedgerByKey(T &&key, V &&block_layer = {}) {
    std::string ret_val;

    if (block_layer.empty()) {
      for (auto &each_block_id_b64 : m_block_layer) { // reverse_order
        if (m_mem_ledger.getVal(key, each_block_id_b64, ret_val)) {
          break;
        }
      }
    } else {
      for (auto &each_block_id_b64 : block_layer) { // reverse_order
        if (m_mem_ledger.getVal(key, each_block_id_b64, ret_val)) {
          break;
        }
      }
    }

    if (ret_val.empty())
      ret_val = m_storage->readLedgerByKey(key);

    return ret_val;
  }

  void flushLedger() { m_storage->flushLedger(); }

  void clearLedger() { m_mem_ledger.clear(); }

  template <typename T = std::string> void moveToDiskLedger(T &&block_id_b64) {

    auto kv_vector = m_mem_ledger.getKV(block_id_b64);

    for (auto &each_record : kv_vector) {
      m_storage->saveLedger(each_record.key, each_record.value);
    }

    m_storage->flushLedger();
    dropLedger(block_id_b64);
  }

  template <typename T = std::string> void dropLedger(T &&block_id_b64) {
    m_mem_ledger.dropKV(block_id_b64);
  }
};

} // namespace gruut

#endif // GRUUT_ENTERPRISE_MERGER_LAYERED_STORAGE_HPP
