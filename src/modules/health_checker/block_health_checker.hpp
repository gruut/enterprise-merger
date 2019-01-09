#ifndef GRUUT_ENTERPRISE_MERGER_BLOCK_HEALTH_HPP
#define GRUUT_ENTERPRISE_MERGER_BLOCK_HEALTH_HPP

#include "../../chain/block.hpp"
#include "../../chain/types.hpp"
#include "../../config/config.hpp"
#include "../../services/storage.hpp"

#include "../module.hpp"

#include <functional>
#include <memory>

namespace gruut {

class BlockHealthChecker : public Module {
private:
public:
  BlockHealthChecker();
  void start() override;

private:
  void startHealthCheck();
  void endCheck();
};

} // namespace gruut

#endif // GRUUT_ENTERPRISE_MERGER_BLOCK_HEALTH_HPP
