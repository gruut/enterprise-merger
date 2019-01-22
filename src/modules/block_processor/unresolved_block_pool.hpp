#ifndef GRUUT_ENTERPRISE_MERGER_UNRESOLVED_BLOCK_POOL_HPP
#define GRUUT_ENTERPRISE_MERGER_UNRESOLVED_BLOCK_POOL_HPP

#include "easy_logging.hpp"

#include "../../config/config.hpp"

#include "../../chain/block.hpp"
#include "../../chain/types.hpp"
#include "../../utils/type_converter.hpp"

#include <deque>
#include <list>
#include <vector>

namespace gruut {
struct UnresolvedBlock {
public:
  Block block;
  int prev_queue_idx{-1};
  bool init_linked{false}; // This is for unresolved block because of previous
                           // missing blocks!
  size_t confirm_level{0};
  UnresolvedBlock() = default;
  UnresolvedBlock(Block &block_, int prev_queue_idx_, size_t confirm_level_,
                  bool init_linked_)
      : block(block_), prev_queue_idx(prev_queue_idx_),
        confirm_level(confirm_level_), init_linked(init_linked_) {}
};

struct BlockPosOnMap {
  size_t height;
  size_t vector_idx;
};

class UnresolvedBlockPool {
private:
  std::deque<std::vector<UnresolvedBlock>> m_block_pool; // bin structure

  std::recursive_mutex m_push_mutex;

  block_id_type m_last_block_id;
  hash_t m_last_hash;
  std::atomic<block_height_type> m_last_height;
  timestamp_t m_last_time;

  std::atomic<size_t> m_height_range_max{0};
  std::atomic<bool> m_force_unresolved{false};

  std::atomic<bool> m_cache_link_valid {false};
  nth_link_type m_cache_possible_link;

  std::atomic<bool> m_cache_layer_valid {false};
  std::vector<std::string> m_cache_possible_block_layer;

  std::atomic<bool> m_cache_pos_valid {false};
  BlockPosOnMap m_cache_possible_pos;

public:
  UnresolvedBlockPool() { el::Loggers::getLogger("URBK"); };

  size_t size() { return m_block_pool.size(); }

  bool empty() { return m_block_pool.empty(); }

  void clear() { m_block_pool.clear(); }

  void setPool(const block_id_type &last_block_id, const hash_t &last_hash,
               block_height_type last_height, timestamp_t last_time) {
    m_last_block_id = last_block_id;
    m_last_hash = last_hash;
    m_last_height = last_height;
    m_last_time = last_time;
    m_height_range_max = last_height;
  }

  bool prepareBins(block_height_type t_height) {
    std::lock_guard<std::recursive_mutex> guard(m_push_mutex);
    if (m_last_height >= t_height) {
      return false;
    }

    if ((Time::now_int() - m_last_time) <
        (t_height - m_last_height - 1) * config::BP_INTERVAL) {
      return false;
    }

    int bin_pos = t_height - m_last_height - 1; // e.g., 0 = 2 - 1 - 1

    if (m_block_pool.size() < bin_pos + 1) {
      m_block_pool.resize(bin_pos + 1);
    }

    return true;
  }

  void invalidateCaches(){
    m_cache_link_valid = false;
    m_cache_layer_valid = false;
    m_cache_pos_valid = false;
  }

  unblk_push_result_type
  push(Block &block) { // we assume this block has valid structure at least
    unblk_push_result_type ret_val;
    ret_val.height = 0;
    ret_val.linked = false;

    std::lock_guard<std::recursive_mutex> guard(m_push_mutex);

    block_height_type block_height = block.getHeight();
    int bin_pos = block_height - m_last_height - 1; // e.g., 0 = 2 - 1 - 1
    if (!prepareBins(block_height))
      return ret_val;

    bool is_new = true;
    for (auto &each_block : m_block_pool[bin_pos]) {
      if (each_block.block.getBlockId() == block.getBlockId()) {
        is_new = false;
        break;
      }
    }

    if (!is_new)
      return ret_val;

    int prev_queue_idx = -1; // no previous
    if (bin_pos > 0) {       // if there is previous bin
      size_t idx = 0;
      for (auto &each_block : m_block_pool[bin_pos - 1]) {
        if (each_block.block.getBlockId() == block.getPrevBlockId() &&
            each_block.block.getHash() == block.getPrevHash()) {
          prev_queue_idx = static_cast<int>(idx);
          break;
        }
        ++idx;
      }
    } else { // no previous
      if (block.getPrevBlockId() == m_last_block_id &&
          block.getPrevHash() == m_last_hash) {
        prev_queue_idx = 0;
      } else {
        return ret_val; // drop block -- this is not linkable block!
      }
    }

    m_block_pool[bin_pos].emplace_back(block, prev_queue_idx, 0,
                                       (prev_queue_idx >= 0));

    invalidateCaches();

    int queue_idx = m_block_pool[bin_pos].size() - 1; // last

    if (bin_pos + 1 < m_block_pool.size()) { // if there is next bin
      for (auto &each_block : m_block_pool[bin_pos + 1]) {
        if (each_block.block.getPrevBlockId() == block.getBlockId() &&
            each_block.block.getPrevHash() == block.getHash()) {
          each_block.prev_queue_idx = queue_idx;
        }
      }
    }

    ret_val.height = block_height;
    ret_val.linked = (prev_queue_idx >= 0);

    if (m_height_range_max < block_height) {
      m_height_range_max = block_height;
    }

    m_push_mutex.unlock();

    return ret_val;
  }

