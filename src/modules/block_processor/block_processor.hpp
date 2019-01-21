#ifndef GRUUT_ENTERPRISE_MERGER_BLOCK_PROCESSOR_HPP
#define GRUUT_ENTERPRISE_MERGER_BLOCK_PROCESSOR_HPP

#include "../../chain/merkle_tree.hpp"
#include "../../chain/transaction.hpp"
#include "../../chain/types.hpp"
#include "../../utils/bytes_builder.hpp"
#include "../../utils/compressor.hpp"
#include "../../utils/ecdsa.hpp"
#include "../../utils/sha256.hpp"

#include "../../services/input_queue.hpp"
#include "../../services/message_proxy.hpp"
#include "../../services/output_queue.hpp"
#include "../../services/setting.hpp"
#include "../../services/storage.hpp"
#include "../../services/layered_storage.hpp"

#include "../module.hpp"
#include "unresolved_block_pool.hpp"

#include <botan-2/botan/base64.h>
#include <botan-2/botan/buf_comp.h>

#include <algorithm>
#include <boost/asio/deadline_timer.hpp>
#include <cstring>
#include <iostream>
#include <vector>

namespace gruut {

class BlockProcessor : public Module {
private:
  MessageProxy m_msg_proxy;
  Storage *m_storage;
  LayeredStorage *m_layered_storage;
  merger_id_type m_my_id;
  UnresolvedBlockPool m_unresolved_block_pool;
  std::unique_ptr<boost::asio::deadline_timer> m_timer;
  std::unique_ptr<boost::asio::io_service::strand> m_task_strand;

public:
  BlockProcessor();
  ~BlockProcessor() = default;

  void start() override;

  void handleMessage(InputMsgEntry &entry);
  block_height_type handleMsgBlock(InputMsgEntry &entry);

  nth_link_type getMostPossibleLink();
  std::vector<std::string> getMostPossibleBlockLayer();
  bool hasUnresolvedBlocks();

private:
  void periodicTask();
  void handleMsgReqBlock(InputMsgEntry &entry);
  void handleMsgReqCheck(InputMsgEntry &entry);
  void sendErrorMessage(ErrorMsgType t_error_typem, id_type &recv_id);
};
} // namespace gruut

#endif // GRUUT_ENTERPRISE_MERGER_BLOCK_PROCESSOR_HPP
