#ifndef GRUUT_ENTERPRISE_MERGER_MESSAGE_FACTORY_HPP
#define GRUUT_ENTERPRISE_MERGER_MESSAGE_FACTORY_HPP

#include "../../include/nlohmann/json.hpp"
#include "../chain/block.hpp"
#include "../chain/message.hpp"
#include "../chain/types.hpp"
#include "../utils/compressor.hpp"

namespace gruut {
class MessageFactory {
public:
  // TODO: Scalability 를 위해서 이래와 같은 코드로 작성해야 함.
  //        template <typename T>
  //        static Message create(T data, MessageType message_type) {
  //            std::unique_ptr<Message> message_pointer;
  //            MessageHeader message_header;
  //
  //            message_header.message_type = message_type;
  //            message_pointer.reset(new Message(message_header));
  //
  //            return *message_pointer;
  //        }

  static OutputMessage createSigRequestMessage(PartialBlock &block) {
    std::unique_ptr<Message> message_pointer;
    MessageHeader message_header;

    // TODO: sender_id sha256으로 refactor하면 주석해제할것
    std::unordered_map<std::string, std::string> block_map{
        {"time", block.sent_time},
        //        {"mID", block.sender_id},
        //        {"cID", block.chain_id},
        //        {"hgt", block.height},
        //        {"txrt", block.transaction_root}
    };
    nlohmann::json j_block_map(block_map);

    // TODO: sender_list를 가져와야 함 (vector<uint8_t>)
    //       임시로 111
    vector<uint64_t> receivers_list{111};
    auto output_message =
        std::make_tuple(MessageType::MSG_REQ_SSIG, receivers_list, j_block_map);

    return output_message;
  }
};
} // namespace gruut
#endif
