#pragma once

#include "../../application.hpp"
#include "../../chain/types.hpp"
#include "../../services/input_queue.hpp"
#include "../../services/output_queue.hpp"
#include "../../services/storage.hpp"
#include "../../utils/bytes_builder.hpp"
#include "../../utils/compressor.hpp"
#include "../../utils/rsa.hpp"
#include "../../utils/sha256.hpp"
#include "../module.hpp"
#include "nlohmann/json.hpp"

#include "../../services/block_validator.hpp"

#include <boost/asio.hpp>
#include <chrono>
#include <ctime>
#include <functional>
#include <iostream>
#include <map>
#include <queue>
#include <tuple>
#include <vector>

namespace gruut {

const int MAX_REQ_BLOCK_RETRY = 5;

struct RcvBlock {
  sha256 hash;
  nlohmann::json block_json;
  nlohmann::json txs;
  std::vector<uint8_t> block_raw;
  std::vector<sha256> mtree;
  std::string mID;
  time_t req_time;
  int num_retry{0};
};

class BlockSynchronizer : public Module {
private:
  InputQueueAlt *m_inputQueue;
  OutputQueueAlt *m_outputQueue;
  Storage *m_storage;

  std::unique_ptr<boost::asio::deadline_timer> m_timer_msg_fetching;
  std::unique_ptr<boost::asio::deadline_timer> m_timer_req_block_control;
  std::unique_ptr<boost::asio::deadline_timer> m_timer_callback;

  int m_my_last_height;        // db에서 가져온 최신 블록의 height
  std::string m_my_last_bhash; // db에서 가져온 최신 블록의 hash
  int m_block_hgt;             // 최신 블록의 hgt
  int m_empty_blk_hgt;

  std::function<void(int)> m_finish_callback;
  std::map<int, RcvBlock> m_block_list; // hgt
  std::mutex m_block_list_mutex;

  bool m_sync_done{false};

  // TODO : 1.1 BLOCK를 map에 저장 (중복 : drop)
  // TODO : 1.2 BLOCK list에서 시간이 너무 오래된 BLk이면 mID 초기화(안보낸
  // 걸로)
  // TODO : 2.1 비어있는 블록 중에 가장 높은 hgt를 가진 블록 찾기 : req보내지
  // 않은 것 중
  // TODO : 2.2 MSG_REQ_BLOCK을 지금 BLOCK을 보낸 머저에게 날리고, 없으면 안함
  // TODO : 3. 진짜 아무것도 없으면 블록 유효성 검사 시작

  bool blockProcess(InputMsgEntry &input_msg_entry) {
    if (!pushMsgToBlockList(input_msg_entry))
      return false;

    int max_height = findEmptyMaxHgtBlock();

    if (max_height == -1) {
      if (blockValidateAll()) {
        m_sync_done = true;

        m_timer_msg_fetching->cancel();
        m_timer_req_block_control->cancel();

        m_timer_callback->expires_from_now(
            boost::posix_time::milliseconds(1000));
        m_timer_callback->async_wait(
            [this](const boost::system::error_code &ec) {
              if (ec == boost::asio::error::operation_aborted) {
              } else if (ec.value() == 0) {
                m_finish_callback(1);
              } else {
                std::cout << "ERROR: " << ec.message() << std::endl;
                throw;
              }
            });
      }
    } else {
      sendBlockRequest(max_height);
    }
  }

  bool blockValidateAll() {

    for (auto it_map = m_block_list.begin(); it_map != m_block_list.end();
         ++it_map) {
      // it_map->first
      // it_map->second

      std::vector<sha256> mtree;

      if (BlockValidator::validate(it_map->second.block_json,
                                   it_map->second.txs, mtree)) {
        it_map->second.mtree = mtree;
      } else {
        std::lock_guard<std::mutex> lock(m_block_list_mutex);

        it_map->second.mID = "";
        it_map->second.hash = {};
        it_map->second.block_raw = {};
        it_map->second.req_time = 0;
        it_map->second.block_json = "{}"_json;
        it_map->second.txs = "{}"_json;
        it_map->second.mtree = {};
        it_map->second.num_retry += 1;

        m_block_list_mutex.unlock();

        if (it_map->second.num_retry <= MAX_REQ_BLOCK_RETRY)
          sendBlockRequest(it_map->first);
      }
    }
  }

  // TODO : 1.1 BLOCK를 map에 저장 (중복 : drop)
  // TODO : 1.2 BLOCK list에서 시간이 너무 오래된 BLock이면 mID 초기화(안보낸
  // 걸로)
  bool pushMsgToBlockList(InputMsgEntry &input_msg_entry) {

    std::string block_raw_str = macaron::Base64::Decode(
        input_msg_entry.body["blockraw"].get<std::string>());
    std::vector<uint8_t> block_raw(block_raw_str.begin(), block_raw_str.end());

    nlohmann::json block_json = BlockValidator::getBlockJson(block_raw);

    if (block_json.empty()) {
      return false;
    }

    int block_hgt = stoi(block_json["hgt"].get<std::string>());

    if (block_hgt <= m_my_last_height) {
      return false;
    }

    if (m_block_list.find(block_hgt) == m_block_list.end()) {
      RcvBlock temp;
      temp.mID = block_json["mID"].get<std::string>();
      temp.hash = Sha256::hash(block_raw);
      temp.block_raw = std::move(block_raw);
      temp.req_time = stoi(block_json["time"].get<std::string>());
      temp.block_json = std::move(block_json);
      temp.txs =
          nlohmann::json::parse(input_msg_entry.body["tx"].get<std::string>());
      // temp.mtree = ....

      std::lock_guard<std::mutex> lock(m_block_list_mutex);
      m_block_list.insert(make_pair(m_block_hgt, temp));
      m_block_list_mutex.unlock();
      return true;
    }
  }

