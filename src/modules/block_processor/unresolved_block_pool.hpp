#ifndef GRUUT_ENTERPRISE_MERGER_UNRESOLVED_BLOCK_POOL_HPP
#define GRUUT_ENTERPRISE_MERGER_UNRESOLVED_BLOCK_POOL_HPP

#include "../../config/config.hpp"

#include "../../chain/block.hpp"
#include "../../chain/types.hpp"
#include "../../services/layered_storage.hpp"
#include "../../utils/type_converter.hpp"

#include <deque>
#include <list>
#include <vector>

namespace gruut {

const std::string UNRESOLVED_BLOCK_IDS_KEY = "UNRESOLVED_BLOCK_IDS_KEY";

struct UnresolvedBlock {
  int prev_vector_idx{-1};
  bool linked{false};
  size_t confirm_level{0};
  block_layer_t block_layer;
  Block block;

  UnresolvedBlock() = default;
  UnresolvedBlock(Block &block_, int prev_queue_idx_, size_t confirm_level_,
                  bool init_linked_)
      : block(block_), prev_vector_idx(prev_queue_idx_),
        confirm_level(confirm_level_), linked(init_linked_) {}
};

struct BlockPosOnMap {
  size_t height{0};
  size_t vector_idx{0};
  BlockPosOnMap() = default;
  BlockPosOnMap(size_t height_, size_t vector_idx_)
      : height(height_), vector_idx(vector_idx_) {}
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

  std::atomic<bool> m_cache_link_valid{false};
  nth_link_type m_cache_possible_link;

  std::atomic<bool> m_cache_layer_valid{false};
  std::vector<std::string> m_cache_possible_block_layer;

  std::atomic<bool> m_cache_pos_valid{false};
  BlockPosOnMap m_cache_possible_pos;

  LayeredStorage *m_layered_storage;

public:
  UnresolvedBlockPool();
  inline size_t size() { return m_block_pool.size(); }
  inline bool empty() { return m_block_pool.empty(); }
  inline void clear() { m_block_pool.clear(); }
  void setPool(const block_id_type &last_block_id, const hash_t &last_hash,
               block_height_type last_height, timestamp_t last_time);
  bool prepareBins(block_height_type t_height);
  void invalidateCaches();
  unblk_push_result_type push(Block &block, bool is_restore = false);
  bool getBlock(block_height_type t_height, const hash_t &t_prev_hash,
                const hash_t &t_hash, Block &ret_block);
  void getResolvedBlocks(std::vector<UnresolvedBlock> &resolved_blocks,
                         std::vector<std::string> &drop_blocks);
  nth_link_type getUnresolvedLowestLink();
  block_layer_t getBlockLayer(const std::string &block_id_b64);
  nth_link_type getMostPossibleLink();
  block_layer_t getMostPossibleBlockLayer();
  bool hasUnresolvedBlocks();
  void restorePool();

private:
  block_layer_t forwardBlockToLedgerAt(int bin_idx, int vector_idx);
  void forwardBlocksToLedgerFrom(int bin_idx, int vector_idx);
  json readBackupIds();
  void backupPool();
  BlockPosOnMap getLongestBlockPos();
  void updateConfirmLevel();
};
} // namespace gruut

#endif // GRUUT_ENTERPRISE_MERGER_UNRESOLVED_BLOCK_POOL_HPP
