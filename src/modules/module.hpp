#ifndef GRUUT_ENTERPRISE_MERGER_MODULE_H
#define GRUUT_ENTERPRISE_MERGER_MODULE_H

#include <functional>

namespace gruut {
class Module {
public:
  virtual void start() = 0;
  void registCallBack(std::function<void(int)> callback) {
    m_callback = std::move(callback);
  };
  void stageOver(int exit_code) {
    if (m_callback)
      m_callback(exit_code);
  }

private:
  std::function<void(int)> m_callback;
};
} // namespace gruut
#endif
