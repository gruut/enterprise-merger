#ifndef GRUUT_ENTERPRISE_MERGER_TEMPORARY_BLOCK_HPP
#define GRUUT_ENTERPRISE_MERGER_TEMPORARY_BLOCK_HPP

#include "merkle_tree.hpp"
#include "signature.hpp"
#include "transaction.hpp"
#include "types.hpp"

#include <vector>

using namespace std;

namespace gruut {
struct PartialBlock {
  timestamp_type time;
  merger_id_type merger_id;
  local_chain_id_type chain_id;
  block_height_type height;
  transaction_root_type transaction_root;

  vector<Transaction> transactions;
};

} // namespace gruut
#endif
