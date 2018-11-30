#ifndef GRUUT_ENTERPRISE_MERGER_BLOCK_PROCESSOR_HPP
#define GRUUT_ENTERPRISE_MERGER_BLOCK_PROCESSOR_HPP

#include "../chain/types.hpp"
#include "../modules/storage/storage.hpp"
#include "../utils/compressor.hpp"
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

public:
  BlockProcessor();
  ~BlockProcessor() {}

  bool messageProcess(InputMsgEntry &entry);
};
} // namespace gruut

#endif // GRUUT_ENTERPRISE_MERGER_BLOCK_PROCESSOR_HPP
