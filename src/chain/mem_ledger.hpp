#ifndef GRUUT_ENTERPRISE_MERGER_MEM_LEDGER_HPP
#define GRUUT_ENTERPRISE_MERGER_MEM_LEDGER_HPP

#include <list>
#include <string>
#include <memory>
#include <vector>
#include <mutex>

namespace gruut {

struct KeyValue {
  std::string key;
  std::string value;
  KeyValue(std::string key_, std::string value_) :
      key(std::move(key_)), value(std::move(value_)) {

  }
};

struct LedgerRecord {
  std::string key;
  std::string value;
  std::string block_id_b64;
  LedgerRecord(std::string key_, std::string value_, std::string block_id_b64_)
      : key(std::move(key_)), value(std::move(value_)),
        block_id_b64(std::move(block_id_b64_)) {}
};

class MemLedger {
private:
  std::list<LedgerRecord> m_ledger;
  std::mutex m_active_mutex;
public:
  MemLedger() = default;

  bool push(std::string key, std::string value, std::string block_id_b64){
    std::lock_guard<std::mutex> lock(m_active_mutex);
    m_ledger.emplace_back(std::move(key),std::move(value),std::move(block_id_b64));
  }

  bool append(MemLedger &other_ledger, const std::string &prefix = ""){
    std::lock_guard<std::mutex> lock(m_active_mutex);
    std::list<LedgerRecord> mem_ledger = other_ledger.getLedger();
    if(!prefix.empty()) {
      for (auto &record : mem_ledger) {
        record.key = prefix + record.key; // key to wrap key
      }
    }

    m_ledger.insert(m_ledger.end(), mem_ledger.begin(), mem_ledger.end());
  }

  bool getVal(const std::string &key, const std::string &block_id_b64, std::string &ret_val){
    std::lock_guard<std::mutex> lock(m_active_mutex);
    bool is_found = false;
    for(auto &each : m_ledger) {
      if(each.key == key && each.block_id_b64 == block_id_b64){
        ret_val = each.value;
        is_found = true;
        break;
      }
    }

    return is_found;
  }

  std::vector<KeyValue> getKV(const std::string &block_id_b64){
    std::vector<KeyValue> ret_kv;
    std::lock_guard<std::mutex> lock(m_active_mutex);
    for(auto &each : m_ledger) {
      if( each.block_id_b64 == block_id_b64){
        ret_kv.emplace_back(each.key,each.value);
      }
    }

    return ret_kv;
  }

  void dropKV(const std::string &block_id_b64){
    std::lock_guard<std::mutex> lock(m_active_mutex);
    m_ledger.remove_if([this, &block_id_b64](LedgerRecord &t) {
      return (t.block_id_b64 == block_id_b64);
    });
  }

  void clear(){
    std::lock_guard<std::mutex> lock(m_active_mutex);
    m_ledger.clear();
  }

protected:
  std::list<LedgerRecord> getLedger(){
    return m_ledger;
  }
};

using mem_ledger_t = MemLedger;

} // namespace gruut

#endif // GRUUT_ENTERPRISE_MERGER_MEM_LEDGER_HPP
