#ifndef GRUUT_ENTERPRISE_MERGER_UNRESOLVED_BLOCK_POOL_HPP
#define GRUUT_ENTERPRISE_MERGER_UNRESOLVED_BLOCK_POOL_HPP

#include "easy_logging.hpp"
#include "omp_hash_map.h"

#include "../../chain/block.hpp"
#include "../../chain/types.hpp"
#include "../../config/config.hpp"
#include "../../services/storage.hpp"

#include <deque>
#include <list>
#include <vector>

namespace gruut {
class UnresolvedBlock {
public:
  Block block;
  int prev_queue_idx{-1};
  size_t confirm_level{0};
  UnresolvedBlock() = default;
  ~UnresolvedBlock() = default;
  UnresolvedBlock(Block &block_, int prev_queue_idx_, size_t confirm_level_)
      : block(block_), prev_queue_idx(prev_queue_idx_),
        confirm_level(confirm_level_) {}
};

class UnresolvedBlockPool {
private:
  std::deque<std::vector<UnresolvedBlock>> m_block_pool; // bin structure
  std::recursive_mutex m_push_mutex;

  std::string m_last_block_id_b64;
  std::string m_last_hash_b64;
  std::atomic<block_height_type> m_last_height;
  timestamp_t m_last_time;

  std::atomic<size_t> m_height_range_max{0};
  std::atomic<bool> m_force_unresolved{false};

public:
  UnresolvedBlockPool() { el::Loggers::getLogger("URBK"); };

  UnresolvedBlockPool(std::string &last_block_id_b64,
                      std::string &last_hash_b64, block_height_type last_height,
                      timestamp_t last_time) {
    setPool(last_block_id_b64, last_hash_b64, last_height, last_time);
    el::Loggers::getLogger("URBK");
  }

  size_t size() {
    return m_block_pool.size();
  }

  bool empty(){
    return m_block_pool.empty();
  }

  void setPool(std::string &last_block_id_b64, std::string &last_hash_b64,
               block_height_type last_height, timestamp_t last_time) {
    m_last_block_id_b64 = last_block_id_b64;
    m_last_hash_b64 = last_hash_b64;
    m_last_height = last_height;
    m_last_time = last_time;
    m_height_range_max = last_height;
  }

  bool push(Block &block) { // we assume this block has valid structure at least

    block_height_type block_height = block.getHeight();

    std::lock_guard<std::recursive_mutex> guard(m_push_mutex);

    if ((Time::now_int() - m_last_time) <
        (block.getHeight() - m_last_height - 1) * config::BP_INTERVAL) {
      return false;
    }

    if (m_last_height >= block_height) {
      return false;
    }

    size_t bin_pos = block_height - m_last_height - 1; // e.g., 0 = 2 - 1 - 1

    if (m_block_pool.size() < bin_pos + 1) {
      m_block_pool.resize(bin_pos + 1);
    }

    bool is_new = true;
    for (auto &each_block : m_block_pool[bin_pos]) {
      if (each_block.block.getBlockId() == block.getBlockId()) {
        is_new = false;
        break;
      }
    }

    if (is_new) {

      int prev_queue_idx = -1; // no previous
      if (bin_pos > 0) {       // if there is previous bin

        size_t idx = 0;
        for (auto &each_block : m_block_pool[bin_pos - 1]) {
          if (each_block.block.getBlockIdB64() == block.getPrevBlockIdB64() &&
              each_block.block.getHashB64() == block.getPrevHashB64()) {
            prev_queue_idx = static_cast<int>(idx);
            break;
          }
          ++idx;
        }
      } else { // no previous
        if (block.getPrevBlockIdB64() == m_last_block_id_b64 &&
            block.getPrevHashB64() == m_last_hash_b64) {
          prev_queue_idx = 0;
        } else {
          return false; // drop block -- this is not linkable block!
        }
      }

      m_block_pool[bin_pos].emplace_back(block, prev_queue_idx, 0);

      int queue_idx = m_block_pool[bin_pos].size() - 1; // last

      if (bin_pos + 1 < m_block_pool.size()) { // if there is next bin
        for (auto &each_block : m_block_pool[bin_pos + 1]) {
          if (each_block.block.getPrevBlockIdB64() == block.getBlockIdB64() &&
              each_block.block.getPrevHashB64() == block.getHashB64()) {
            each_block.prev_queue_idx = queue_idx;
          }
        }
      }

      if (m_height_range_max < block_height) {
        m_height_range_max = block_height;
      }
    }

    m_push_mutex.unlock();

    return true;
  }

