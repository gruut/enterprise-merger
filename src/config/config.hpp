#ifndef GRUUT_ENTERPRISE_MERGER_CONFIG_HPP
#define GRUUT_ENTERPRISE_MERGER_CONFIG_HPP

#include "../chain/types.hpp"

namespace gruut {
namespace config {
constexpr CompressionAlgorithmType COMPRESSION_ALGO_TYPE =
    CompressionAlgorithmType::LZ4;

constexpr size_t MAX_MERKLE_LEAVES = 4096;

constexpr size_t MAX_COLLECT_TRANSACTION_SIZE = 4096;

constexpr size_t MIN_SIGNATURE_COLLECT_SIZE =
    1; // TODO: 테스트를 위해 임시로 1개로 설정
constexpr size_t MAX_SIGNATURE_COLLECT_SIZE = 4096;

constexpr uint64_t JOIN_TIMEOUT_SEC = 10;
} // namespace config
} // namespace gruut
#endif
