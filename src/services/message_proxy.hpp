#ifndef GRUUT_ENTERPRISE_MERGER_MESSAGE_PROXY_HPP
#define GRUUT_ENTERPRISE_MERGER_MESSAGE_PROXY_HPP

#include "../chain/message.hpp"

namespace gruut {
class MessageProxy {
public:
  void deliverInputMessage(InputMessage &input_message);
  void deliverOutputMessage(OutputMessage &ouput_message);
};
} // namespace gruut

#endif // GRUUT_ENTERPRISE_MERGER_MESSAGE_PROXY_HPP
