#ifndef GRUUT_ENTERPRISE_MERGER_MESSAGE_PROXY_HPP
#define GRUUT_ENTERPRISE_MERGER_MESSAGE_PROXY_HPP

#include "../chain/message.hpp"
#include "../services/input_queue.hpp"
#include "../services/output_queue.hpp"

namespace gruut {
class MessageProxy {
public:
  MessageProxy();
  void deliverInputMessage(InputMsgEntry &input_message);
  void deliverOutputMessage(OutputMsgEntry &ouput_message);

private:
  OutputQueueAlt* m_output_queue;
};
} // namespace gruut

#endif // GRUUT_ENTERPRISE_MERGER_MESSAGE_PROXY_HPP
