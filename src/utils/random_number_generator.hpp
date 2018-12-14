#ifndef GRUUT_ENTERPRISE_MERGER_RANDOM_NUM_GENERATOR_HPP
#define GRUUT_ENTERPRISE_MERGER_RANDOM_NUM_GENERATOR_HPP

#include <botan/auto_rng.h>
#include <botan/base64.h>
#include <botan/rng.h>
#include <vector>

using namespace std;

class RandomNumGenerator {
public:
  static vector<uint8_t> randomize(size_t random_number_bytes) {
    std::unique_ptr<Botan::RandomNumberGenerator> generator;
    generator.reset(new Botan::AutoSeeded_RNG());

    uint8_t buffer[random_number_bytes];
    generator->randomize(buffer, random_number_bytes);

    vector<uint8_t> random_number(&buffer[0], &buffer[random_number_bytes]);

    return random_number;
  }

  static string toString(vector<uint8_t> random_number_list) {
    return Botan::base64_encode(random_number_list);
  }
};

#endif
