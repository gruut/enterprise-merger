#ifndef GRUUT_ENTERPRISE_MERGER_MESSAGE_PROXY_HPP
#define GRUUT_ENTERPRISE_MERGER_MESSAGE_PROXY_HPP

#include "nlohmann/json.hpp"

#include "../chain/message.hpp"
#include "../chain/types.hpp"
#include "../services/input_queue.hpp"
#include "../services/output_queue.hpp"

#include "message_validator.hpp"

namespace gruut {
class MessageProxy {
public:
  MessageProxy();
  void deliverInputMessage(InputMsgEntry &input_message);
  void deliverOutputMessage(OutputMsgEntry &ouput_message);

private:
  OutputQueueAlt *m_output_queue;
};
} // namespace gruut

#endif // GRUUT_ENTERPRISE_MERGER_MESSAGE_PROXY_HPP
