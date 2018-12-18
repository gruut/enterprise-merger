#pragma once

#include "../../application.hpp"
#include "../../chain/types.hpp"
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
#include "../module.hpp"
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

const int MAX_REQ_BLOCK_RETRY = 5;
const int MAX_WAIT_TIME = 5;

enum class BlockState { RECEIVED, TOSAVE, TODELETE, RETRIED };

struct RcvBlock {
  sha256 hash;
  nlohmann::json block_json;
  nlohmann::json txs;
  std::vector<uint8_t> block_raw;
  std::vector<sha256> mtree;
  std::string mID;
  int num_retry{0};
  BlockState state{BlockState::RECEIVED};
};

class BlockSynchronizer : public Module {
private:
  InputQueueAlt *m_inputQueue;
  OutputQueueAlt *m_outputQueue;
  Storage *m_storage;

  std::unique_ptr<boost::asio::deadline_timer> m_timer_msg_fetching;
  std::unique_ptr<boost::asio::deadline_timer> m_timer_sync_control;
  std::unique_ptr<boost::asio::io_service::strand> m_block_sync_strand;

  int m_my_last_height;
  std::string m_my_last_bhash;
  merger_id_type m_id;
  int m_first_recv_block_height{-1};

  std::function<void(int)> m_finish_callback;
  std::map<int, RcvBlock> m_block_list;
  std::mutex m_block_list_mutex;

  timestamp_type m_last_task_time{0};

  bool m_sync_done{false};
  bool m_sync_fail{false};

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

    auto it_map = m_block_list.find(block_hgt);

    if (it_map == m_block_list.end()) {
      RcvBlock temp;
      temp.mID = block_json["mID"].get<std::string>();
      temp.hash = Sha256::hash(block_raw);
      temp.block_raw = std::move(block_raw);
      temp.block_json = std::move(block_json);
      temp.txs =
          nlohmann::json::parse(input_msg_entry.body["tx"].get<std::string>());

      std::lock_guard<std::mutex> lock(m_block_list_mutex);

      m_block_list.insert(make_pair(block_hgt, temp));

      m_block_list_mutex.unlock();
    } else if (it_map->second.state == BlockState::RETRIED) {
      std::lock_guard<std::mutex> lock(m_block_list_mutex);

      it_map->second.mID = block_json["mID"].get<std::string>();
      it_map->second.hash = Sha256::hash(block_raw);
      it_map->second.block_raw = std::move(block_raw);
      it_map->second.block_json = std::move(block_json);
      it_map->second.txs =
          nlohmann::json::parse(input_msg_entry.body["tx"].get<std::string>());
      it_map->second.mtree = {};

      m_block_list_mutex.unlock();
    } else {
      return false;
    }

    if (m_first_recv_block_height < 0)
      m_first_recv_block_height = block_hgt;

