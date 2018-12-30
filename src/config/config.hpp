#ifndef GRUUT_ENTERPRISE_MERGER_CONFIG_HPP
#define GRUUT_ENTERPRISE_MERGER_CONFIG_HPP

#include "../chain/types.hpp"

#include <string>

namespace gruut {
namespace config {

constexpr size_t MAX_THREAD = 40;
constexpr CompressionAlgorithmType COMPRESSION_ALGO_TYPE =
    CompressionAlgorithmType::LZ4;

constexpr size_t BP_INTERVAL = 10;
constexpr size_t BP_PING_PERIOD = 4;

constexpr size_t MAX_MERKLE_LEAVES = 4096;

constexpr size_t MAX_COLLECT_TRANSACTION_SIZE = 4096;

constexpr size_t MIN_SIGNATURE_COLLECT_SIZE = 1;
constexpr size_t MAX_SIGNATURE_COLLECT_SIZE = 20;

constexpr size_t JOIN_TIMEOUT_SEC = 10;

constexpr size_t MAX_SIGNER_NUM = 200;
constexpr size_t REQ_SSIG_SIGNERS_NUM = 10;

constexpr size_t INQUEUE_MSG_FETCHER_INTERVAL = 5;
constexpr size_t OUTQUEUE_MSG_FETCHER_INTERVAL = 100;

constexpr size_t SIGNATURE_COLLECTION_INTERVAL = 3000;
constexpr size_t SIGNATURE_COLLECTION_CHECK_INTERVAL = 500;

constexpr size_t BOOTSTRAP_RETRY_TIMEOUT = 10;
constexpr size_t BOOTSTRAP_MAX_REQ_BLOCK_RETRY = 5;
constexpr size_t BOOTSTRAP_MAX_TASK_WAIT_TIME = 10;

constexpr size_t MAX_WAIT_TIME = 5;

constexpr size_t SYNC_CONTROL_INTERVAL = 1000;

constexpr size_t AVAILABLE_INPUT_SIZE = 100;

constexpr size_t CONN_CHECK_PERIOD = 1;

const std::string DEFAULT_PORT_NUM = "50051";
const std::string DEFAULT_DB_PATH = "./db";

const std::string GENESIS_BLOCK_PREV_HASH_B64 = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=";
const std::string GENESIS_BLOCK_PREV_ID_B64 = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=";

constexpr uint8_t G = 'G';
constexpr uint8_t VERSION = '1';
constexpr uint8_t NOT_USED = 0x00;
constexpr uint8_t RESERVED[6] = {0x00};
constexpr int HEADER_LENGTH = 32;
constexpr int SENDER_ID_LENGTH = 8;
constexpr int RESERVED_LENGTH = 6;
constexpr int MSG_LENGTH_SIZE = 4;

} // namespace config
} // namespace gruut
#endif
