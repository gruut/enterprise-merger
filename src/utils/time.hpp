#ifndef GRUUT_ENTERPRISE_MERGER_TIME_HPP
#define GRUUT_ENTERPRISE_MERGER_TIME_HPP

#include <chrono>
#include <string>

class Time {
public:
  static std::string now() {
    auto now = std::chrono::duration_cast<std::chrono::seconds>(
                   std::chrono::system_clock::now().time_since_epoch())
                   .count();

    return std::to_string(now);
  }
};

#endif
