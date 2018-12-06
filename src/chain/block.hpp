#ifndef GRUUT_ENTERPRISE_MERGER_TEMPORARY_BLOCK_HPP
#define GRUUT_ENTERPRISE_MERGER_TEMPORARY_BLOCK_HPP

#include "types.hpp"
#include <array>

namespace gruut {
struct PartialBlock {
  merger_id_type merger_id;
  chain_id_type chain_id;
  block_height_type height;
  transaction_root_type transaction_root;
};

struct Block {};
} // namespace gruut
#endif
