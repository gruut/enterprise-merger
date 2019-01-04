#include "message_proxy.hpp"
#include "../application.hpp"
#include "easy_logging.hpp"

namespace gruut {

MessageProxy::MessageProxy() {
  m_output_queue = OutputQueueAlt::getInstance();
  el::Loggers::getLogger("MPRX");
}

void MessageProxy::deliverInputMessage(InputMsgEntry &input_message) {

  if (input_message.type != MessageType::MSG_TX)
    CLOG(INFO, "MPRX") << "MSG IN: " << (int)input_message.type;

  auto message_type = input_message.type;
  auto message_body_json = input_message.body;

  if (MessageValidator::validate(message_type, message_body_json)) {
    switch (message_type) {
    case MessageType::MSG_JOIN:
    case MessageType::MSG_RESPONSE_1:
    case MessageType::MSG_SUCCESS:
    case MessageType::MSG_LEAVE: {
      Application::app().getSignerPoolManager().handleMessage(
          message_type, message_body_json);
    } break;
    case MessageType::MSG_TX: {
      Application::app().getTransactionCollector().handleMessage(
          message_body_json);
    } break;
    case MessageType::MSG_SSIG: {
      Application::app().getSignaturePool().handleMessage(message_body_json);
    } break;
    case MessageType::MSG_PING:
    case MessageType::MSG_UP: {
      Application::app().getBpScheduler().handleMessage(input_message);
    } break;
    case MessageType::MSG_REQ_CHECK:
    case MessageType::MSG_BLOCK:
    case MessageType::MSG_REQ_BLOCK: {
      Application::app().getBlockProcessor().handleMessage(input_message);
    } break;
    default:
      break;
    }
  } else {
    CLOG(INFO, "MPRX") << "Incomming message is not valid";
  }
}

void MessageProxy::deliverOutputMessage(OutputMsgEntry &output_message) {

  CLOG(INFO, "MPRX") << "MSG OUT: " << (int)output_message.type;

  m_output_queue->push(output_message);
}
} // namespace gruut
