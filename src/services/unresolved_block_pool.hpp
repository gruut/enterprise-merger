#ifndef GRUUT_ENTERPRISE_MERGER_UNRESOLVED_BLOCK_POOL_HPP
#define GRUUT_ENTERPRISE_MERGER_UNRESOLVED_BLOCK_POOL_HPP

#include "omp_hash_map.h"

#include "../chain/block.hpp"
#include "../chain/types.hpp"
#include "../config/config.hpp"
#include "storage.hpp"

namespace gruut {
struct unresolved_block {
  Block block;
  std::atomic<int> prev_queue_idx;
  unresolved_block() {}
  unresolved_block(Block &block_, int prev_queue_idx_)
      : block(block_), prev_queue_idx(prev_queue_idx_) {}
};

class UnresolvedBlockPool {
private:
  std::deque<std::vector<unresolved_block>> m_block_pool; // bin structure
  std::mutex m_push_mutex;

  std::atomic<size_t> m_lowest_height;
  std::atomic<size_t> m_highest_height{0};
  block_id_type m_last_block_id;
  sha256 m_last_hash;
  block_height_type m_last_height;

public:
  UnresolvedBlockPool() {
    m_lowest_height = std::numeric_limits<size_t>::max();
  }
  UnresolvedBlockPool(block_id_type last_block_id, sha256 last_hash,
                      block_height_type last_height)
      : m_last_block_id(last_block_id), m_last_hash(last_hash),
        m_last_height(last_height) {
    m_lowest_height = std::numeric_limits<size_t>::max();
  }

  void setPool(block_id_type last_block_id, sha256 last_hash,
               block_height_type last_height) {
    m_last_block_id = last_block_id;
    m_last_hash = last_hash;
    m_last_height = last_height;
  }

  void push(Block &block) {
    std::vector<unresolved_block> empty_bin;
    block_height_type block_height = block.getHeight();

    std::lock_guard<std::mutex> guard(m_push_mutex);

    size_t bin_pos = block_height - m_last_height - 1; // e.g., 0 = 2 - 1 - 1

    while (m_block_pool.size() < bin_pos + 1) {
      m_block_pool.emplace_back(empty_bin);
    }

    bool is_new = true;
    for (auto &each_block : m_block_pool[bin_pos]) {
      if (each_block.block.getBlockId() == block.getBlockId()) {
        is_new = false;
        break;
      }
    }

    if (is_new) {

      int prev_queue_idx = -1;
      if (bin_pos > 0) { // if there is previous bin
        for (size_t i = 0; i < m_block_pool[bin_pos - 1].size(); ++i) {
          if (m_block_pool[bin_pos - 1][i].block.getBlockIdB64() ==
                  block.getPrevBlockIdB64() &&
              m_block_pool[bin_pos - 1][i].block.getHashB64() ==
                  block.getPrevHashB64()) {
            prev_queue_idx = static_cast<int>(i);
            break;
          }
        }
      }

      m_block_pool[bin_pos].emplace_back(
          unresolved_block(block, prev_queue_idx));

      int queue_idx = m_block_pool[bin_pos].size() - 1; // last

      if (bin_pos + 1 < m_block_pool.size()) { // if there is next bin
        for (auto &each_block : m_block_pool[bin_pos + 1]) {
          if (each_block.block.getPrevBlockIdB64() == block.getBlockIdB64() &&
              each_block.block.getPrevHashB64() == block.getHashB64()) {
            each_block.prev_queue_idx = queue_idx;
          }
        }
      }

      if (block.getHeight() < m_lowest_height) {
        m_lowest_height = block.getHeight();
      }

      if (block.getHeight() > m_highest_height) {
        m_highest_height = block.getHeight();
      }
    }

    m_push_mutex.unlock();
  }

  //  std::vector<Block> getResolvedBlocks(){
  //
  //  }

private:
};
} // namespace gruut

#endif // GRUUT_ENTERPRISE_MERGER_UNRESOLVED_BLOCK_POOL_HPP
