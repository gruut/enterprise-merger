#ifndef GRUUT_ENTERPRISE_MERGER_TIME_HPP
#define GRUUT_ENTERPRISE_MERGER_TIME_HPP

#include <chrono>
#include <string>

class Time {
public:
  static std::string now() {
    auto now = now_int();

    return std::to_string(now);
  }

  static uint64_t now_int() {
    auto now = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());

    return now;
  }

  static uint64_t from_now(uint64_t seconds) {
    auto now = now_int();
    return now + seconds;
  }
};

#endif
