#ifndef GRUUT_ENTERPRISE_MERGER_BLOCK_SYNCHRONIZER_HPP
#define GRUUT_ENTERPRISE_MERGER_BLOCK_SYNCHRONIZER_HPP

#include "../../chain/block.hpp"
#include "../../chain/types.hpp"
#include "../../config/config.hpp"
#include "../../services/input_queue.hpp"
#include "../../services/message_proxy.hpp"
#include "../../services/storage.hpp"
#include "../../utils/bytes_builder.hpp"
#include "../../utils/compressor.hpp"
#include "../../utils/random_number_generator.hpp"
#include "../../utils/rsa.hpp"
#include "../../utils/sha256.hpp"
#include "../../utils/type_converter.hpp"
#include "nlohmann/json.hpp"

#include <boost/asio.hpp>
#include <chrono>
#include <ctime>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <queue>
#include <random>
#include <tuple>
#include <vector>

namespace gruut {

struct RcvBlockMapItem {
  std::string merger_id_b64;
  Block block;
  int num_retry{0};
  BlockState state{BlockState::RECEIVED};
};

class BlockSynchronizer {
private:
  InputQueueAlt *m_inputQueue;
  Storage *m_storage;
  MessageProxy m_msg_proxy;

  std::unique_ptr<boost::asio::deadline_timer> m_msg_fetching_timer;
  std::unique_ptr<boost::asio::deadline_timer> m_sync_ctrl_timer;
  std::unique_ptr<boost::asio::io_service::strand> m_block_sync_strand;

  size_t m_my_last_height;
  std::string m_my_last_blk_hash_b64;
  std::string m_my_last_blk_id_b64;
  merger_id_type m_my_id;
  size_t m_first_recv_block_height{0};

  std::function<void(ExitCode)> m_finish_callback;
  std::map<size_t, RcvBlockMapItem> m_recv_block_list;
  std::mutex m_block_list_mutex;

  timestamp_type m_last_task_time{0};

  std::once_flag m_end_sync_call_flag;

  bool m_sync_alone{true};

  bool m_sync_done{false};
  bool m_sync_fail{false};

public:
  BlockSynchronizer();

  void startBlockSync(std::function<void(ExitCode)> callback);

private:
  void reserveBlockList(size_t begin, size_t end);
  bool pushMsgToBlockList(InputMsgEntry &msg_block);
  bool sendBlockRequest(size_t height);
  void sendErrorToSigner(InputMsgEntry &input_msg_entry);
  bool validateBlock(size_t height);
  void saveBlock(size_t height);
  void syncFinish();
  void blockSyncControl();
  void messageFetch();
  bool checkMsgFromOtherMerger(MessageType msg_type);
  bool checkMsgFromSigner(MessageType msg_type);
  void updateTaskTime();
};
} // namespace gruut

#endif