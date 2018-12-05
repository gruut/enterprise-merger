#ifndef GRUUT_ENTERPRISE_MERGER_BLOCK_PROCESSOR_HPP
#define GRUUT_ENTERPRISE_MERGER_BLOCK_PROCESSOR_HPP

#include "../chain/merkle_tree.hpp"
#include "../chain/static_merkle_tree.hpp"
#include "../chain/transaction.hpp"
#include "../chain/types.hpp"
#include "../modules/storage/storage.hpp"
#include "../utils/compressor.hpp"
#include "../utils/rsa.hpp"
#include "../utils/sha256.hpp"
#include "botan-2/botan/base64.h"
#include "botan-2/botan/buf_comp.h"
#include "input_queue.hpp"
#include "output_queue.hpp"

#include <iostream>
#include <vector>

namespace gruut {

class BlockProcessor {
private:
  InputQueue *m_input_queue;
  OutputQueue *m_output_queue;
  Storage *m_storage;
  std::deque<InputMsgEntry> m_block_pool;

public:
  BlockProcessor();
  ~BlockProcessor() {}

  bool messageProcess(InputMsgEntry &entry);
};
} // namespace gruut

#endif // GRUUT_ENTERPRISE_MERGER_BLOCK_PROCESSOR_HPP
