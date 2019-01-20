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
  size_t confirm_level{0};
  UnresolvedBlock() = default;
  UnresolvedBlock(Block &block_, int prev_queue_idx_, size_t confirm_level_)
      : block(block_), prev_queue_idx(prev_queue_idx_),
        confirm_level(confirm_level_) {}
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


public:
  UnresolvedBlockPool() { el::Loggers::getLogger("URBK"); };

  size_t size() {
    return m_block_pool.size();
  }

  bool empty(){
    return m_block_pool.empty();
  }

  void clear(){
    m_block_pool.clear();
  }

  void setPool(block_id_type& last_block_id, hash_t& last_hash, block_height_type last_height, timestamp_t last_time) {
    m_last_block_id = last_block_id;
    m_last_hash = last_hash;
    m_last_height = last_height;
    m_last_time = last_time;
    m_height_range_max = last_height;
  }

  block_height_type push(Block &block) { // we assume this block has valid structure at least

    block_height_type block_height = block.getHeight();

    std::lock_guard<std::recursive_mutex> guard(m_push_mutex);

    if (m_last_height >= block_height) {
      return 0;
    }

    if ((Time::now_int() - m_last_time) < (block_height - m_last_height - 1) * config::BP_INTERVAL) {
      return 0;
    }


    int bin_pos = block_height - m_last_height - 1; // e.g., 0 = 2 - 1 - 1

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
          if (each_block.block.getBlockId() == block.getPrevBlockId() &&
              each_block.block.getHash() == block.getPrevHash()) {
            prev_queue_idx = static_cast<int>(idx);
            break;
          }
          ++idx;
        }
      } else { // no previous
        if (block.getPrevBlockId() == m_last_block_id && block.getPrevHash() == m_last_hash) {
          prev_queue_idx = 0;
        } else {
          return 0; // drop block -- this is not linkable block!
        }
      }

      m_block_pool[bin_pos].emplace_back(block, prev_queue_idx, 0);

      int queue_idx = m_block_pool[bin_pos].size() - 1; // last

      if (bin_pos + 1 < m_block_pool.size()) { // if there is next bin
        for (auto &each_block : m_block_pool[bin_pos + 1]) {
          if (each_block.block.getPrevBlockId() == block.getBlockId() &&
              each_block.block.getPrevHash() == block.getHash()) {
            each_block.prev_queue_idx = queue_idx;
          }
        }
      }

      if (m_height_range_max < block_height) {
        m_height_range_max = block_height;
      }
    }

    m_push_mutex.unlock();

    return block_height;
  }

  template <typename T = std::string>
  bool getBlock(block_height_type t_height, T&& t_prev_hash_b64, T&& t_hash_b64, Block &ret_block) {
    std::lock_guard<std::recursive_mutex> guard(m_push_mutex);
    if(m_block_pool.empty()){
      return false;
    }

    if(t_height <= m_last_height || m_last_height + m_block_pool.size() < t_height) {
      return false;
    }

    int bin_pos = t_height - m_last_height;

    hash_t block_hash = TypeConverter::decodeBase64(t_hash_b64);
    hash_t prev_hash = TypeConverter::decodeBase64(t_prev_hash_b64);

    if(t_height == 0) {
      nth_link_type most_possible_link = getMostPossibleLink();
      bin_pos = most_possible_link.height - m_last_height;
      prev_hash = most_possible_link.prev_hash;
      block_hash = most_possible_link.hash;
    }

    if(bin_pos < 0) // something wrong
      return false;

    bool is_some = false;
    for(auto &each_block : m_block_pool[bin_pos]) {
      if((prev_hash.empty() || each_block.block.getPrevHash() == prev_hash) &&
        (block_hash.empty() || each_block.block.getHash() == block_hash)) {
        ret_block = each_block.block;
        is_some = true;
        break;
      }
    }

    return is_some;
  }

  void getResolvedBlocks(std::vector<Block> &blocks, std::vector<std::string> &drop_blocks) { // flushing out resolved blocks
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

    if(longest_pos.height >= m_block_pool.size() + m_last_height || longest_pos.height < m_last_height){
      ret_link.height = 0; // no unresolved block or invalid link info
    } else {
      int deque_idx = ret_link.height - m_last_height - 1;

      if(deque_idx >= 0) {
        ret_link.height = m_block_pool[deque_idx][longest_pos.vector_idx].block.getHeight() + 1;
        ret_link.prev_hash = m_block_pool[deque_idx][longest_pos.vector_idx].block.getHash();
      }
    }

    return ret_link;
  }

  nth_link_type getMostPossibleLink() {
    nth_link_type ret_link;

    std::lock_guard<std::recursive_mutex> guard(m_push_mutex);

    // when resolved situation, the blow are the same with storage
    // unless, they are faster than storage

    ret_link.height = m_last_height;
    ret_link.id = m_last_block_id;
    ret_link.hash = m_last_hash;

    if (m_block_pool.empty())
      return ret_link;

    BlockPosOnMap longest_pos = getLongestBlockPos();

    if (ret_link.height < longest_pos.height) {

      int deque_idx = longest_pos.height - m_last_height - 1; // must be larger or equal to 0

      if(deque_idx >= 0) {
        auto &t_block = m_block_pool[deque_idx][longest_pos.vector_idx];
        ret_link.height = t_block.block.getHeight();
        ret_link.id = t_block.block.getBlockId();
        ret_link.hash = t_block.block.getHash();
      }
    }

    return ret_link;
  }

  // reverse order
  std::vector<std::string> getMostPossibleBlockLayer(){
    auto t_block_pos = getLongestBlockPos();
    std::vector<std::string> ret_block_layer;

    int queue_idx = t_block_pos.vector_idx;

    int bin_end = t_block_pos.height - m_last_height - 1;

    for(int i = bin_end; i >= 0; --i) {
      ret_block_layer.emplace_back(m_block_pool[i][queue_idx].block.getBlockIdB64());
      queue_idx = m_block_pool[i][queue_idx].prev_queue_idx;
    }

    return ret_block_layer;
  }

  bool hasUnresolvedBlocks() {
    CLOG(INFO, "URBK") << "Unresolved block pool status = (" << m_last_height
                       << "," << m_height_range_max << ")";
    return m_force_unresolved;
  }

