#ifndef GRUUT_ENTERPRISE_MERGER_MEM_LEDGER_HPP
#define GRUUT_ENTERPRISE_MERGER_MEM_LEDGER_HPP

namespace gruut {

struct LedgerRecord {
  std::string key;
  std::string value;
  LedgerRecord(std::string &key_, std::string &value_) : key(key_), value(value_) {}
};

using mem_ledger_t = std::vector<LedgerRecord>;

}

#endif //GRUUT_ENTERPRISE_MERGER_MEM_LEDGER_HPP
