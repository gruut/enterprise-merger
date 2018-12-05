#ifndef GRUUT_ENTERPRISE_MERGER_BLOCK_PROCESSOR_HPP
#define GRUUT_ENTERPRISE_MERGER_BLOCK_PROCESSOR_HPP

#include "../../include/base64.hpp"
#include "../chain/knowledge.hpp"
#include "../chain/merkle_tree.hpp"
#include "../chain/transaction.hpp"
#include "../chain/types.hpp"
#include "../utils/bytes_builder.hpp"
#include "../utils/compressor.hpp"
#include "../utils/rsa.hpp"
#include "../utils/sha256.hpp"
#include "botan-2/botan/base64.h"
#include "botan-2/botan/buf_comp.h"
#include "input_queue.hpp"
#include "output_queue.hpp"
#include "storage.hpp"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace gruut {

class BlockProcessor {
private:
  InputQueueAlt *m_input_queue;
  OutputQueueAlt *m_output_queue;
  Storage *m_storage;

public:
  BlockProcessor();
  ~BlockProcessor() {}

  bool messageProcess(InputMsgEntry &entry);
};
} // namespace gruut

#endif // GRUUT_ENTERPRISE_MERGER_BLOCK_PROCESSOR_HPP