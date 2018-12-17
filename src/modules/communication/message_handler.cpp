#include "message_handler.hpp"
#include "../../utils/compressor.hpp"
#include "merger_client.hpp"

namespace gruut {
void MessageHandler::unpackMsg(std::string &packed_msg,
                               grpc::Status &rpc_status, id_type &recv_id) {
  using namespace grpc;
  auto &input_queue = Application::app().getInputQueue();
  MessageHeader header = HeaderController::parseHeader(packed_msg);
  if (!validateMessage(header)) {
    rpc_status = Status(StatusCode::INVALID_ARGUMENT, "Wrong Message");
    return;
  }
  int body_size = getMsgBodySize(header);

  if (header.mac_algo_type == MACAlgorithmType::HMAC) {
    std::string msg = packed_msg.substr(0, HEADER_LENGTH + body_size);
    std::vector<uint8_t> hmac(packed_msg.begin() + HEADER_LENGTH + body_size,
                              packed_msg.end());

    auto &signer_pool = Application::app().getSignerPool();
    Botan::secure_vector<uint8_t> secure_vector_key =
        signer_pool.getHmacKey(recv_id);
    std::vector<uint8_t> key = std::vector<uint8_t>(secure_vector_key.begin(),
                                                    secure_vector_key.end());

    if (!Hmac::verifyHMAC(msg, hmac, key)) {
      rpc_status = Status(StatusCode::UNAUTHENTICATED, "Wrong HMAC");
      return;
    }
  }

  std::string msg_body = getMsgBody(packed_msg, body_size);
  nlohmann::json json_data = getJson(header.compression_algo_type, msg_body);

  if (!JsonValidator::validateSchema(json_data, header.message_type)) {
    rpc_status =
        Status(grpc::StatusCode::INVALID_ARGUMENT, "json schema check fail");
    return;
  }

  input_queue->emplace(make_tuple(header.message_type, recv_id, json_data));
  rpc_status = Status::OK;
}

void MessageHandler::packMsg(OutputMessage &output_msg) {
  MessageType msg_type = get<0>(output_msg);

  nlohmann::json body = get<2>(output_msg);
  MessageHeader header;
  header.message_type = msg_type;
  // TODO : Compression type에 따라 수정 될 수 있습니다.
  header.compression_algo_type = CompressionAlgorithmType::NONE;
  std::string packed_msg = genPackedMsg(header, body);
  std::vector<std::string> packed_msg_list;

  if (msg_type == MessageType::MSG_ACCEPT ||
      msg_type == MessageType::MSG_REQ_SSIG) {
    auto &signer_pool = Application::app().getSignerPool();

    for (auto &recv_id : get<1>(output_msg)) {
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
  merger_client.sendMessage(msg_type, get<1>(output_msg), packed_msg_list);
}

bool MessageHandler::validateMessage(MessageHeader &header) {
  bool check = (header.identifier == G /*&& msg_header.version == VERSION*/);
  if (header.mac_algo_type == MACAlgorithmType::HMAC) {
    check &= (header.message_type == MessageType::MSG_SUCCESS ||
              header.message_type == MessageType::MSG_SSIG);
  }
  return check;
}

int MessageHandler::getMsgBodySize(MessageHeader &header) {
  int total_size = HeaderController::convertU8ToU32BE(header.total_length);
  int body_size = total_size - static_cast<int>(HEADER_LENGTH);
  return body_size;
}

std::string MessageHandler::getMsgBody(std::string &packed_msg, int body_size) {
  std::string packed_body = packed_msg.substr(HEADER_LENGTH, body_size);
  return packed_body;
}

nlohmann::json
MessageHandler::getJson(CompressionAlgorithmType compression_type,
                        std::string &body) {
  nlohmann::json unpacked_body;
  switch (compression_type) {
  case CompressionAlgorithmType::LZ4: {
    std::string origin_data;
    Compressor::decompressData(body, origin_data, body.size());
    unpacked_body = nlohmann::json::parse(origin_data);
  } break;
  case CompressionAlgorithmType::NONE: {
    unpacked_body = nlohmann::json::parse(body);
  } break;
  default:
    break;
  }
  return unpacked_body;
}

std::string MessageHandler::genPackedMsg(MessageHeader &header,
                                         nlohmann::json &body) {
  std::string body_dump = body.dump();

  switch (header.compression_algo_type) {
  case CompressionAlgorithmType::LZ4: {
    std::string compressed_body;
    Compressor::compressData(body_dump, compressed_body);
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
}; // namespace gruut