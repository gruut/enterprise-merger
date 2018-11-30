#include "block_processor.hpp"
#include "botan-2/botan/base64.h"
#include "../utils/compressor.hpp"

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


      Botan::secure_vector<uint8_t> block_raw = Botan::base64_decode(entry.body["blockraw"].get<std::string>());
      union ByteToInt {
          uint8_t b[4];
          uint32_t t;
      };

      ByteToInt len_parse;
      len_parse.b[0] = block_raw[1];
      len_parse.b[1] = block_raw[2];
      len_parse.b[2] = block_raw[3];
      len_parse.b[3] = block_raw[4];

      size_t header_end = len_parse.t;
      std::string block_header_comp(block_raw.begin()+5, block_raw.begin()+header_end);


      std::string block_header_json;
      if( block_raw[0] == CompressionAlgorithmType::LZ4 ) {
          Compressor::decompressData(block_header_comp, block_header_json, (int)header_end-5);
      }
      else if( block_raw[0] == CompressionAlgorithmType::NONE ) {
          block_header_json.assign(block_header_comp);
      }
      else {
          std::cout << "unknown compress type" << std::endl;
          return false;
      }

      //TODO : block 유효성 검사 - 서명 확인은 MessageValidator가 하기를 희망
      // block_header_json을 확인 -> txid, tx-digest, merkle_tree, ssig



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