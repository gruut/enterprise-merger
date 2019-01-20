#ifndef GRUUT_ENTERPRISE_MERGER_MEM_LEDGER_HPP
#define GRUUT_ENTERPRISE_MERGER_MEM_LEDGER_HPP

#include <list>

namespace gruut {

struct LedgerRecord {
  std::string key;
  std::string value;
  std::string block_id_b64;
  LedgerRecord(std::string &key_, std::string &value_, std::string &block_id_b64_) :
  key(key_), value(value_), block_id_b64(block_id_b64_) {

  }
};

using mem_ledger_t = std::list<LedgerRecord>;

}

#endif //GRUUT_ENTERPRISE_MERGER_MEM_LEDGER_HPP
