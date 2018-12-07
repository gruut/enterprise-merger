#include "block_generator.hpp"
#include "../chain/merkle_tree.hpp"
#include "../chain/signature.hpp"
#include "../utils/bytes_builder.hpp"
#include "../utils/rsa.hpp"
#include "../utils/time.hpp"
#include "../utils/type_converter.hpp"

using namespace std;

namespace gruut {
PartialBlock
BlockGenerator::generatePartialBlock(vector<sha256> &transactions_digest,
                                     vector<Transaction> &transactions) {
  PartialBlock block;

  // TODO: 설정파일이 없어서 하드코딩(1)
  block.merger_id = 1;
  // TODO: 위와 같은 이유로 임시값 할당
  block.chain_id = 1;
  // TODO: 위와 같은 이유로 임시값 할당
  block.height = "1";
  block.transaction_root = transactions_digest.back();
  block.transactions = transactions;

  return block;
}

Block BlockGenerator::generateBlock(PartialBlock &partial_block,
                                    vector<Signature> &signatures,
                                    MerkleTree &merkle_tree) {
  Block block(partial_block);
  BytesBuilder signature_builder;

  // Meta
  // TODO: 설정파일(config)로 설정해야 함
  block.compression_algo_type = CompressionAlgorithmType::LZ4;
  string compression_algo_type_str;
  if (block.compression_algo_type == CompressionAlgorithmType::LZ4)
    compression_algo_type_str = "LZ4";
  else
    compression_algo_type_str = "NONE";
  signature_builder.append(compression_algo_type_str);

  // TODO: 임시처리
  block.header_length = 1;
  signature_builder.append(block.header_length);

  // Header
  // TODO: 임시처리
  block.version = 1;
  signature_builder.append(block.version);

  signature_builder.append(block.chain_id);

  // TODO: 임시처리
  block.previous_header_hash = Sha256::hash("1");
  signature_builder.append(block.previous_header_hash);

  BytesBuilder builder;
  builder.append(block.chain_id);
  builder.append(block.height);
  builder.append(block.merger_id);
  // TODO: 과거 블럭에 대한 해시를 계산할 수 없어서 현재 블럭해시로 입력
  block.previous_block_id = Sha256::hash(builder.getString());
  signature_builder.append(block.previous_block_id);

  block.block_id = Sha256::hash(builder.getString());
  signature_builder.append(block.block_id);

  block.timestamp = Time::now_int();
  signature_builder.append(block.timestamp);

  signature_builder.append(block.height);

  transform(block.transactions.begin(), block.transactions.end(),
            back_inserter(block.transaction_ids),
            [](Transaction &t) { return t.transaction_id; });

  signature_builder.append(block.merger_id);

  BytesBuilder signer_signatures_builder;
  block.signer_signatures = move(signatures);
  for_each(block.signer_signatures.begin(), block.signer_signatures.end(),
           [&signer_signatures_builder](const Signature &signer_sig) {
             signer_signatures_builder.append(signer_sig.signer_id);
             auto signer_sig_bytes = signer_sig.signer_signature;
             signer_signatures_builder.append(signer_sig_bytes);
           });
  auto signer_sig_bytes = signer_signatures_builder.getBytes();
  signature_builder.append(signer_sig_bytes);

  // Body
  block.merkle_tree = merkle_tree;
  block.transactions_count = block.transactions.size();

  auto sig_bytes = signature_builder.getBytes();
  // TODO: Config로 부터 sk 읽어와야 함
  string rsa_sk = "";
  block.signature = RSA::doSign(rsa_sk, sig_bytes, true);

  return block;
}
} // namespace gruut
