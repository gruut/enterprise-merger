#include "block_generator.hpp"
#include "../chain/merkle_tree.hpp"
#include "../utils/time.hpp"
#include "../utils/type_converter.hpp"

namespace gruut {
PartialBlock
BlockGenerator::generatePartialBlock(vector<sha256> &transactions_digest) {
  PartialBlock block;

  // TODO: 설정파일이 없어서 하드코딩(1)
  block.merger_id = 1;
  // TODO: 위와 같은 이유로 임시값 할당
  block.chain_id = 1;
  // TODO: 위와 같은 이유로 임시값 할당
  block.height = "1";
  block.transaction_root = transactions_digest.back();

  return block;
}
} // namespace gruut
