#ifndef GRUUT_ENTERPRISE_MERGER_RANDOM_NUM_GENERATOR_HPP
#define GRUUT_ENTERPRISE_MERGER_RANDOM_NUM_GENERATOR_HPP

#include <botan-2/botan/auto_rng.h>
#include <botan-2/botan/base64.h>
#include <botan-2/botan/rng.h>
#include <random>
#include <vector>

using namespace std;

class RandomNumGenerator {
public:
  static vector<uint8_t> randomize(size_t random_number_bytes) {
    std::unique_ptr<Botan::RandomNumberGenerator> generator(
        new Botan::AutoSeeded_RNG());

    uint8_t buffer[random_number_bytes];
    generator->randomize(buffer, random_number_bytes);

    vector<uint8_t> random_number(&buffer[0], &buffer[random_number_bytes]);

    return random_number;
  }

  static string toString(vector<uint8_t> random_number_list) {
    return Botan::base64_encode(random_number_list);
  }

  static int getRandRange(int low, int max) {
    std::random_device rd;
    std::mt19937 prng(rd());
    std::uniform_int_distribution<> dist(low, max);
    return dist(prng);
  }
};

#endif
