#ifndef GRUUT_ENTERPRISE_MERGER_TEMPORARY_BLOCK_HPP
#define GRUUT_ENTERPRISE_MERGER_TEMPORARY_BLOCK_HPP

#include "types.hpp"

namespace gruut {
struct PartialBlock {
  timestamp sent_time;
  sender_id_type sender_id;
  chain_id_type chain_id;
  block_height_type height;
  transaction_root_type transaction_root;
};

struct Block {};
} // namespace gruut
#endif
