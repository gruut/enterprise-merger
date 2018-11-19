#ifndef GRUUT_ENTERPRISE_MERGER_MESSAGE_FACTORY_HPP
#define GRUUT_ENTERPRISE_MERGER_MESSAGE_FACTORY_HPP

#include "../chain/message.hpp"
#include "../utils/compressor.hpp"

namespace gruut {
    class MessageFactory {
    public:
        template <typename T>
        static Message create(T data) {
            std::unique_ptr<Message> message_pointer;
            MessageHeader message_header;
            message_pointer.reset(new Message(message_header));

            return *message_pointer;
        }
    };
}
#endif
