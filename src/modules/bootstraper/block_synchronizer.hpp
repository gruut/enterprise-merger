#pragma once

#include "../../chain/types.hpp"
#include "../../config/config.hpp"
#include "../../services/block_validator.hpp"
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
#include <queue>
#include <random>
#include <tuple>
#include <vector>

namespace gruut {

struct RcvBlock {
  sha256 hash;
  nlohmann::json block_json;
  nlohmann::json txs;
  bytes block_raw;
  std::vector<sha256> mtree;
  std::string merger_id_b64;
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
  merger_id_type m_my_id;
  int m_first_recv_block_height{-1};

  std::function<void(ExitCode)> m_finish_callback;
  std::map<size_t, RcvBlock> m_recv_block_list;
  std::mutex m_block_list_mutex;

  timestamp_type m_last_task_time{0};

  bool m_sync_alone{true};

  bool m_sync_done{false};
  bool m_sync_fail{false};

public:
  BlockSynchronizer();

  void startBlockSync(std::function<void(ExitCode)> callback);

private:
  bool pushMsgToBlockList(InputMsgEntry &input_msg_entry);

  bool sendBlockRequest(int height);

  void sendErrorToSigner(InputMsgEntry &input_msg_entry);

  bool validateBlock(int height);

  void saveBlock(int height);

  void syncFinish();

  void blockSyncControl();

  void messageFetch();

  bool checkMsgFromOtherMerger(MessageType msg_type);

  bool checkMsgFromSigner(MessageType msg_type);
};
} // namespace gruut