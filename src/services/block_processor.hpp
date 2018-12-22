#ifndef GRUUT_ENTERPRISE_MERGER_BLOCK_PROCESSOR_HPP
#define GRUUT_ENTERPRISE_MERGER_BLOCK_PROCESSOR_HPP

#include "../../include/base64.hpp"
#include "../chain/merkle_tree.hpp"
#include "../chain/transaction.hpp"
#include "../chain/types.hpp"
#include "../utils/bytes_builder.hpp"
#include "../utils/compressor.hpp"
#include "../utils/rsa.hpp"
#include "../utils/sha256.hpp"
#include "block_validator.hpp"
#include "botan-2/botan/base64.h"
#include "botan-2/botan/buf_comp.h"
#include "input_queue.hpp"
#include "message_proxy.hpp"
#include "output_queue.hpp"
#include "setting.hpp"
#include "storage.hpp"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <vector>

namespace gruut {

class BlockProcessor {
private:
  MessageProxy m_msg_proxy;
  Storage *m_storage;

public:
  BlockProcessor();
  ~BlockProcessor() {}

  bool handleMessage(InputMsgEntry &entry);
  bool handleMsgReqBlock(InputMsgEntry &entry);
  bool handleMsgBlock(InputMsgEntry &entry);
  bool handleMsgReqCheck(InputMsgEntry &entry);
};
} // namespace gruut

#endif // GRUUT_ENTERPRISE_MERGER_BLOCK_PROCESSOR_HPP
