#include "unresolved_block_pool.hpp"
#include "../../application.hpp"
#include "easy_logging.hpp"

namespace gruut {

UnresolvedBlockPool::UnresolvedBlockPool() {
  m_layered_storage = LayeredStorage::getInstance();
  el::Loggers::getLogger("URBK");
}

void UnresolvedBlockPool::invalidateCaches() {
  m_cache_link_valid = false;
  m_cache_layer_valid = false;
  m_cache_pos_valid = false;
}

bool UnresolvedBlockPool::hasUnresolvedBlocks() {
  CLOG(INFO, "URBK") << "Unresolved block pool status = (" << m_last_height
                     << "," << m_height_range_max << ")";
  return m_force_unresolved;
}

void UnresolvedBlockPool::setPool(const block_id_type &last_block_id,
                                  const hash_t &last_hash,
                                  block_height_type last_height,
                                  timestamp_t last_time) {
  m_last_block_id = last_block_id;
  m_last_hash = last_hash;
  m_last_height = last_height;
  m_last_time = last_time;
  m_height_range_max = last_height;
}

bool UnresolvedBlockPool::prepareBins(block_height_type t_height) {
  std::lock_guard<std::recursive_mutex> guard(m_push_mutex);
  if (m_last_height >= t_height) {
    return false;
  }

  if ((Time::now_int() - m_last_time) <
      (t_height - m_last_height - 1) * config::BP_INTERVAL) {
    return false;
  }

  int bin_pos =
      static_cast<int>(t_height - m_last_height) - 1; // e.g., 0 = 2 - 1 - 1

  if (m_block_pool.size() < bin_pos + 1) {
    m_block_pool.resize(bin_pos + 1);
  }

  return true;
}

// we assume this block has valid structure at least
unblk_push_result_type UnresolvedBlockPool::push(Block &block,
                                                 bool is_restore) {
  unblk_push_result_type ret_val;
  ret_val.height = 0;
  ret_val.linked = false;
  ret_val.block_layer = {};

  std::lock_guard<std::recursive_mutex> guard(m_push_mutex);

  block_height_type block_height = block.getHeight();
  int bin_idx =
      static_cast<int>(block_height - m_last_height) - 1; // e.g., 0 = 2 - 1 - 1
  if (!prepareBins(block_height))
    return ret_val;

  for (auto &each_block : m_block_pool[bin_idx]) {
    if (each_block.block == block) {
      return ret_val;
    }
  }

  int prev_queue_idx = -1; // no previous

  if (bin_idx > 0) { // if there is previous bin
    size_t idx = 0;
    for (auto &each_block : m_block_pool[bin_idx - 1]) {
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
      // drop block -- this is not linkable block!
      return ret_val;
    }
  }

  int queue_idx = m_block_pool[bin_idx].size();

  m_block_pool[bin_idx].emplace_back(block, prev_queue_idx, 0, false);

  block_layer_t block_layer_of_this;
  if (bin_idx > 0)
    block_layer_of_this = getBlockLayer(block.getBlockIdB64());

  ret_val.height = block_height;
  ret_val.linked = (bin_idx == 0) ? true : !block_layer_of_this.empty();
  ret_val.block_layer = block_layer_of_this;

  m_block_pool[bin_idx][queue_idx].block_layer = block_layer_of_this;
  m_block_pool[bin_idx][queue_idx].linked = ret_val.linked;

  if (!is_restore)
    backupPool();

  if (bin_idx + 1 < m_block_pool.size()) { // if there is next bin
    for (auto &each_block : m_block_pool[bin_idx + 1]) {
      if (each_block.block.getPrevBlockId() == block.getBlockId() &&
          each_block.block.getPrevHash() == block.getHash()) {
        if (each_block.prev_vector_idx < 0) {
          each_block.prev_vector_idx = queue_idx;
        }
      }
    }
  }

  if (ret_val.linked) {
    forwardBlocksToLedgerFrom(bin_idx, queue_idx);
  }

  if (m_height_range_max < block_height) {
    m_height_range_max = block_height;
  }

  invalidateCaches();

  m_layered_storage->setBlockLayer(getMostPossibleBlockLayer());

  m_push_mutex.unlock();

  return ret_val;
}

block_layer_t UnresolvedBlockPool::forwardBlockToLedgerAt(int bin_idx,
                                                          int vector_idx) {
  if (bin_idx < 0 || m_block_pool.size() < bin_idx + 1 ||
      m_block_pool[bin_idx].size() < vector_idx + 1)
    return {};

  auto &t_block = m_block_pool[bin_idx][vector_idx];
  Application::app().getCustomLedgerManager().procLedgerBlock(
      t_block.block.getBlockBodyJson(), t_block.block.getBlockIdB64(),
      t_block.block_layer);

  return t_block.block_layer;
}

void UnresolvedBlockPool::forwardBlocksToLedgerFrom(int bin_idx,
                                                    int vector_idx) {

  if (bin_idx < 0 || m_block_pool.size() < bin_idx + 1 ||
      m_block_pool[bin_idx].size() < vector_idx + 1)
    return;

  std::function<void(size_t, size_t, block_layer_t)> recBlockToLedger;
  recBlockToLedger = [this, &recBlockToLedger](size_t bin_idx,
                                               size_t prev_vector_idx,
                                               block_layer_t block_layer) {
    if (bin_idx < 0 || m_block_pool.size() < bin_idx + 1)
      return;

    for (size_t i = 0; i < m_block_pool[bin_idx].size(); ++i) {
      auto &each_block = m_block_pool[bin_idx][i];
      if (each_block.prev_vector_idx == prev_vector_idx) {
        each_block.block_layer = block_layer;
        block_layer.emplace_back(each_block.block.getBlockIdB64());
        forwardBlockToLedgerAt(bin_idx, i);
        recBlockToLedger(bin_idx + 1, i, block_layer);
      }
    }
  };

  block_layer_t block_layer = forwardBlockToLedgerAt(bin_idx, vector_idx);
  recBlockToLedger(bin_idx + 1, vector_idx, block_layer);
}

bool UnresolvedBlockPool::getBlock(block_height_type t_height,
                                   const hash_t &t_prev_hash,
                                   const hash_t &t_hash, Block &ret_block) {

  std::lock_guard<std::recursive_mutex> guard(m_push_mutex);
  if (m_block_pool.empty()) {
    return false;
  }

  if (t_height <= m_last_height ||
      m_last_height + m_block_pool.size() < t_height) {
    return false;
  }

  int bin_pos = static_cast<int>(t_height - m_last_height) - 1;

  hash_t block_hash = t_hash;
  hash_t prev_hash = t_prev_hash;

  if (t_height == 0) {
    nth_link_type most_possible_link = getMostPossibleLink();
    bin_pos = static_cast<int>(most_possible_link.height - m_last_height) - 1;
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

// flushing out resolved blocks
void UnresolvedBlockPool::getResolvedBlocks(
    std::vector<UnresolvedBlock> &resolved_blocks,
    std::vector<std::string> &drop_blocks) {
  resolved_blocks.clear();
  drop_blocks.clear();

  std::lock_guard<std::recursive_mutex> guard(m_push_mutex);

  size_t num_resolved_block = 0;

  auto resolveBlocksStepByStep =
      [this](std::vector<UnresolvedBlock> &resolved_blocks,
             std::vector<std::string> &drop_blocks) {
        updateConfirmLevel();

        if (m_block_pool.size() < 2 || m_block_pool[0].empty() ||
            m_block_pool[1].empty())
          return;

        size_t highest_confirm_level = 0;
        int resolved_block_idx = -1;

        for (size_t i = 0; i < m_block_pool[0].size(); ++i) {
          if (m_block_pool[0][i].prev_vector_idx == 0 &&
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
          if (each_block.prev_vector_idx ==
              resolved_block_idx) { // some block links this block
            is_after = true;
            break;
          }
        }

        if (!is_after)
          return;

        m_last_block_id =
            m_block_pool[0][resolved_block_idx].block.getBlockId();
        m_last_hash = m_block_pool[0][resolved_block_idx].block.getHash();
        m_last_height = m_block_pool[0][resolved_block_idx].block.getHeight();

        resolved_blocks.emplace_back(m_block_pool[0][resolved_block_idx]);

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
            each_block.prev_vector_idx = 0;
          } else {
            // this block is unlinkable => to be deleted
            each_block.prev_vector_idx = -1;
          }
        }
      };

  do {
    num_resolved_block = resolved_blocks.size();
    resolveBlocksStepByStep(resolved_blocks, drop_blocks);
  } while (num_resolved_block < resolved_blocks.size());

  m_push_mutex.unlock();

  auto storage = Storage::getInstance();

  json id_array = readBackupIds();

  if (id_array.empty() || !id_array.is_array())
    return;

  json del_id_array = json::array();
  json new_id_array = json::array();

  for (auto &each_id : id_array) {
    bool is_dux = false;
    std::string block_id_b64 = Safe::getString(each_id);
    if (block_id_b64.empty())
      continue;

    for (auto &each_block : resolved_blocks) {
      if (block_id_b64 == each_block.block.getBlockIdB64()) {
        is_dux = true;
        break;
      }
    }

    if (!is_dux) {
      new_id_array.push_back(block_id_b64);
    } else {
      del_id_array.push_back(block_id_b64);
    }
  }

  storage->saveBackup(
      UNRESOLVED_BLOCK_IDS_KEY,
      TypeConverter::bytesToString(json::to_cbor(new_id_array)));

  for (auto &each_id : del_id_array) {
    std::string block_id_b64 = Safe::getString(each_id);
    storage->delBackup(block_id_b64);
  }

  storage->flushBackup();
}

nth_link_type UnresolvedBlockPool::getUnresolvedLowestLink() {
  BlockPosOnMap longest_pos = getLongestBlockPos();

  nth_link_type ret_link;
  ret_link.height = 0; // no unresolved block or invalid link info

  if (m_last_height <= longest_pos.height &&
      longest_pos.height < m_block_pool.size() + m_last_height) {

    int bin_idx = static_cast<int>(longest_pos.height - m_last_height) - 1;

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

block_layer_t
UnresolvedBlockPool::getBlockLayer(const std::string &block_id_b64) {
  block_layer_t block_layer_of_this;
  int bin_idx = -1;
  int queue_idx = -1;

  if (!block_id_b64.empty()) {
    for (size_t i = 0; i < m_block_pool.size(); ++i) {
      for (size_t j = 0; j < m_block_pool[j].size(); ++j) {

        if (block_id_b64 == m_block_pool[i][j].block.getBlockIdB64()) {
          bin_idx = static_cast<int>(i);
          queue_idx = static_cast<int>(j);
          break;
        }
      }
      if (bin_idx >= 0 && queue_idx >= 0)
        break;
    }

    for (int i = bin_idx; i >= 0; --i) {
      std::string new_block_id_b64 =
          m_block_pool[i][queue_idx].block.getBlockIdB64();
      if (block_id_b64 != new_block_id_b64)
        block_layer_of_this.emplace_back(new_block_id_b64);

      queue_idx = m_block_pool[i][queue_idx].prev_vector_idx;
      if (queue_idx < 0) {
        block_layer_of_this.clear();
        break;
      }
    }
  }

  return block_layer_of_this;
}

nth_link_type UnresolvedBlockPool::getMostPossibleLink() {

  if (m_cache_link_valid)
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

    // must be larger or equal to 0
    int deque_idx = static_cast<int>(longest_pos.height - m_last_height) - 1;

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
block_layer_t UnresolvedBlockPool::getMostPossibleBlockLayer() {

  if (m_cache_layer_valid) {
    return m_cache_possible_block_layer;
  }

  auto t_block_pos = getLongestBlockPos();
  std::vector<std::string> ret_block_layer;

  int queue_idx = static_cast<int>(t_block_pos.vector_idx);

  int bin_end = static_cast<int>(t_block_pos.height - m_last_height) - 1;

  for (int i = bin_end; i >= 0; --i) {
    ret_block_layer.emplace_back(
        m_block_pool[i][queue_idx].block.getBlockIdB64());
    queue_idx = m_block_pool[i][queue_idx].prev_vector_idx;
  }

  m_cache_possible_block_layer = ret_block_layer;
  m_cache_layer_valid = true;
  return ret_block_layer;
}

void UnresolvedBlockPool::restorePool() {

  json id_array = readBackupIds();
  if (id_array.empty() || !id_array.is_array())
    return;

  auto storage = Storage::getInstance();

  size_t num_pused_block = 0;

  for (auto &id_each : id_array) {
    std::string block_id_b64 = Safe::getString(id_each);
    if (!block_id_b64.empty()) {
      std::string serialized_block = storage->readBackup(block_id_b64);
      if (serialized_block.empty()) {
        CLOG(ERROR, "URBK") << "Failed to read block [" << block_id_b64 << "]";
        continue;
      }

      Block new_block;
      if (!new_block.deserialize(serialized_block)) {
        CLOG(ERROR, "URBK")
            << "Failed to deserialize block [" << block_id_b64 << "]";
        continue;
      }

      auto push_result = push(new_block, true);
      if (push_result.height == 0) {
        CLOG(ERROR, "URBK")
            << "Failed to restore block [" << block_id_b64 << "]";
      } else {
        ++num_pused_block;
      }
    }
  }

  CLOG(INFO, "URBK") << num_pused_block
                     << " unresolved block(s) have been restored.";
}

json UnresolvedBlockPool::readBackupIds() {
  json id_array = json::array();

  auto storage = Storage::getInstance();
  std::string backup_block_ids = storage->readBackup(UNRESOLVED_BLOCK_IDS_KEY);
  if (backup_block_ids.empty())
    return id_array;

  try {
    id_array = json::from_cbor(backup_block_ids);
  } catch (json::exception &e) {
    CLOG(ERROR, "URBK") << "Failed to restore unresolved pool - " << e.what();
    return id_array;
  }

  return id_array;
}

void UnresolvedBlockPool::backupPool() {

  json id_array = json::array();
  auto storage = Storage::getInstance();

  for (auto &each_level : m_block_pool) {
    for (auto &each_block : each_level) {
      std::string key = each_block.block.getBlockIdB64();
      storage->saveBackup(key, each_block.block.serialize());
      id_array.push_back(key);
    }
  }

  storage->saveBackup(UNRESOLVED_BLOCK_IDS_KEY,
                      TypeConverter::bytesToString(json::to_cbor(id_array)));

  storage->flushBackup();
}

BlockPosOnMap UnresolvedBlockPool::getLongestBlockPos() {
  if (m_cache_pos_valid)
    return m_cache_possible_pos;

  // search prev_level
  std::function<void(size_t, size_t, BlockPosOnMap &)> recSearchLongestLink;
  recSearchLongestLink = [this, &recSearchLongestLink](
                             size_t bin_idx, size_t prev_vector_idx,
                             BlockPosOnMap &longest_pos) {
    if (m_block_pool.size() < bin_idx + 1)
      return;

    // if this level is empty, it will automatically stop to search
    for (size_t i = 0; i < m_block_pool[bin_idx].size(); ++i) {
      auto &each_block = m_block_pool[bin_idx][i];
      if (each_block.prev_vector_idx == prev_vector_idx) {
        if (each_block.block.getHeight() > longest_pos.height) { // relaxation
          longest_pos.height = each_block.block.getHeight();
          longest_pos.vector_idx = i;
        }

        recSearchLongestLink(bin_idx + 1, i, longest_pos);
      }
    }
  };

  BlockPosOnMap longest_pos(0, 0);

  if (!m_block_pool.empty()) {
    for (size_t i = 0; i < m_block_pool[0].size(); ++i) {
      if (m_block_pool[0][i].prev_vector_idx == 0) {

        if (m_block_pool[0][i].block.getHeight() >
            longest_pos.height) { // relaxation
          longest_pos.height = m_block_pool[0][i].block.getHeight();
          longest_pos.vector_idx = i;
        }

        recSearchLongestLink(1, i, longest_pos); // recursive call
      }
    }
  }

  m_cache_possible_pos = longest_pos;
  m_cache_pos_valid = true;

  return longest_pos;
}

void UnresolvedBlockPool::updateConfirmLevel() {
  if (m_block_pool.empty())
    return;

  for (auto &each_level : m_block_pool) {
    for (auto &each_block : each_level) {
      each_block.confirm_level = each_block.block.getNumSSigs();
    }
  }

  for (int i = (int)m_block_pool.size() - 1; i > 0; --i) {
    for (auto &each_block : m_block_pool[i]) { // for vector
      if (each_block.prev_vector_idx >= 0 &&
          m_block_pool[i - 1].size() > each_block.prev_vector_idx) {
        m_block_pool[i - 1][each_block.prev_vector_idx].confirm_level +=
            each_block.confirm_level;
      }
    }
  }
}
} // namespace gruut