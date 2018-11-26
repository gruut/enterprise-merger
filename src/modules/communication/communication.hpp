#pragma once

#include "../module.hpp"

namespace gruut {

class Communication : public Module {
public:
  void start() override { startCommunicationLoop(); }

private:
  void startCommunicationLoop();
};
} // namespace gruut
