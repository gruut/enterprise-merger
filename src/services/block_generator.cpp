#include "block_generator.hpp"
#include "../chain/merkle_tree.hpp"
#include "../utils/bytes_builder.hpp"
#include "../utils/time.hpp"
#include "../utils/type_converter.hpp"

using namespace std;

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

Block BlockGenerator::generateBlock(PartialBlock &partial_block,
                                    vector<Transaction> &transactions,
                                    vector<Signature> &signatures,
                                    MerkleTree &merkle_tree) {
  Block block(partial_block);

  // TODO: 설정파일(config)로 설정해야 함
  block.compression_algo_type = CompressionAlgorithmType::LZ4;
  // TODO: 임시처리
  block.header_length = 1;
  // TODO: 임시처리
  block.version = 1;
  // TODO: 임시처리
  block.previous_header_hash = Sha256::hash("1");

  BytesBuilder builder;
  builder.append(block.chain_id);
  builder.append(block.height);
  builder.append(block.merger_id);
  // TODO: 과거 블럭에 대한 해시를 계산할 수 없어서 현재 블럭해시로 입력
  block.previous_block_id = Sha256::hash(builder.getString());
  block.block_id = Sha256::hash(builder.getString());
  block.timestamp = Time::now_int();

  transform(transactions.begin(), transactions.end(),
            back_inserter(block.transaction_ids),
            [](Transaction &t) { return t.transaction_id; });

  block.signer_signatures = move(signatures);

  block.merkle_tree = merkle_tree;
  block.transactions_count = transactions.size();
  block.transactions = transactions;

  return block;
}
} // namespace gruut
