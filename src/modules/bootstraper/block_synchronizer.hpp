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

class BlockSynchronizer {
private:
  InputQueueAlt *m_inputQueue;
  MessageProxy m_msg_proxy;

  std::unique_ptr<boost::asio::deadline_timer> m_msg_fetching_timer;
  std::unique_ptr<boost::asio::deadline_timer> m_sync_ctrl_timer;
  std::unique_ptr<boost::asio::io_service::strand> m_block_sync_strand;

  merger_id_type m_my_id;

  std::function<void(ExitCode)> m_finish_callback;

  std::once_flag m_end_sync_call_flag;

  std::atomic<bool> m_sync_alone{true};
  std::atomic<bool> m_sync_done{false};
  std::atomic<bool> m_sync_fail{false};

  nth_link_type m_link_from;
  std::vector<bool> m_sync_map;

public:
  BlockSynchronizer();

  void startBlockSync(std::function<void(ExitCode)> callback);

private:
  void sendRequestBlock(size_t height);
  void sendRequestStatus();
  void sendErrorToSigner(InputMsgEntry &input_msg_entry);
  void syncFinish();
  void blockSyncControl();
  void messageFetch();
  bool checkMsgFromOtherMerger(MessageType msg_type);
  bool checkMsgFromSigner(MessageType msg_type);
};
} // namespace gruut

#endif