    return true;
  }

  bool sendBlockRequest(int height) {

    std::vector<std::string> receivers = {};

    if (height != -1) { // unicast

      if (m_block_list.empty()) { // but, no body
        return false;
      }

      std::vector<std::string> ans_merger_list;

      for (auto it_map = m_block_list.begin(); it_map != m_block_list.end();
           ++it_map) {
        ans_merger_list.emplace_back(it_map->second.mID);
      }

      std::random_device rd;
      std::mt19937 prng(rd);
      std::uniform_int_distribution<> dist(0,
                                           (int)(ans_merger_list.size() - 1));

      auto it_map = m_block_list.find(height + 1);
      receivers.emplace_back(ans_merger_list[dist(prng)]);
    }

    m_last_task_time = Time::now_int();

    OutputMsgEntry msg_req_block;

    msg_req_block.type = MessageType::MSG_REQ_BLOCK;
    msg_req_block.body["mID"] = m_id;
    msg_req_block.body["time"] = to_string(Time::now_int());
    msg_req_block.body["mCert"] = {};
    msg_req_block.body["hgt"] = std::to_string(height);
    msg_req_block.body["mSig"] = {};
    msg_req_block.receivers = receivers;

    m_outputQueue->push(msg_req_block);

    return true;
  }

  bool validateBlock(int height) {
    auto it_map = m_block_list.find(height);
    if (it_map == m_block_list.end())
      return false;

    std::vector<sha256> mtree;

    if (BlockValidator::validate(it_map->second.block_json, it_map->second.txs,
                                 mtree)) {
      it_map->second.mtree = mtree;
    }

    return true;
  }

  void saveBlock(int height) {

    auto it_map = m_block_list.find(height);
    if (it_map == m_block_list.end()) {
      return;
    }

    nlohmann::json block_body;
    block_body["tx"] = it_map->second.txs;
    block_body["txCnt"] = it_map->second.txs.size();
    nlohmann::json mtree(it_map->second.mtree.size(), "");
    for (size_t i = 0; i < it_map->second.mtree.size(); ++i) {
      mtree[i] = it_map->second.mtree[i];
    }
    block_body["mtree"] = mtree;

    m_storage->saveBlock(std::string(it_map->second.block_raw.begin(),
                                     it_map->second.block_raw.end()),
                         it_map->second.block_json, block_body);
  }

  void blockSyncControl() {
    if (m_sync_done)
      return;

    auto &io_service = Application::app().getIoService();
    io_service.post(m_block_sync_strand->wrap([this]() {
      if (m_block_list.empty()) {
        return;
      }

      // step 1 - validate min height block
      for (auto it_map = m_block_list.begin(); it_map != m_block_list.end();
           ++it_map) {
        if (it_map->first == m_my_last_height + 1) {
          if (it_map->second.block_json["prevH"].get<std::string>() ==
              m_my_last_bhash) {
            if (validateBlock(it_map->first)) {

              std::lock_guard<std::mutex> lock(m_block_list_mutex);
              it_map->second.state = BlockState::TOSAVE;
              m_block_list_mutex.unlock();

              //              m_my_last_bhash =
              //              macaron::Base64::Encode(std::string(
              //                  it_map->second.hash.begin(),
              //                  it_map->second.hash.end()));
              m_my_last_bhash = TypeConverter::toBase64Str(std::string(
                  it_map->second.hash.begin(), it_map->second.hash.end()));
              m_my_last_height = it_map->first;

              m_last_task_time = Time::now_int();

            } else {
              std::lock_guard<std::mutex> lock(m_block_list_mutex);
              it_map->second.state = BlockState::RETRIED;
              m_block_list_mutex.unlock();
            }
          }
        }
      }

      // step 2 - save block
      for (auto it_map = m_block_list.begin(); it_map != m_block_list.end();
           ++it_map) {
        if (it_map->second.state == BlockState::TOSAVE) {
          saveBlock(it_map->first);
          it_map->second.state = BlockState::TODELETE;
        }
      }

      // step 3 - delete block list
      for (auto it_map = m_block_list.begin(); it_map != m_block_list.end();) {
        if (it_map->second.state == BlockState::TODELETE) {
          std::lock_guard<std::mutex> lock(m_block_list_mutex);
          m_block_list.erase(it_map++);
          m_block_list_mutex.unlock();
        } else {
          ++it_map;
        }
      }

      // step 4 - retry block
      for (auto it_map = m_block_list.begin(); it_map != m_block_list.end();
           ++it_map) {

        if (it_map->second.num_retry > MAX_REQ_BLOCK_RETRY) {
          m_sync_done = true;
          m_sync_fail = true;
          break;
        }

        if (it_map->second.state == BlockState::RETRIED) {
          std::lock_guard<std::mutex> lock(m_block_list_mutex);
          it_map->second.num_retry += 1;
          m_block_list_mutex.unlock();
          sendBlockRequest(it_map->first);
        }
      }

      // step 5 - finishing
      if (Time::now_int() - m_last_task_time > MAX_WAIT_TIME) {
        m_sync_done = true;
        m_sync_fail = true;
      }

      if (m_first_recv_block_height <= m_my_last_height &&
          m_block_list.empty()) {
        m_sync_done = true;
      }

      if (m_sync_done) {
        m_timer_msg_fetching->cancel();
        m_timer_sync_control->cancel();

        if (m_sync_fail)
          m_finish_callback(-1);
        else
          m_finish_callback(1);
      }
    }));

    m_timer_sync_control->expires_from_now(
        boost::posix_time::milliseconds(1000));
    m_timer_sync_control->async_wait(
        [this](const boost::system::error_code &ec) {
          if (ec == boost::asio::error::operation_aborted) {
          } else if (ec.value() == 0) {
            blockSyncControl();
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
      InputMsgEntry input_msg_entry = m_inputQueue->fetch();
      if (input_msg_entry.type == MessageType::MSG_BLOCK) {
        pushMsgToBlockList(input_msg_entry);
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

    m_block_sync_strand.reset(new boost::asio::io_service::strand(io_service));
    m_timer_msg_fetching.reset(new boost::asio::deadline_timer(io_service));
    m_timer_sync_control.reset(new boost::asio::deadline_timer(io_service));

    m_storage = Storage::getInstance();
    m_inputQueue = InputQueueAlt::getInstance();
    m_outputQueue = OutputQueueAlt::getInstance();
  }

  // TODO : TYPE이 정해지면 바꿀 것
  inline void setMyID(const merger_id_type &my_ID) { m_id = my_ID; }

  void start() override {
    messageFetch();
    blockSyncControl();
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