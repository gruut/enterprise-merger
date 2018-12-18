#pragma once

#include "../../chain/types.hpp"
#include "../../config/config.hpp"
#include "../../services/block_validator.hpp"
#include "../../services/input_queue.hpp"
#include "../../services/output_queue.hpp"
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
  InputQueueAlt* m_inputQueue;
  OutputQueueAlt* m_outputQueue;
  Storage* m_storage;

  std::unique_ptr<boost::asio::deadline_timer> m_timer_msg_fetching;
  std::unique_ptr<boost::asio::deadline_timer> m_timer_sync_control;
  std::unique_ptr<boost::asio::io_service::strand> m_block_sync_strand;

  int m_my_last_height;
  std::string m_my_last_bhash;
  merger_id_type m_my_id;
  int m_first_recv_block_height{-1};

  std::function<void(int)> m_finish_callback;
  std::map<int, RcvBlock> m_recv_block_list;
  std::mutex m_block_list_mutex;

  timestamp_type m_last_task_time{0};

  bool m_sync_done{false};
  bool m_sync_fail{false};

public:
  BlockSynchronizer();

  void setMyID(const merger_id_type &my_ID);

  void startBlockSync(std::function<void(int)> callback);

private:
  bool pushMsgToBlockList(InputMsgEntry &input_msg_entry);

  bool sendBlockRequest(int height);

  bool validateBlock(int height);

  void saveBlock(int height);

  void blockSyncControl();

  void messageFetch();
};
} // namespace gruut