#ifndef GRUUT_ENTERPRISE_MERGER_MODULE_H
#define GRUUT_ENTERPRISE_MERGER_MODULE_H

#include "../chain/types.hpp"

#include <functional>

namespace gruut {
class Module {
public:
  virtual void start() = 0;
  void registCallBack(std::function<void(ExitCode)> callback) {
    m_callback = std::move(callback);
  };
  void stageOver(ExitCode exit_code) {
    if (m_callback)
      m_callback(exit_code);
  }

private:
  std::function<void(ExitCode)> m_callback;
};
} // namespace gruut
#endif
