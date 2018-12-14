#include "block_generator.hpp"
#include "../chain/merkle_tree.hpp"
#include "../chain/signature.hpp"
#include "../chain/types.hpp"
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
  block.merger_id = TypeConverter::integerToBytes(1);
  // TODO: 위와 같은 이유로 임시값 할당
  block.chain_id = TypeConverter::integerToArray<CHAIN_ID_TYPE_SIZE>(1);
  // TODO: 위와 같은 이유로 임시값 할당
  block.height = 1;
  block.transaction_root = transactions_digest.back();
  block.transactions = transactions;

  return block;
}

void BlockGenerator::generateBlock(PartialBlock &partial_block,
                                   vector<Signature> &signatures,
                                   MerkleTree &merkle_tree) {
  BytesBuilder block_raw_builder;

  // Meta
  auto compression_algo_type = config::COMPRESSION_ALGO_TYPE;
  string compression_algo_type_str;
  if (compression_algo_type == CompressionAlgorithmType::LZ4)
    compression_algo_type_str = "LZ4";
  else
    compression_algo_type_str = "NONE";
  block_raw_builder.append(compression_algo_type_str, 1);

  // TODO: 임시처리
  header_length_type header_length = 1;
  block_raw_builder.append(header_length); // ,4 붙여야함

  // Header
  // TODO: 임시처리
  block_version_type version = 1;
  block_raw_builder.append(version);

  block_raw_builder.append(partial_block.chain_id);

  // TODO: 임시처리
  auto previous_header_hash = Sha256::hash("1");
  block_raw_builder.append(previous_header_hash);

  BytesBuilder builder;
  builder.append(partial_block.chain_id);
  builder.append(partial_block.height);
  builder.append(partial_block.merger_id);

  // TODO: 과거 블럭에 대한 해시를 계산할 수 없어서 현재 블럭해시로 입력
  auto previous_block_id = Sha256::hash(builder.getString());
  block_raw_builder.append(previous_block_id);

  auto block_id = Sha256::hash(builder.getString());
  block_raw_builder.append(block_id);

  auto timestamp = Time::now_int();
  block_raw_builder.append(timestamp);

  block_raw_builder.append(partial_block.height);
  block_raw_builder.append(partial_block.merger_id);

  BytesBuilder signer_signatures_builder;
  auto signer_signatures = move(signatures);
  for_each(signer_signatures.begin(), signer_signatures.end(),
           [&signer_signatures_builder](const Signature &signer_sig) {
             signer_signatures_builder.append(signer_sig.signer_id);
             auto signer_sig_bytes = signer_sig.signer_signature;
             signer_signatures_builder.append(signer_sig_bytes);
           });
  auto signer_sig_bytes = signer_signatures_builder.getBytes();
  block_raw_builder.append(signer_sig_bytes);

  auto sig_bytes = block_raw_builder.getBytes();
  // TODO: Config로 부터 sk 읽어와야 함
  string rsa_sk = "";
  auto signature = RSA::doSign(rsa_sk, sig_bytes, true);
  block_raw_builder.append(signature);

  string block_raw = block_raw_builder.getString();

  vector<transaction_id_type> transaction_ids;
  transform(partial_block.transactions.begin(),
            partial_block.transactions.end(), back_inserter(transaction_ids),
            [](Transaction &t) { return t.transaction_id; });

  // Header (JSON)
  json block_header;
  block_header["ver"] = to_string(version);
  auto chain_id_str = TypeConverter::toString(partial_block.chain_id);
  block_header["cID"] = TypeConverter::toBase64Str(chain_id_str);
  block_header["prevH"] = TypeConverter::toBase64Str(previous_header_hash);
  block_header["prevbID"] = TypeConverter::toBase64Str(previous_block_id);
  block_header["time"] = to_string(timestamp);
  block_header["hgt"] = to_string(partial_block.height);
  block_header["txrt"] =
      TypeConverter::toBase64Str(partial_block.transaction_root);

  vector<string> tx_ids;
  std::transform(transaction_ids.begin(), transaction_ids.end(),
                 back_inserter(tx_ids),
                 [](const transaction_id_type &transaction_id) {
                   return TypeConverter::toBase64Str(transaction_id);
                 });
  block_header["txids"] = tx_ids;

  vector<unordered_map<signer_id_type, signature_type>> sig_map;
  for (size_t i = 0; i < signer_signatures.size(); ++i) {
    block_header["SSig"][i]["sID"] = to_string(signer_signatures[i].signer_id);
    block_header["SSig"][i]["sig"] =
        TypeConverter::toBase64Str(signer_signatures[i].signer_signature);
  }

  auto merger_id_str = TypeConverter::toString(partial_block.merger_id);
  block_header["mID"] = TypeConverter::toBase64Str(merger_id_str);

  BytesBuilder b_id_builder;
  b_id_builder.append(partial_block.chain_id);
  b_id_builder.append(partial_block.height);
  b_id_builder.append(partial_block.merger_id);
  auto b_id_bytes = b_id_builder.getBytes();
  auto hashed_b_id = Sha256::hash(b_id_bytes);
  block_header["bID"] = TypeConverter::toBase64Str(hashed_b_id);

  block_header["mSig"] = TypeConverter::toBase64Str(signature);

  // Body (JSON)
  json block_body;
  vector<string> mtree;
  auto tree_nodes_vector = merkle_tree.getMerkleTree();
  std::transform(tree_nodes_vector.begin(), tree_nodes_vector.end(),
                 back_inserter(mtree), [](sha256 &transaction_id) {
                   return TypeConverter::toBase64Str(transaction_id);
                 });
  block_body["mtree"] = mtree;
  auto transactions_count = partial_block.transactions.size();
  block_body["txCnt"] = to_string(transactions_count);

  vector<json> transactions_arr;
  std::transform(partial_block.transactions.begin(),
                 partial_block.transactions.end(),
                 back_inserter(transactions_arr), [this](Transaction &tx) {
                   json transaction_json;
                   this->toJson(transaction_json, tx);
                   return transaction_json;
                 });
  block_body["tx"] = transactions_arr;

  Storage *storage = Storage::getInstance();
  storage->saveBlock(block_raw, block_header, block_body);
  Storage::destroyInstance();
}

void BlockGenerator::toJson(json &j, const Transaction &tx) {
  auto tx_id = TypeConverter::toBase64Str(tx.transaction_id);
  auto tx_time = TypeConverter::toString(tx.sent_time);
  auto requester_id = TypeConverter::toBase64Str(tx.requestor_id);
  string transaction_type;
  if (tx.transaction_type == TransactionType::CERTIFICATE)
    transaction_type = "certificates";
  else
    transaction_type = "digests";
  auto signature = TypeConverter::toBase64Str(tx.signature);

  j = json{{"txID", tx_id},       {"time", tx_time},
           {"rID", requester_id}, {"type", transaction_type},
           {"rSig", signature},   {"content", tx.content_list}};
}
} // namespace gruut
