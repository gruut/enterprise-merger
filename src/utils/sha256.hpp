#ifndef GRUUT_ENTERPRISE_MERGER_SHA_256_HPP
#define GRUUT_ENTERPRISE_MERGER_SHA_256_HPP

#include <botan/hash.h>
#include <botan/hex.h>
#include <string>
#include <sstream>
#include "../chain/types.hpp"

using namespace std;
using namespace Botan;

class Sha256 {
public:
    static string encrypt(const string &message) {
        std::unique_ptr<Botan::HashFunction> hash_function(Botan::HashFunction::create("SHA-256"));
        secure_vector<uint8_t> v(message.begin(), message.end());
        hash_function->update(v);

        return Botan::hex_encode(hash_function->final());
    }

    static bool isMatch(const string &target_message, const string &encrypted_message) {
        auto encrypted_target_message = Sha256::encrypt(target_message);
        bool result = encrypted_target_message == encrypted_message;

        return result;
    }
};

#endif