  // TODO : 2.1 비어있는 블록 중에 가장 높은 hgt를 가진 블록 찾기 : req보내지
  // 않은 것 중
  int findEmptyMaxHgtBlock() {
    for (int i = m_block_hgt; i > m_my_last_height; --i) {
      if (!m_block_list.count(i) && (m_block_list.find(i)->second.mID == "")) {
        m_empty_blk_hgt = i;
        return m_empty_blk_hgt;
      }
    }
  }

  // TODO : 2.2 MSG_REQ_BLOCK을 지금 BLOCK을 보낸 머저에게 날리고, 없으면 안함
  bool sendBlockRequest(int height = -1) {

    std::vector<std::string> receivers = {};

    if (height != -1) {
      // TODO : find a suitable receiver
    }

    OutputMsgEntry msg_req_block;

    msg_req_block.type = MessageType::MSG_REQ_BLOCK;
    msg_req_block.body["mID"] = "TUVSR0VSLTE=";
    msg_req_block.body["time"] = to_string(std::time(nullptr));
    msg_req_block.body["mCert"] = "";
    msg_req_block.body["hgt"] = std::to_string(height - 1); // hgt 설정
    msg_req_block.body["mSig"] = "";
    msg_req_block.receivers =
        receivers; // msg_req_block은 제일 처음 -1일 경우 제외하고는 하나의
                   // merger에게 보내는 것 아님??

    m_outputQueue->push(msg_req_block);
  }

  // TODO : 3. 진짜 아무것도 없으면 블록 유효성 검사 시작
  // TODO : false -> 다시 msg_req (hgt에 해당하는) -> 다른 머저에게?
  bool saveBlock(int height) {

    auto it_map = m_block_list.find(height);
    if (it_map == m_block_list.end()) {
      return false;
    }

    nlohmann::json block_body;
    block_body["tx"] = m_block_list[height].txs;
    block_body["txCnt"] = m_block_list[height].txs.size();
    nlohmann::json mtree(m_block_list[height].mtree.size(), "");
    for (size_t i = 0; i < m_block_list[height].mtree.size(); ++i) {
      mtree[i] = m_block_list[height].mtree[i];
    }
    block_body["mtree"] = mtree;

    m_storage->saveBlock(std::string(m_block_list[height].block_raw.begin(),
                                     m_block_list[height].block_raw.end()),
                         m_block_list[height].block_json, block_body);

    std::lock_guard<std::mutex> lock(m_block_list_mutex);
    m_block_list.erase(m_block_hgt);
    m_block_list_mutex.unlock();

    return true;
  }

  void reqBlockControl() {
    if (m_sync_done)
      return;

    auto &io_service = Application::app().getIoService();
    io_service.post([this]() {
      // TODO : block request based on pre-requesting time (retry)
    });

    m_timer_req_block_control->expires_from_now(
        boost::posix_time::milliseconds(1000));
    m_timer_req_block_control->async_wait(
        [this](const boost::system::error_code &ec) {
          if (ec == boost::asio::error::operation_aborted) {
          } else if (ec.value() == 0) {
            reqBlockControl();
          } else {
            std::cout << "ERROR: " << ec.message() << std::endl;
            throw;
          }
        });
  }

  void messageFetch() {
    if (m_sync_done)
      return;

    auto &io_service = Application::app().getIoService();
    io_service.post([this]() {
      // TODO: message fetching from InputQueue and drop all except MSG_BLOCK

      InputMsgEntry input_msg_entry = m_inputQueue->fetch();
      if (input_msg_entry.type == MessageType::MSG_BLOCK) {
        blockProcess(input_msg_entry);
      }
    });

    m_timer_msg_fetching->expires_from_now(
        boost::posix_time::milliseconds(1000));
    m_timer_msg_fetching->async_wait(
        [this](const boost::system::error_code &ec) {
          if (ec == boost::asio::error::operation_aborted) {
          } else if (ec.value() == 0) {
            messageFetch();
          } else {
            std::cout << "ERROR: " << ec.message() << std::endl;
            throw;
          }
        });
  }

public:
  BlockSynchronizer() {

    auto &io_service = Application::app().getIoService();

    m_timer_msg_fetching.reset(new boost::asio::deadline_timer(io_service));
    m_timer_req_block_control.reset(
        new boost::asio::deadline_timer(io_service));
    m_timer_callback.reset(new boost::asio::deadline_timer(io_service));

    m_storage = Storage::getInstance();
    m_inputQueue = InputQueueAlt::getInstance();
    m_outputQueue = OutputQueueAlt::getInstance();
  }

  void start() override {
    messageFetch();    // inf-loop until SyncJobStage::SYNC_DONE
    reqBlockControl(); // inf-loop until SyncJobStage::BLOCK_FETCHING
  }

  bool startBlockSync(std::function<void(int)> callback) {
    std::cout << "block sync" << std::endl;

    std::pair<std::string, std::string> hash_and_height =
        m_storage->findLatestHashAndHeight();
    m_my_last_height = stoi(hash_and_height.second);
    m_my_last_bhash = hash_and_height.first;

    m_block_list.clear();

    sendBlockRequest(-1);

    m_finish_callback = callback;
  }
};
} // namespace gruut