  template <typename T = std::string, typename H = hash_t>
  bool getBlock(block_height_type t_height, H &&t_prev_hash, H &&t_hash,
                Block &ret_block) {
    std::lock_guard<std::recursive_mutex> guard(m_push_mutex);
    if (m_block_pool.empty()) {
      return false;
    }

    if (t_height <= m_last_height ||
        m_last_height + m_block_pool.size() < t_height) {
      return false;
    }

    int bin_pos = t_height - m_last_height;

    hash_t block_hash = t_hash;
    hash_t prev_hash = t_prev_hash;

    if (t_height == 0) {
      nth_link_type most_possible_link = getMostPossibleLink();
      bin_pos = most_possible_link.height - m_last_height;
      prev_hash = most_possible_link.prev_hash;
      block_hash = most_possible_link.hash;
    }

    if (bin_pos < 0) // something wrong
      return false;

    bool is_some = false;
    for (auto &each_block : m_block_pool[bin_pos]) {
      if ((prev_hash.empty() || each_block.block.getPrevHash() == prev_hash) &&
          (block_hash.empty() || each_block.block.getHash() == block_hash)) {
        ret_block = each_block.block;
        is_some = true;
        break;
      }
    }

    return is_some;
  }

  void getResolvedBlocks(
      std::vector<std::pair<Block, bool>> &blocks,
      std::vector<std::string> &drop_blocks) { // flushing out resolved blocks
    blocks.clear();
    drop_blocks.clear();

    std::lock_guard<std::recursive_mutex> guard(m_push_mutex);

    size_t num_resolved_block = 0;

    do {
      num_resolved_block = blocks.size();
      resolveBlocksStepByStep(blocks, drop_blocks);
    } while (num_resolved_block < blocks.size());

    m_push_mutex.unlock();
  }

  nth_link_type getUnresolvedLowestLink() {
    BlockPosOnMap longest_pos = getLongestBlockPos();

    nth_link_type ret_link;
    ret_link.height = 0; // no unresolved block or invalid link info

    if (m_last_height <= longest_pos.height &&
        longest_pos.height < m_block_pool.size() + m_last_height) {

      int bin_idx = longest_pos.height - m_last_height - 1;

      if (longest_pos.height == m_last_height) {
        ret_link.height = m_last_height + 1;
        ret_link.prev_hash = m_last_hash;
      } else if (bin_idx >= 0) {
        ret_link.height =
            m_block_pool[bin_idx][longest_pos.vector_idx].block.getHeight() + 1;
        ret_link.prev_hash =
            m_block_pool[bin_idx][longest_pos.vector_idx].block.getHash();
      }
    }

    return ret_link;
  }

  nth_link_type getMostPossibleLink() {

    if(m_cache_link_valid)
      return m_cache_possible_link;

    nth_link_type ret_link;

    std::lock_guard<std::recursive_mutex> guard(m_push_mutex);

    // when resolved situation, the blow are the same with storage
    // unless, they are faster than storage

    ret_link.height = m_last_height;
    ret_link.id = m_last_block_id;
    ret_link.hash = m_last_hash;

    if (m_block_pool.empty()) {
      m_cache_possible_link = ret_link;
      m_cache_link_valid = true;
      return ret_link;
    }

    BlockPosOnMap longest_pos = getLongestBlockPos();

    if (ret_link.height < longest_pos.height) {

      int deque_idx = longest_pos.height - m_last_height -
                      1; // must be larger or equal to 0

      if (deque_idx >= 0) {
        auto &t_block = m_block_pool[deque_idx][longest_pos.vector_idx];
        ret_link.height = t_block.block.getHeight();
        ret_link.id = t_block.block.getBlockId();
        ret_link.hash = t_block.block.getHash();
      }
    }

    m_cache_possible_link = ret_link;
    m_cache_link_valid = true;

    return ret_link;
  }

