#ifndef GRUUT_ENTERPRISE_MERGER_MEM_LEDGER_HPP
#define GRUUT_ENTERPRISE_MERGER_MEM_LEDGER_HPP

#include <list>
#include <string>

namespace gruut {

struct LedgerRecord {
  std::string key;
  std::string value;
  std::string block_id_b64;
  LedgerRecord(std::string key_, std::string value_, std::string block_id_b64_)
      : key(std::move(key_)), value(std::move(value_)),
        block_id_b64(std::move(block_id_b64_)) {}
};

using mem_ledger_t = std::list<LedgerRecord>;

} // namespace gruut

#endif // GRUUT_ENTERPRISE_MERGER_MEM_LEDGER_HPP
