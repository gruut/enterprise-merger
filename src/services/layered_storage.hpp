#ifndef GRUUT_ENTERPRISE_MERGER_LAYERED_STORAGE_HPP
#define GRUUT_ENTERPRISE_MERGER_LAYERED_STORAGE_HPP

#include "../utils/template_singleton.hpp"
#include "../chain/mem_ledger.hpp"

#include "storage.hpp"

namespace gruut {

class LayeredStorage : public TemplateSingleton<LayeredStorage>{
private:
  Storage* m_storage;
  mem_ledger_t m_mem_ledger;
  std::recursive_mutex m_push_mutex;
public:
  LayeredStorage(){
    m_storage = Storage::getInstance();
  }

  bool saveLedger(mem_ledger_t &mem_ledger) {
    std::lock_guard<std::recursive_mutex> lock_guard(m_push_mutex);
    m_mem_ledger.insert(m_mem_ledger.end(), mem_ledger.begin(), mem_ledger.end());
    return true;
  }

  template <typename T = std::string>
  bool saveLedger(T&& key, T&& value, T&& block_id_b64 = ""){

    if(block_id_b64.empty())
      return m_storage->saveLedger(key,value);

    std::lock_guard<std::recursive_mutex> lock_guard(m_push_mutex);
    m_mem_ledger.emplace_back(key,value,block_id_b64);

    return true;
  }

  template <typename T = std::string, typename V = std::vector<std::string> >
  std::string readLedgerByKey(T &&key, V&& block_layer = {}){
    std::lock_guard<std::recursive_mutex> lock_guard(m_push_mutex);

    std::string ret_val;

    for(auto &each_block_id_b64 : block_layer) { // reverse_order
      bool is_found = false;
      for (auto it_list = m_mem_ledger.begin(); it_list != m_mem_ledger.end(); ++it_list) {
        if (it_list->key == key && each_block_id_b64 == it_list->block_id_b64) {
          ret_val = it_list->value;
          is_found = true;
          break;
        }
      }
      if(is_found)
        break;
    }

    if(ret_val.empty())
      ret_val = m_storage->readLedgerByKey(key);

    return ret_val;
  }

  void flushLedger(){
    m_storage->flushLedger();
  }

  void clearLedger(){
    std::lock_guard<std::recursive_mutex> lock_guard(m_push_mutex);
    m_mem_ledger.clear();
  }

  template <typename T = std::string>
  void moveToDiskLedger(T&& block_id_b64){
    std::lock_guard<std::recursive_mutex> lock_guard(m_push_mutex);
    for(auto &each_record : m_mem_ledger){
      if(each_record.block_id_b64 == block_id_b64) {
        m_storage->saveLedger(each_record.key,each_record.value);
      }
    }

    m_storage->flushLedger();
    dropLedger(block_id_b64);
  }

  template <typename T = std::string>
  void dropLedger(T&& block_id_b64){
    std::lock_guard<std::recursive_mutex> lock_guard(m_push_mutex);
    m_mem_ledger.remove_if([this,&block_id_b64](LedgerRecord &t){
      return (t.block_id_b64 == block_id_b64);
    });
  }

};

}

#endif //GRUUT_ENTERPRISE_MERGER_LAYERED_STORAGE_HPP
