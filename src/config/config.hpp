#ifndef GRUUT_ENTERPRISE_MERGER_CONFIG_HPP
#define GRUUT_ENTERPRISE_MERGER_CONFIG_HPP

#include "../chain/types.hpp"

#include <string>

namespace gruut {
namespace config {

// clang-format off

// APP INFO

const std::string APP_NAME = "Merger for Gruut Enterprise Networks (C++)";
const std::string APP_CODE_NAME = "Chicken Egg";
const std::string APP_BUILD_DATE = __DATE__;
const std::string APP_BUILD_TIME = __TIME__;
constexpr block_version_type DEFAULT_VERSION = 0x01;

// SETTING

constexpr size_t MAX_THREAD = 40;
constexpr auto DEFAULT_COMPRESSION_TYPE = CompressionAlgorithmType::LZ4;
constexpr auto DEFAULT_BLOCKRAW_COMP_ALGO = CompressionAlgorithmType::LZ4;
constexpr size_t MAX_SIGNER_NUM = 200;
constexpr size_t AVAILABLE_INPUT_SIZE = 1000;
constexpr size_t MAX_MERKLE_LEAVES = 4096;
constexpr size_t MAX_COLLECT_TRANSACTION_SIZE = 4096;
constexpr size_t BLOCK_CONFIRM_LEVEL = 3;
constexpr size_t MIN_SIGNATURE_COLLECT_SIZE = 1;
constexpr size_t MAX_SIGNATURE_COLLECT_SIZE = 20;

// TIMING

constexpr size_t SYNC_CONTROL_INTERVAL = 1000;
constexpr size_t BP_INTERVAL = 10;
constexpr size_t BP_PING_PERIOD = 4;
constexpr size_t BROC_PROCESSOR_TASK_INTERVAL = 500;
constexpr size_t BROC_PROCESSOR_REQ_WAIT = 3;
constexpr size_t STATUS_COLLECTING_TIMEOUT = 4000;
constexpr size_t JOIN_TIMEOUT_SEC = 10;
constexpr size_t INQUEUE_MSG_FETCHER_INTERVAL = 5;
constexpr size_t OUTQUEUE_MSG_FETCHER_INTERVAL = 100;
constexpr size_t SIGNATURE_COLLECTION_TIMEOUT = 3000;
constexpr size_t SIGNATURE_COLLECTION_CHECK_INTERVAL = 500;
constexpr int RPC_CHECK_INTERVAL = 1000;
constexpr int HTTP_CHECK_INTERVAL = 5000;
constexpr size_t TIME_MAX_DIFF_SEC = 3;
constexpr size_t MAX_WAIT_CONNECT_OTHERS_BSYNC_SEC = 5;

// KNOWLEDGE

const std::string DEFAULT_PORT_NUM = "50051";
const std::string DEFAULT_DB_PATH = "./db";
const std::string GENESIS_BLOCK_PREV_HASH_B64 = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=";
const std::string GENESIS_BLOCK_PREV_ID_B64 = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=";
const std::string DB_SUB_DIR_HEADER = "block_header";
const std::string DB_SUB_DIR_RAW = "block_raw";
const std::string DB_SUB_DIR_LATEST = "latest_block_header";
const std::string DB_SUB_DIR_TRANSACTION = "transaction";
const std::string DB_SUB_DIR_IDHEIGHT = "blockid_height";
const std::string DB_SUB_DIR_LEDGER = "ledger";
const std::string DB_SUB_DIR_BACKUP = "backup";

// clang-format on

} // namespace config
} // namespace gruut
#endif
