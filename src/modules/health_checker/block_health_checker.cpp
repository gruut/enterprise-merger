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
    CLOG(INFO, "BHCH") << "SKIP BLOCK HEATH CHECKING (no block in storage)";
    endCheck();
    return;
  }

  std::unique_ptr<Block> test_block;

  for (size_t i = 1; i < latest_block_info.height; ++i) {
    read_block_type nth_block = storage->readBlock(i);
    test_block.reset(new Block);
    test_block->initialize(nth_block);
    if (!test_block->isValid()) {
    }
  }
}

void BlockHealthChecker::endCheck() { stageOver(ExitCode::NORMAL); }

} // namespace gruut