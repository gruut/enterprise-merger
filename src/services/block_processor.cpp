#include "block_processor.hpp"

namespace gruut {

bool BlockProcessor::messageProcess(InputMsgEntry &entry) {
  if (entry.type == (uint8_t)MessageType::MSG_REQ_BLOCK) {
    // TODO : storage read
    // TODO : make MSG_BLOCK
    // TODO : push to outputqueue
      //m_output_queue->push();
  }
  // msg_block
  else if (entry.type == (uint8_t)MessageType::MSG_BLOCK) {
      //TODO : MSG_BLOCK의 구조 분석
      //TODO : 압축된 json 풀고
      //TODO : block 유효성 검사 - 서명 확인은 MessageValidator가 하기를 희망
      // TODO : storage save 추가

  } else {
  }
}
BlockProcessor::BlockProcessor() {
  m_input_queue = InputQueue::getInstance();
  m_output_queue = OutputQueue::getInstance();
  m_storage = Storage::getInstance();
}
} // namespace gruut