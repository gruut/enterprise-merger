#ifndef GRUUT_ENTERPRISE_MERGER_UNRESOLVED_BLOCK_POOL_HPP
#define GRUUT_ENTERPRISE_MERGER_UNRESOLVED_BLOCK_POOL_HPP

#include "omp_hash_map.h"
#include "easy_logging.hpp"

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
  int prev_queue_idx {-1};
  size_t confirm_level {0} ;
  UnresolvedBlock() = default;
  ~UnresolvedBlock() = default;
  UnresolvedBlock(Block &block_, int prev_queue_idx_, size_t confirm_level_)
      : block(block_), prev_queue_idx(prev_queue_idx_), confirm_level(confirm_level_) {

  }
};

class UnresolvedBlockPool {
private:
  std::deque< std::vector<UnresolvedBlock> > m_block_pool; // bin structure
  std::mutex m_push_mutex;

  std::string m_last_block_id_b64;
  std::string m_last_hash_b64;
  block_height_type m_last_height;

public:
  UnresolvedBlockPool(){
    el::Loggers::getLogger("URBK");
  };

  UnresolvedBlockPool(std::string &last_block_id_b64, std::string &last_hash_b64,
                      block_height_type last_height) {
    setPool(last_block_id_b64,last_hash_b64,last_height);
    el::Loggers::getLogger("URBK");
  }

  void setPool(std::string &last_block_id_b64, std::string &last_hash_b64,
               block_height_type last_height) {
    m_last_block_id_b64 = last_block_id_b64;
    m_last_hash_b64 = last_hash_b64;
    m_last_height = last_height;
  }

  bool push(Block &block) {

    block_height_type block_height = block.getHeight();

    std::lock_guard<std::mutex> guard(m_push_mutex);

    if(m_last_height <= block_height) {
      return false;
    }

    size_t bin_pos = block_height - m_last_height - 1; // e.g., 0 = 2 - 1 - 1

    if(m_block_pool.size() < bin_pos+1){
      m_block_pool.resize(bin_pos+1);
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
      if (bin_pos > 0) { // if there is previous bin

        size_t idx = 0;
        for(auto&& each_block : m_block_pool[bin_pos - 1]){
          if (each_block.block.getBlockIdB64() == block.getPrevBlockIdB64() &&
              each_block.block.getHashB64() == block.getPrevHashB64()) {
            prev_queue_idx = static_cast<int>(idx);
            break;
          }
          ++idx;
        }
      } else { // no previous
        if(block.getPrevBlockIdB64() == m_last_block_id_b64 && block.getPrevHashB64() == m_last_hash_b64) {
          prev_queue_idx = 0;
        } else {
          return false; // drop block -- this is not linkable block!
        }
      }

      m_block_pool[bin_pos].emplace_back(block, prev_queue_idx, 0);

      int queue_idx = m_block_pool[bin_pos].size() - 1; // last

      if (bin_pos + 1 < m_block_pool.size()) { // if there is next bin
        for (auto&& each_block : m_block_pool[bin_pos + 1]) {
          if (each_block.block.getPrevBlockIdB64() == block.getBlockIdB64() &&
              each_block.block.getPrevHashB64() == block.getHashB64()) {
            each_block.prev_queue_idx = queue_idx;
          }
        }
      }
    }

    m_push_mutex.unlock();

    return true;
  }

  std::vector<Block> getResolvedBlocks(){
    std::vector<Block> blocks;
    std::lock_guard<std::mutex> guard(m_push_mutex);
    resolveBlocksStepByStep(blocks);
    m_push_mutex.unlock();
    return blocks;
  }

  block_height_type getUnresolvedLowestHeight(){ // must be called after getResolvedBlocks()
    std::lock_guard<std::mutex> guard(m_push_mutex);

    int unresolved_lowest_height = -1;
    for(size_t i = 0; i < m_block_pool.size(); ++i){
      if(m_block_pool[i].empty()) { // empty, but reserved
        unresolved_lowest_height = static_cast<int>(i);
        break;
      } else {
        bool is_some = false;
        for (auto&& each_block : m_block_pool[i]) {
          if (each_block.prev_queue_idx != -1) {
            is_some = true;
            break;
          }
        }

        if(!is_some) {
          unresolved_lowest_height = static_cast<int>(i);
          break;
        }
      }
    }

    if(unresolved_lowest_height < 0)
      return 0;

    return unresolved_lowest_height + m_last_height;
  }

private:

  void resolveBlocksStepByStep(std::vector<Block> &resolved_blocks){

    updateConfirmLevel();

    if(!m_block_pool.empty() && !m_block_pool[0].empty()) {

      size_t highest_confirm_level = 0;
      auto highest_confirm_level_it = m_block_pool[0].begin();

      bool is_some = false;
      for(auto it_list = m_block_pool[0].begin(); it_list != m_block_pool[0].end(); ++it_list){
        if(it_list->prev_queue_idx == 0 && it_list->confirm_level > highest_confirm_level) {
          highest_confirm_level = it_list->confirm_level;
          highest_confirm_level_it = it_list;
          is_some = true;
        }
      }

      if(!is_some) { // nothing to do
        return;
      }

      if(highest_confirm_level > config::MAX_SIGNATURE_COLLECT_SIZE) { // at lease 1 confirmation is needed

        m_last_block_id_b64 = highest_confirm_level_it->block.getBlockIdB64();
        m_last_hash_b64 = highest_confirm_level_it->block.getHashB64();
        m_last_height = highest_confirm_level_it->block.getHeight();

        resolved_blocks.emplace_back(highest_confirm_level_it->block);

        // clear this height list

        if(m_block_pool[0].size() > 1)
          CLOG(INFO, "BPRO") << "Dropped out " << m_block_pool[0].size() - 1 << " unresolved block(s)";

        m_block_pool.pop_front();

        if(!m_block_pool.empty()) {

          for (auto&& each_block : m_block_pool[0]) {
            if(each_block.block.getPrevBlockIdB64() == m_last_block_id_b64 && each_block.block.getPrevHashB64() == m_last_hash_b64) {
              each_block.prev_queue_idx = 0;
            } else {
              each_block.prev_queue_idx = -1; // this block is unlinkable
            }
          }

          // call recursively
          resolveBlocksStepByStep(resolved_blocks);
        }
      }
    }
  }

  void updateConfirmLevel(){
    if(m_block_pool.empty())
      return;

    for(auto &each_height : m_block_pool){
      for(auto &each_block : each_height){
        each_block.confirm_level = each_block.block.getNumSSigs();
      }
    }

    for(size_t i = m_block_pool.size() - 1; i >= 0; --i){
      for(auto&& each_block : m_block_pool[i]){ // for list
        if(i > 0 && each_block.prev_queue_idx >= 0 && m_block_pool[i-1].size() > each_block.prev_queue_idx) {
          size_t idx = 0;
          for(auto&& prev_each_block :  m_block_pool[i-1]) { // for list
            if(idx == each_block.prev_queue_idx) {
              prev_each_block.confirm_level += each_block.block.getNumSSigs();
              break;
            }
            ++idx;
          }
        }
      }
    }
  }


};
} // namespace gruut

#endif // GRUUT_ENTERPRISE_MERGER_UNRESOLVED_BLOCK_POOL_HPP