private:

  BlockPosOnMap getLongestBlockPos(){
    BlockPosOnMap longest_pos;
    longest_pos.height = 0;
    for (size_t i = 0; i < m_block_pool[0].size(); ++i) {
      if (m_block_pool[0][i].prev_queue_idx == 0) {

        if (m_block_pool[0][i].block.getHeight() > longest_pos.height) { // relaxation
          longest_pos.height = m_block_pool[0][i].block.getHeight();
          longest_pos.vector_idx = i;
        }

        recSearchLongestLink(0, i, longest_pos); // recursive call
      }
    }

    return longest_pos;
  }

  // search prev_level + 1
  void recSearchLongestLink(size_t prev_level, size_t prev_node, BlockPosOnMap &longest_pos) {
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

  void resolveBlocksStepByStep(std::vector<Block> &resolved_blocks, std::vector<std::string> &drop_blocks) {

    updateConfirmLevel();

    if(m_block_pool.size() < 2 || m_block_pool[0].empty() || m_block_pool[1].empty())
      return;

    size_t highest_confirm_level = 0;
    int t_idx = -1;

    for (size_t i = 0; i < m_block_pool[0].size(); ++i) {
      if (m_block_pool[0][i].prev_queue_idx == 0 &&
          m_block_pool[0][i].confirm_level > highest_confirm_level) {
        highest_confirm_level = m_block_pool[0][i].confirm_level;
        t_idx = static_cast<int>(i);
      }
    }

    if (t_idx < 0 || highest_confirm_level < config::BLOCK_CONFIRM_LEVEL)
      return; // nothing to do

    bool is_after = false;
    for (size_t i = 0; i < m_block_pool[1].size(); ++i) {
      if (m_block_pool[1][i].prev_queue_idx == t_idx) {
        is_after = true;
        break;
      }
    }

    if (!is_after)
      return;

    m_last_block_id = m_block_pool[0][t_idx].block.getBlockId();
    m_last_hash = m_block_pool[0][t_idx].block.getHash();
    m_last_height = m_block_pool[0][t_idx].block.getHeight();

    resolved_blocks.emplace_back(m_block_pool[0][t_idx].block);

    // clear this height list

    auto leyered_storage = LayeredStorage::getInstance();

    if (m_block_pool[0].size() > 1) {
      for(auto &each_block : m_block_pool[0]) {
        if(each_block.block.getBlockId() != m_last_block_id) {
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
