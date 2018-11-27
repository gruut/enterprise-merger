#include "message_proxy.hpp"
#include "../../include/nlohmann/json.hpp"
#include "../application.hpp"
#include "../chain/message.hpp"
#include "../chain/types.hpp"

using namespace nlohmann;

namespace gruut {
void MessageProxy::deliverInputMessage(InputMessage &input_message) {
  auto message_type = std::get<0>(input_message);
  auto receiver_id = std::get<1>(input_message);
  auto message_body_json = std::get<2>(input_message);

  switch (message_type) {
  case MessageType::MSG_JOIN:
  case MessageType::MSG_RESPONSE_1: {
    Application::app().getSignerPoolManager().handleMessage(
        message_type, receiver_id, message_body_json);
  } break;
  default:
    break;
  }
}

void MessageProxy::deliverOutputMessage(OutputMessage &output_message) {
  Application::app().getOutputQueue()->emplace(output_message);
}
} // namespace gruut