  bool getBlock(block_height_type t_height, const std::string &t_prev_hash_b64, Block &ret_block) {
    std::lock_guard<std::recursive_mutex> guard(m_push_mutex);
    if(m_block_pool.empty()){
      return false;
    }

    if(t_height <= m_last_height || m_last_height + m_block_pool.size() < t_height) {
      return false;
    }

    size_t bin_pos = t_height - m_last_height;
    std::string prev_hash_b64 = t_prev_hash_b64;
    if(t_height == 0) {
      nth_link_type most_possible_link = getMostPossibleLink();
      bin_pos = most_possible_link.height - m_last_height;
      prev_hash_b64 = most_possible_link.prev_hash_b64;
    }

    bool is_some = false;
    for(auto &each_block : m_block_pool[bin_pos]) {
      if(each_block.block.getPrevHashB64() == prev_hash_b64) {
        ret_block = each_block.block;
        is_some = true;
        break;
      }
    }

    return is_some;
  }

  std::vector<Block> getResolvedBlocks() { // flushing out resolved blocks
    std::vector<Block> blocks;
    std::lock_guard<std::recursive_mutex> guard(m_push_mutex);

    size_t num_resolved_block = 0;

    do {
      num_resolved_block = blocks.size();
      resolveBlocksStepByStep(blocks);
    } while (num_resolved_block < blocks.size());

    m_push_mutex.unlock();
    return blocks;
  }

  block_height_type
  getUnresolvedLowestHeight() { // must be called after getResolvedBlocks()
    std::lock_guard<std::recursive_mutex> guard(m_push_mutex);

    int unresolved_lowest_height = -1;
    for (size_t i = 0; i < m_block_pool.size(); ++i) {
      if (m_block_pool[i].empty()) { // empty, but reserved
        unresolved_lowest_height = static_cast<int>(i);
        break;
      } else {
        bool is_some = false;
        for (auto &each_block : m_block_pool[i]) {
          if (each_block.prev_queue_idx != -1) {
            is_some = true;
            break;
          }
        }

        if (!is_some) {
          unresolved_lowest_height = static_cast<int>(i);
          break;
        }
      }
    }

    if (unresolved_lowest_height < 0)
      return 0;

    return static_cast<block_height_type>((size_t)unresolved_lowest_height +
                                          m_last_height);
  }

  nth_link_type getMostPossibleLink() {

    std::lock_guard<std::recursive_mutex> guard(m_push_mutex);

    nth_link_type ret_link;

    // when resolved situation, the blow are the same with storage
    // unless, they are faster than storage
    ret_link.height = m_last_height;
    ret_link.id_b64 = m_last_block_id_b64;
    ret_link.hash_b64 = m_last_hash_b64;

    if (m_block_pool.empty()) {
      return ret_link;
    }

    nth_link_type longest_link;
    longest_link.height = 0;
    for (size_t i = 0; i < m_block_pool[0].size(); ++i) {
      if (m_block_pool[0][i].prev_queue_idx == 0) {

        if (m_block_pool[0][i].block.getHeight() >
            longest_link.height) { // relaxation
          longest_link.height = m_block_pool[0][i].block.getHeight();
          longest_link.id_b64 = m_block_pool[0][i].block.getBlockIdB64();
          longest_link.hash_b64 = m_block_pool[0][i].block.getHashB64();
        }

        recSearchLongestLink(0, i, longest_link);
      }
    }

    if (ret_link.height < longest_link.height)
      return longest_link;

    return ret_link;
  }

