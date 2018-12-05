#pragma once

#include "../../application.hpp"
#include "../../chain/types.hpp"
#include "../../services/input_queue.hpp"
#include "../../services/output_queue.hpp"
#include "../../services/storage.hpp"
#include "../module.hpp"
#include "nlohmann/json.hpp"

#include <boost/asio.hpp>
#include <chrono>
#include <functional>
#include <iostream>
#include <queue>
#include <tuple>

namespace gruut {

class BlockSynchronizer : public Module {
private:
  InputQueueAlt *m_inputQueue;
  OutputQueueAlt *m_outputQueue;
  std::unique_ptr<boost::asio::deadline_timer> m_timer;

  int m_my_last_height;
  std::string m_my_last_bhash;
  Storage *m_storage;

  std::function<void(int)> m_finish_callback;

  std::vector<std::tuple<int, Sha256, nlohmann::json>> m_received_blocks;

public:
  BlockSynchronizer() {

    m_timer.reset(
        new boost::asio::deadline_timer(Application::app().getIoService()));

    m_storage = Storage::getInstance();
    m_inputQueue = InputQueueAlt::getInstance();
    m_outputQueue = OutputQueueAlt::getInstance();
  }

  void start() override { fetch(); }

  void fetch() {
    auto &io_service = Application::app().getIoService();
    io_service.post([this]() {
      // TODO: block sync가 끝날 때 까지 계속 돌아가야함
      auto input_msg_entry = m_inputQueue->fetch();
    });

    m_timer->expires_from_now(boost::posix_time::milliseconds(1000));
    m_timer->async_wait([this](const boost::system::error_code &ec) {
      if (ec == boost::asio::error::operation_aborted) {
      } else if (ec.value() == 0) {
        fetch();
      } else {
        std::cout << "ERROR: " << ec.message() << std::endl;
        throw;
      }
    });
  }

  bool blockSync(std::function<void(int)> callback) {
    std::cout << "block sync" << std::endl;

    std::pair<std::string, std::string> hash_and_height =
        m_storage->findLatestHashAndHeight();
    m_my_last_height = stoi(hash_and_height.second);
    m_my_last_bhash = hash_and_height.first;

    m_received_blocks.clear();

    OutputMsgEntry block_req_msg;
    block_req_msg.type = MessageType::MSG_REQ_BLOCK;
    block_req_msg.body["mID"] = "TUVSR0VSLTE=";
    block_req_msg.body["time"] = to_string(std::time(nullptr));
    block_req_msg.body["mCert"] = "";
    block_req_msg.body["hgt"] = "-1";
    block_req_msg.body["mSig"] = "";

    m_outputQueue->push(block_req_msg);

    m_finish_callback = callback;
  }
};
} // namespace gruut