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
    CLOG(INFO, "MPRX") << "MSG IN: 0x" << std::hex << (int)input_message.type;

  auto message_type = input_message.type;
  auto message_body_json = input_message.body;

  MessageValidator validator;

  if (validator.validate(input_message)) {
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
    case MessageType::MSG_UP:
    case MessageType::MSG_WELCOME: {
      Application::app().getBpScheduler().handleMessage(input_message);
    } break;
    case MessageType::MSG_REQ_CHECK:
    case MessageType::MSG_BLOCK:
    case MessageType::MSG_REQ_BLOCK:
    case MessageType::MSG_REQ_STATUS: {
      Application::app().getBlockProcessor().handleMessage(input_message);
    } break;
    default:
      break;
    }
  } else {
    CLOG(INFO, "MPRX") << "Incomming message is not valid";
    CLOG(INFO, "MPRX") << input_message.body.dump();
  }
}

void MessageProxy::deliverOutputMessage(OutputMsgEntry &output_message) {

  CLOG(INFO, "MPRX") << "MSG OUT: 0x" << std::hex << (int)output_message.type;

  m_output_queue->push(output_message);
}

void MessageProxy::deliverBlockProcessor(InputMsgEntry &input_message) {
  CLOG(INFO, "MPRX") << "MSG TO BPRO: 0x" << std::hex
                     << (int)input_message.type;
  Application::app().getBlockProcessor().handleMessage(input_message);
}
} // namespace gruut