  // reverse order
  std::vector<std::string> getMostPossibleBlockLayer() {

    if(m_cache_layer_valid){
      return m_cache_possible_block_layer;
    }

    auto t_block_pos = getLongestBlockPos();
    std::vector<std::string> ret_block_layer;

    int queue_idx = t_block_pos.vector_idx;

    int bin_end = t_block_pos.height - m_last_height - 1;

    for (int i = bin_end; i >= 0; --i) {
      ret_block_layer.emplace_back(
          m_block_pool[i][queue_idx].block.getBlockIdB64());
      queue_idx = m_block_pool[i][queue_idx].prev_queue_idx;
    }

    m_cache_possible_block_layer = ret_block_layer;
    m_cache_layer_valid = true;
    return ret_block_layer;
  }

  bool hasUnresolvedBlocks() {
    CLOG(INFO, "URBK") << "Unresolved block pool status = (" << m_last_height
                       << "," << m_height_range_max << ")";
    return m_force_unresolved;
  }

private:
  BlockPosOnMap getLongestBlockPos() {
    if(m_cache_pos_valid)
      return m_cache_possible_pos;

    BlockPosOnMap longest_pos;
    longest_pos.height = 0;
    longest_pos.vector_idx = 0;
    if (!m_block_pool.empty()) {
      for (size_t i = 0; i < m_block_pool[0].size(); ++i) {
        if (m_block_pool[0][i].prev_queue_idx == 0) {

          if (m_block_pool[0][i].block.getHeight() >
              longest_pos.height) { // relaxation
            longest_pos.height = m_block_pool[0][i].block.getHeight();
            longest_pos.vector_idx = i;
          }

          recSearchLongestLink(0, i, longest_pos); // recursive call
        }
      }
    }

    m_cache_possible_pos = longest_pos;
    m_cache_pos_valid = true;

    return longest_pos;
  }

  // search prev_level + 1
  void recSearchLongestLink(size_t prev_level, size_t prev_node,
                            BlockPosOnMap &longest_pos) {
    if (m_block_pool.size() <= prev_level + 1)
      return;

    // if this level is empty, it will automatically stop to search
    for (size_t i = 0; i < m_block_pool[prev_level + 1].size(); ++i) {
      auto &each_block = m_block_pool[prev_level + 1][i];
      if (each_block.prev_queue_idx == prev_node) {
        if (each_block.block.getHeight() > longest_pos.height) { // relaxation
          longest_pos.height = each_block.block.getHeight();
          longest_pos.vector_idx = i;
        }

        recSearchLongestLink(prev_level + 1, i, longest_pos);
      }
    }
  }

  void
  resolveBlocksStepByStep(std::vector<std::pair<Block, bool>> &resolved_blocks,
                          std::vector<std::string> &drop_blocks) {

    updateConfirmLevel();

    if (m_block_pool.size() < 2 || m_block_pool[0].empty() ||
        m_block_pool[1].empty())
      return;

    size_t highest_confirm_level = 0;
    int resolved_block_idx = -1;

    for (size_t i = 0; i < m_block_pool[0].size(); ++i) {
      if (m_block_pool[0][i].prev_queue_idx == 0 &&
          m_block_pool[0][i].confirm_level > highest_confirm_level) {
        highest_confirm_level = m_block_pool[0][i].confirm_level;
        resolved_block_idx = static_cast<int>(i);
      }
    }

    if (resolved_block_idx < 0 ||
        highest_confirm_level < config::BLOCK_CONFIRM_LEVEL)
      return; // nothing to do

    bool is_after = false;
    for (auto &each_block : m_block_pool[1]) {
      if (each_block.prev_queue_idx ==
          resolved_block_idx) { // some block links this block
        is_after = true;
        break;
      }
    }

    if (!is_after)
      return;

    m_last_block_id = m_block_pool[0][resolved_block_idx].block.getBlockId();
    m_last_hash = m_block_pool[0][resolved_block_idx].block.getHash();
    m_last_height = m_block_pool[0][resolved_block_idx].block.getHeight();

    resolved_blocks.emplace_back(
        std::make_pair(m_block_pool[0][resolved_block_idx].block,
                       m_block_pool[0][resolved_block_idx].init_linked));

    // clear this height list

    auto leyered_storage = LayeredStorage::getInstance();

    if (m_block_pool[0].size() > 1) {
      for (auto &each_block : m_block_pool[0]) {
        if (each_block.block.getBlockId() != m_last_block_id) {
          drop_blocks.emplace_back(each_block.block.getBlockIdB64());
        }
      }
      CLOG(INFO, "URBK") << "Dropped out " << m_block_pool[0].size() - 1
                         << " unresolved block(s)";
    }

    m_block_pool.pop_front();

    if (m_block_pool.empty())
      return;

    for (auto &each_block : m_block_pool[0]) {
      if (each_block.block.getPrevBlockId() == m_last_block_id &&
          each_block.block.getPrevHash() == m_last_hash) {
        each_block.prev_queue_idx = 0;
      } else {
        // this block is unlinkable => to be deleted
        each_block.prev_queue_idx = -1;
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
