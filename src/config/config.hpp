#ifndef GRUUT_ENTERPRISE_MERGER_CONFIG_HPP
#define GRUUT_ENTERPRISE_MERGER_CONFIG_HPP

#include "../chain/types.hpp"

#include <string>

namespace gruut {
namespace config {

const std::string APP_NAME = "Merger for Gruut Enterprise Networks (C++)";
const std::string APP_CODE_NAME = "Chicken Egg";
const std::string APP_BUILD_DATE = __DATE__;
const std::string APP_BUILD_TIME = __TIME__;

constexpr block_version_type DEFAULT_VERSION = 0x01;

constexpr size_t MAX_THREAD = 40;
constexpr CompressionAlgorithmType DEFAULT_COMPRESSION_TYPE =
    CompressionAlgorithmType::LZ4;
constexpr CompressionAlgorithmType DEFAULT_BLOCKRAW_COMP_ALGO =
    CompressionAlgorithmType::LZ4;

constexpr size_t BP_INTERVAL = 10;
constexpr size_t BP_PING_PERIOD = 4;

constexpr size_t BROC_PROCESSOR_TASK_PERIOD = 1500;

constexpr size_t MAX_MERKLE_LEAVES = 4096;

constexpr size_t MAX_COLLECT_TRANSACTION_SIZE = 4096;

constexpr size_t MIN_SIGNATURE_COLLECT_SIZE = 1;
constexpr size_t MAX_SIGNATURE_COLLECT_SIZE = 20;

constexpr size_t JOIN_TIMEOUT_SEC = 10;

constexpr size_t MAX_SIGNER_NUM = 200;

constexpr size_t INQUEUE_MSG_FETCHER_INTERVAL = 5;
constexpr size_t OUTQUEUE_MSG_FETCHER_INTERVAL = 100;

constexpr size_t SIGNATURE_COLLECTION_INTERVAL = 3000;
constexpr size_t SIGNATURE_COLLECTION_CHECK_INTERVAL = 500;

constexpr size_t BOOTSTRAP_RETRY_TIMEOUT = 10;
constexpr size_t BOOTSTRAP_MAX_REQ_BLOCK_RETRY = 5;
constexpr size_t BOOTSTRAP_MAX_TASK_WAIT_TIME = 10;
constexpr size_t BSYNC_RETRY_TIME_INTERVAL = 2;

constexpr size_t MAX_WAIT_TIME = 120;

constexpr size_t SYNC_CONTROL_INTERVAL = 1000;

constexpr size_t AVAILABLE_INPUT_SIZE = 1000;

constexpr int RPC_CHECK_PERIOD = 1;
constexpr int HTTP_CHECK_PERIOD = 5;

const std::string DEFAULT_PORT_NUM = "50051";
const std::string DEFAULT_DB_PATH = "./db";

const std::string GENESIS_BLOCK_PREV_HASH_B64 =
    "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=";
const std::string GENESIS_BLOCK_PREV_ID_B64 =
    "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=";

const std::string DB_SUB_DIR_HEADER = "block_header";
const std::string DB_SUB_DIR_RAW = "block_raw";
const std::string DB_SUB_DIR_CERT = "certificate";
const std::string DB_SUB_DIR_LATEST = "latest_block_header";
const std::string DB_SUB_DIR_TRANSACTION = "transaction";
const std::string DB_SUB_DIR_IDHEIGHT = "blockid_height";

} // namespace config
} // namespace gruut
#endif
