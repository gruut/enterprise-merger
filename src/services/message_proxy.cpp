#include "message_proxy.hpp"
#include "../../include/nlohmann/json.hpp"
#include "../application.hpp"
#include "../chain/message.hpp"
#include "../chain/types.hpp"

#include <iostream>

using namespace nlohmann;

namespace gruut {

MessageProxy::MessageProxy() { m_output_queue = OutputQueueAlt::getInstance(); }

void MessageProxy::deliverInputMessage(InputMsgEntry &input_message) {
  auto message_type = input_message.type;
  auto message_body_json = input_message.body;

  switch (message_type) {
  case MessageType::MSG_JOIN:
  case MessageType::MSG_RESPONSE_1:
  case MessageType::MSG_SUCCESS: {
    Application::app().getSignerPoolManager().handleMessage(message_type,
                                                            message_body_json);
  } break;
  case MessageType::MSG_TX: {
    Application::app().getTransactionCollector().handleMessage(
        message_body_json);
  } break;
  case MessageType::MSG_SSIG: {
    Application::app().getSignaturePool().handleMessage(message_body_json);
  } break;
  default:
    break;
  }
}

void MessageProxy::deliverOutputMessage(OutputMsgEntry &output_message) {
  cout << "MSG OUT : " << (int)output_message.type << endl;
  m_output_queue->push(output_message);
}
} // namespace gruut
