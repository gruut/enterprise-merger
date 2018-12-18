#ifndef GRUUT_ENTERPRISE_MERGER_MESSAGE_FACTORY_HPP
#define GRUUT_ENTERPRISE_MERGER_MESSAGE_FACTORY_HPP

#include "../../include/nlohmann/json.hpp"
#include "../chain/block.hpp"
#include "../chain/message.hpp"
#include "../chain/signer.hpp"
#include "../chain/types.hpp"
#include "../utils/time.hpp"
#include "../utils/type_converter.hpp"
#include "../services/output_queue.hpp"

using namespace nlohmann;

namespace gruut {
using Signers = std::vector<Signer>;

class MessageFactory {
public:
  static OutputMsgEntry createSigRequestMessage(PartialBlock &block,
                                               Signers &signers) {
    json j_partial_block;

    j_partial_block["time"] = Time::now();
    j_partial_block["mID"] = TypeConverter::toBase64Str(block.merger_id);
    j_partial_block["cID"] = TypeConverter::toBase64Str(block.chain_id);
    j_partial_block["txrt"] =
        TypeConverter::toBase64Str(block.transaction_root);
    j_partial_block["hgt"] = block.height;

    vector<id_type> receivers_list;
    for_each(signers.begin(), signers.end(), [&receivers_list](Signer signer) {
      receivers_list.emplace_back(signer.user_id);
    });

    OutputMsgEntry output_message;
    output_message.type = MessageType::MSG_REQ_SSIG;
    output_message.body = j_partial_block;
    output_message.receivers = receivers_list;

    return output_message;
  }
};
} // namespace gruut
#endif
