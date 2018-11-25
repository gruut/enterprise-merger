#ifndef GRUUT_ENTERPRISE_MERGER_SHA_256_HPP
#define GRUUT_ENTERPRISE_MERGER_SHA_256_HPP

#include <botan/hash.h>
#include <botan/base64.h>
#include <sstream>
#include <string>

using namespace std;
using namespace Botan;

class Sha256 {
    using sha256 = secure_vector<uint8_t>;
public:
    static sha256 hash(const string &message) {
        std::unique_ptr<Botan::HashFunction> hash_function(
                Botan::HashFunction::create("SHA-256"));

        sha256 v(message.begin(), message.end());
        hash_function->update(v);

        return hash_function->final();
    }

    static bool isMatch(const string &target_message,
                        const sha256 &encrypted_message) {
        auto encrypted_target_message = Sha256::hash(target_message);
        bool result = encrypted_target_message == encrypted_message;

        return result;
    }

    static string toString(sha256 hashed_list) {
        return base64_encode(hashed_list);
    }
};

#endif
