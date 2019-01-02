#include "message_handler.hpp"
#include "../../config/config.hpp"
#include "../../utils/compressor.hpp"
#include "../../utils/safe.hpp"
#include "../../utils/time.hpp"
#include "../../utils/type_converter.hpp"
#include "easy_logging.hpp"
#include "merger_client.hpp"

namespace gruut {

MessageHandler::MessageHandler() {
  m_input_queue = InputQueueAlt::getInstance();
  el::Loggers::getLogger("MHDL");
}

void MessageHandler::unpackMsg(std::string &packed_msg,
                               grpc::Status &rpc_status, id_type &recv_id) {
  using namespace grpc;
  if (packed_msg.size() < config::HEADER_LENGTH) {
    rpc_status = Status(StatusCode::INVALID_ARGUMENT,
                        "Wrong Message (MessageHandler::unpackMsg)");
    return;
  }
  MessageHeader header = HeaderController::parseHeader(packed_msg);
  if (!validateMsgFormat(header)) {
    rpc_status = Status(StatusCode::INVALID_ARGUMENT,
                        "Wrong Message (MessageHandler::unpackMsg)");
    return;
  }
  size_t body_size = getMsgBodySize(header);
  recv_id = header.sender_id;
  std::string recv_str_id = TypeConverter::encodeBase64(recv_id);

  if (header.mac_algo_type == MACAlgorithmType::HMAC) {
    std::string msg = packed_msg.substr(0, config::HEADER_LENGTH + body_size);
    std::vector<uint8_t> hmac(packed_msg.begin() + config::HEADER_LENGTH +
                                  body_size,
                              packed_msg.end());

    auto &signer_pool = Application::app().getSignerPool();
    Botan::secure_vector<uint8_t> secure_vector_key =
        signer_pool.getHmacKey(recv_id);
    std::vector<uint8_t> key = std::vector<uint8_t>(secure_vector_key.begin(),
                                                    secure_vector_key.end());

    if (!Hmac::verifyHMAC(msg, hmac, key)) {
      rpc_status = Status(StatusCode::UNAUTHENTICATED,
                          "Wrong HMAC (MessageHandler::unpackMsg)");
      return;
    }
  }

  std::string msg_body = getMsgBody(packed_msg, body_size);
  nlohmann::json json_data = getJson(header.compression_algo_type, msg_body);

  if (!JsonValidator::validateSchema(json_data, header.message_type)) {
    rpc_status = Status(grpc::StatusCode::INVALID_ARGUMENT,
                        "Json schema check fail (MessageHandler::unpackMsg)");
    return;
  }

  m_input_queue->push(header.message_type, json_data);
  rpc_status = Status::OK;
}

void MessageHandler::packMsg(OutputMsgEntry &output_msg) {
  MessageType msg_type = output_msg.type;

  nlohmann::json body = output_msg.body;
  MessageHeader header;
  header.message_type = msg_type;

  header.compression_algo_type = config::DEFAULT_COMPRESSION_TYPE;
  std::string packed_msg = genPackedMsg(header, body);
  std::vector<std::string> packed_msg_list;

  if (msg_type == MessageType::MSG_ACCEPT ||
      msg_type == MessageType::MSG_REQ_SSIG) {
    auto &signer_pool = Application::app().getSignerPool();

    for (auto &recv_id : output_msg.receivers) {
      Botan::secure_vector<uint8_t> secure_vector_key =
          signer_pool.getHmacKey(recv_id);
      std::vector<uint8_t> key(secure_vector_key.begin(),
                               secure_vector_key.end());
      std::vector<uint8_t> hmac = Hmac::generateHMAC(packed_msg, key);
      std::string str_hmac(hmac.begin(), hmac.end());
      std::string hmac_packed_data = packed_msg + str_hmac;
      packed_msg_list.emplace_back(hmac_packed_data);
    }
  } else {
    packed_msg_list.emplace_back(packed_msg);
  }

  MergerClient merger_client;
  merger_client.sendMessage(msg_type, output_msg.receivers, packed_msg_list,
                            output_msg);
}

bool MessageHandler::validateMsgFormat(MessageHeader &header) {
  // TODO : Message header에서 확인해야 하는 사항이 있을때 추가예정
  bool check = (header.identifier == config::G);
  if (header.mac_algo_type == MACAlgorithmType::HMAC) {
    check &= (header.message_type == MessageType::MSG_SUCCESS ||
              header.message_type == MessageType::MSG_SSIG);
  }
  return check;
}

int MessageHandler::getMsgBodySize(MessageHeader &header) {
  int total_size = HeaderController::convertU8ToU32BE(header.total_length);
  int body_size = total_size - static_cast<int>(config::HEADER_LENGTH);
  return body_size;
}

std::string MessageHandler::getMsgBody(std::string &packed_msg, int body_size) {
  std::string packed_body = packed_msg.substr(config::HEADER_LENGTH, body_size);
  return packed_body;
}

nlohmann::json
MessageHandler::getJson(CompressionAlgorithmType compression_type,
                        std::string &body) {
  nlohmann::json unpacked_body;
  if (!body.empty()) {
    switch (compression_type) {
    case CompressionAlgorithmType::LZ4: {
      std::string origin_data = Compressor::decompressData(body);
      unpacked_body = Safe::parseJson(origin_data);
    } break;
    case CompressionAlgorithmType::NONE: {
      unpacked_body = Safe::parseJson(body);
    } break;
    default:
      break;
    }
  }
  return unpacked_body;
}

std::string MessageHandler::genPackedMsg(MessageHeader &header,
                                         nlohmann::json &body) {
  std::string body_dump = body.dump();

  switch (header.compression_algo_type) {
  case CompressionAlgorithmType::LZ4: {
    std::string compressed_body = Compressor::compressData(body_dump);
    body_dump = compressed_body;
  } break;
  case CompressionAlgorithmType ::NONE:
  default:
    break;
  }

  std::string packed_msg = HeaderController::attachHeader(
      body_dump, header.message_type, header.compression_algo_type);
  return packed_msg;
}

void MessageHandler::genInternalMsg(MessageType msg_type, std::string &id_b64) {
  nlohmann::json msg_body;
  switch (msg_type) {
  case MessageType::MSG_LEAVE: {
    msg_body["sID"] = id_b64;
    msg_body["time"] = Time::now();
    msg_body["msg"] = "disconnected with signer";
  } break;

  default:
    break;
  }

  m_input_queue->push(msg_type, msg_body);
}

}; // namespace gruut