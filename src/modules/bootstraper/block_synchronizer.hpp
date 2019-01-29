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
#include "../../utils/periodic_task.hpp"
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

struct OtherStatusData {
  std::string hash_b64;
  block_height_type height;
  OtherStatusData(std::string hash_b64_, block_height_type height_)
      : hash_b64(hash_b64_), height(height_) {}
};

class BlockSynchronizer {
private:
  InputQueueAlt *m_inputQueue;
  MessageProxy m_msg_proxy;
  merger_id_type m_my_id;

  PeriodicTask m_msg_fetch_scheduler;
  PeriodicTask m_sync_control_scheduler;
  DelayedTask m_sync_begin_scheduler;

  std::function<void(ExitCode)> m_sync_finish_callback;
  std::once_flag m_sync_finish_call_flag;

  std::atomic<bool> m_is_sync_begin{false};
  std::atomic<bool> m_is_sync_done{false};

  nth_link_type m_link_from;
  std::vector<bool> m_sync_flags;

  std::map<std::string, OtherStatusData> m_chain_status;

  std::mutex m_chain_mutex;
  std::mutex m_sync_flags_mutex;

public:
  BlockSynchronizer();

  void startBlockSync(std::function<void(ExitCode)> callback);

private:
  void collectingStatus(InputMsgEntry &entry);
  void sendRequestLastBlock();
  void sendRequestBlock(size_t height, const std::string &block_hash_b64,
                        const merger_id_type &t_merger);
  void sendRequestStatus();
  void sendErrorToSigner(InputMsgEntry &input_msg_entry);
  void syncFinish(ExitCode exit_code);
  void blockSyncControl();
  void messageFetch();
  bool checkMsgFromSigner(MessageType msg_type);
};
} // namespace gruut

#endif