  bool hasUnresolvedBlocks() {
    CLOG(INFO, "URBK") << "Unresolved block pool status = (" << m_last_height
                       << "," << m_height_range_max << ")";
    return m_force_unresolved;
  }

private:
  // search prev_level + 1
  void recSearchLongestLink(size_t prev_level, size_t prev_node,
                            nth_link_type &longest_link) {
    if (m_block_pool.size() > prev_level + 1) {

      for (size_t i = 0; i < m_block_pool[prev_level + 1].size();
           ++i) { // if this level is empty, it will automatically stop to
                  // search
        auto &each_block = m_block_pool[prev_level + 1][i];
        if (each_block.prev_queue_idx == prev_node) {
          if (each_block.block.getHeight() >
              longest_link.height) { // relaxation
            longest_link.height = each_block.block.getHeight();
            longest_link.id_b64 = each_block.block.getBlockIdB64();
            longest_link.hash_b64 = each_block.block.getHashB64();
          }

          recSearchLongestLink(prev_level + 1, i, longest_link);
        }
      }
    }
  }

  void resolveBlocksStepByStep(std::vector<Block> &resolved_blocks) {

    updateConfirmLevel();

    if (m_block_pool.size() > 2 && !m_block_pool[0].empty() &&
        !m_block_pool[1].empty()) {

      size_t highest_confirm_level = 0;
      int t_idx = -1;

      for (size_t i = 0; i < m_block_pool[0].size(); ++i) {
        if (m_block_pool[0][i].prev_queue_idx == 0 &&
            m_block_pool[0][i].confirm_level > highest_confirm_level) {
          highest_confirm_level = m_block_pool[0][i].confirm_level;
          t_idx = static_cast<int>(i);
        }
      }

      if (t_idx < 0) { // nothing to do
        return;
      }

      if (highest_confirm_level > config::CONFIRM_LEVEL) {

        bool is_after = false;
        for (size_t i = 0; i < m_block_pool[1].size(); ++i) {
          if (m_block_pool[1][i].prev_queue_idx == t_idx) {
            is_after = true;
            break;
          }
        }

        if (!is_after) {
          return;
        }

        m_last_block_id_b64 = m_block_pool[0][t_idx].block.getBlockIdB64();
        m_last_hash_b64 = m_block_pool[0][t_idx].block.getHashB64();
        m_last_height = m_block_pool[0][t_idx].block.getHeight();

        resolved_blocks.emplace_back(m_block_pool[0][t_idx].block);

        // clear this height list

        if (m_block_pool[0].size() > 1)
          CLOG(INFO, "URBK") << "Dropped out " << m_block_pool[0].size() - 1
                             << " unresolved block(s)";

        m_block_pool.pop_front();

        if (!m_block_pool.empty()) {

          for (auto &each_block : m_block_pool[0]) {
            if (each_block.block.getPrevBlockIdB64() == m_last_block_id_b64 &&
                each_block.block.getPrevHashB64() == m_last_hash_b64) {
              each_block.prev_queue_idx = 0;
            } else {
              each_block.prev_queue_idx =
                  -1; // this block is unlinkable => to be deleted
            }
          }
        }
      }
    }
  }

  void updateConfirmLevel() {
    if (m_block_pool.empty())
      return;

    for (auto &each_level : m_block_pool) {
      for (auto &each_block : each_level) {
        each_block.confirm_level = each_block.block.getNumSSigs();
      }
    }

    for (int i = (int)m_block_pool.size() - 1; i > 0; --i) {
      for (auto &each_block : m_block_pool[i]) { // for vector
        if (each_block.prev_queue_idx >= 0 &&
            m_block_pool[i - 1].size() > each_block.prev_queue_idx) {
          m_block_pool[i - 1][each_block.prev_queue_idx].confirm_level +=
              each_block.confirm_level;
        }
      }
    }
  }
};
} // namespace gruut

#endif // GRUUT_ENTERPRISE_MERGER_UNRESOLVED_BLOCK_POOL_HPP
