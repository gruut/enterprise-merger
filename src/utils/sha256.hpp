#ifndef GRUUT_ENTERPRISE_MERGER_SHA_256_HPP
#define GRUUT_ENTERPRISE_MERGER_SHA_256_HPP

#include <cryptopp/sha.h>
#include <cryptopp/hex.h>

#include <string>
#include <iostream>

using namespace std;

class Sha256 {
    using byte = CryptoPP::byte;

public:
    static string encrypt(const string &message) {
        CryptoPP::SHA256 hash;
        byte digest[CryptoPP::SHA256::DIGESTSIZE];
        hash.CalculateDigest(digest, (byte *) message.c_str(), message.length());

        CryptoPP::HexEncoder encoder;
        string result;
        encoder.Attach(new CryptoPP::StringSink(result));
        encoder.Put(digest, sizeof(digest));
        encoder.MessageEnd();

        std::cout << result << std::endl;

        return result;
    }

    static bool isMatch(const string &target_message, const string &encrypted_message) {
        auto encrypted_target_message = Sha256::encrypt(target_message);
        bool result = encrypted_target_message == encrypted_message;

        return result;
    }

private:
};

#endif
