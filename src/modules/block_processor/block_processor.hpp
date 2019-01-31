#ifndef GRUUT_ENTERPRISE_MERGER_BLOCK_PROCESSOR_HPP
#define GRUUT_ENTERPRISE_MERGER_BLOCK_PROCESSOR_HPP

#include "../../chain/merkle_tree.hpp"
#include "../../chain/transaction.hpp"
#include "../../chain/types.hpp"
#include "../../utils/bytes_builder.hpp"
#include "../../utils/compressor.hpp"
#include "../../utils/ecdsa.hpp"
#include "../../utils/periodic_task.hpp"
#include "../../utils/sha256.hpp"

#include "../../services/input_queue.hpp"
#include "../../services/layered_storage.hpp"
#include "../../services/message_proxy.hpp"
#include "../../services/output_queue.hpp"
#include "../../services/setting.hpp"
#include "../../services/storage.hpp"

#include "../module.hpp"
#include "unresolved_block_pool.hpp"

#include <botan-2/botan/base64.h>
#include <botan-2/botan/buf_comp.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <vector>

namespace gruut {

struct BlockRequestRecord {
  std::string hash_b64;
  std::string prev_hash_b64;
  block_height_type height;
  id_type recv_id;
  timestamp_t request_time;
};

class BlockProcessor : public Module {
private:
  MessageProxy m_msg_proxy;
  Storage *m_storage;
  LayeredStorage *m_layered_storage;
  std::string m_my_id_b64;
  std::string m_my_chain_id_b64;
  UnresolvedBlockPool m_unresolved_block_pool;
  std::list<BlockRequestRecord> m_request_list;
  std::recursive_mutex m_request_mutex;

  PeriodicTask m_task_scheduler;

public:
  BlockProcessor();
  ~BlockProcessor() = default;

  void start() override;

  void handleMessage(InputMsgEntry &entry);
  block_height_type handleMsgBlock(InputMsgEntry &entry);

  block_layer_t getBlockLayer(const std::string &block_id_b64);
  nth_link_type getMostPossibleLink();
  bool hasUnresolvedBlocks();

private:
  void periodicTask();
  void handleMsgReqBlock(InputMsgEntry &entry);
  void handleMsgRequestHeader(InputMsgEntry &entry);
  void handleMsgReqCheck(InputMsgEntry &entry);
  void handleMsgReqStatus(InputMsgEntry &entry);
  void sendErrorMessage(ErrorMsgType t_error_typem, id_type &recv_id);
  void resolveBlocksIf();
};
} // namespace gruut

#endif // GRUUT_ENTERPRISE_MERGER_BLOCK_PROCESSOR_HPP
