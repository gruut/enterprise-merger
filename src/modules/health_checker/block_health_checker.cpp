#include "block_health_checker.hpp"
#include "../../application.hpp"
#include "easy_logging.hpp"

namespace gruut {

BlockHealthChecker::BlockHealthChecker() { el::Loggers::getLogger("BHCH"); }

void BlockHealthChecker::start() {
  auto &io_service = Application::app().getIoService();
  io_service.post([this]() { startHealthCheck(); });
}

void BlockHealthChecker::startHealthCheck() {

  auto storage = Storage::getInstance();

  auto latest_block_info = storage->getNthBlockLinkInfo();

  if (latest_block_info.height == 0) {
    CLOG(INFO, "BHCH")
        << "BLOCK HEALTH CHECKING ---- SKIP (no block in storage)";
    endCheck(ExitCode::BLOCK_HEALTH_SKIP);
    return;
  }

  CLOG(INFO, "BHCH") << "BLOCK HEALTH CHECKING ---- START";

  std::unique_ptr<Block> test_block;

  bool is_healthy = true;
  std::string prev_hash_b64 = config::GENESIS_BLOCK_PREV_HASH_B64;
  std::string prev_block_id_b64 = config::GENESIS_BLOCK_PREV_ID_B64;
  size_t last_healty_block = 0;

  CLOG(INFO, "BHCH") << "Checking ... 0/" << latest_block_info.height;

  size_t unit_step =
      (latest_block_info.height < 10) ? 1 : latest_block_info.height / 10;

  for (size_t i = 1; i < latest_block_info.height; ++i) {

    if (i % unit_step == 0)
      CLOG(INFO, "BHCH") << "Checking ... " << i << "/"
                         << latest_block_info.height;

    read_block_type nth_block = storage->readBlock(i);
    test_block.reset(new Block);
    test_block->initialize(nth_block);
    if (test_block->isValid() &&
        test_block->getPrevHashB64() == prev_hash_b64 &&
        test_block->getPrevBlockIdB64() == prev_block_id_b64) {

      last_healty_block = i;

      prev_hash_b64 = test_block->getHashB64();
      prev_block_id_b64 = test_block->getBlockIdB64();

    } else {
      CLOG(ERROR, "BHCH") << "Health check is aborted. (found problem in block "
                          << test_block->getBlockIdB64() << ")";
      is_healthy = false;
      break;
    }
  }

  if (!is_healthy) {
    // clang-format off
    CLOG(ERROR, "BHCH") << "+-------------------------------------------------------------------------+";
    CLOG(ERROR, "BHCH") << "| DB health check has been failed.                                        |";
    CLOG(ERROR, "BHCH") << "| It is strongly recommended to stop running and repair DB.               |";
    CLOG(ERROR, "BHCH") << "| To run anyway, input 'run'.                                             |";
    CLOG(ERROR, "BHCH") << "| To run after deleting blocks from corrupted one, input 'del'.           |";
    CLOG(ERROR, "BHCH") << "| To abort run, press ctrl+C or input others                              |";
    CLOG(ERROR, "BHCH") << "+-------------------------------------------------------------------------+";
    // clang-format on

    std::string commend;
    std::cout << "run / del / [quit] : ";
    std::getline(std::cin, commend);
    std::transform(commend.begin(), commend.end(), commend.begin(), ::tolower);

    if (commend == "run") {
      // clang-format off
      CLOG(ERROR, "BHCH") << "+-------------------------------------------------------------------------+";
      CLOG(ERROR, "BHCH") << "| Warning! No guarantee to work correctly. :(                             |";
      CLOG(ERROR, "BHCH") << "+-------------------------------------------------------------------------+";
      // clang-format on

      endCheck(ExitCode::ERROR_BLOCK_HEALTH_SKIP);
      return;

    } else if (commend == "del") {

      // TODO : delete blocks in the storage

    } else {
      // clang-format off
      CLOG(INFO, "BHCH") << "+-------------------------------------------------------------------------+";
      CLOG(INFO, "BHCH") << "| Bye.                                                                    |";
      CLOG(INFO, "BHCH") << "+-------------------------------------------------------------------------+";
      // clang-format on

      endCheck(ExitCode::ERROR_ABORT);
      return;
    }

  } else {
    // clang-format off
    CLOG(INFO, "BHCH") << "+-------------------------------------------------------------------------+";
    CLOG(INFO, "BHCH") << "| All blocks are clear. :)                                                |";
    CLOG(INFO, "BHCH") << "+-------------------------------------------------------------------------+";
    CLOG(INFO, "BHCH") << "BLOCK HEALTH CHECKING ---- END";
    // clang-format on
  }

  endCheck(ExitCode::NORMAL);
}

void BlockHealthChecker::endCheck(ExitCode exit_code) {
  stageOver(exit_code);
} // keep running post handler

} // namespace gruut