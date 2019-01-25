#ifndef GRUUT_ENTERPRISE_MERGER_BLOCK_HEALTH_HPP
#define GRUUT_ENTERPRISE_MERGER_BLOCK_HEALTH_HPP

#include "../../chain/block.hpp"
#include "../../chain/types.hpp"
#include "../../config/config.hpp"
#include "../../services/storage.hpp"

#include "../module.hpp"

#include <algorithm>
#include <functional>
#include <memory>
#include <string>

namespace gruut {

class BlockHealthChecker : public Module {
private:
public:
  BlockHealthChecker();
  void start() override;

  bool isFinished();

private:
  void startHealthCheck();
  void endCheck(ExitCode exit_code);
  std::atomic<bool> m_finish{false};
};

} // namespace gruut

#endif // GRUUT_ENTERPRISE_MERGER_BLOCK_HEALTH_HPP
