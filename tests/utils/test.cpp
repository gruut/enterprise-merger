#define BOOST_TEST_MODULE

#include <boost/test/unit_test.hpp>
#include <string>
#include <ctime>

#include "../../src/utils/sha256.hpp"
#include "../../src/utils/compressor.hpp"
#include "../../src/utils/rsa.hpp"
#include "../../src/utils/random_number_generator.hpp"
#include "../../src/utils/hmac.hpp"
#include "../../src/utils/hmac_key_maker.hpp"
#include "../../src/utils/bytes_builder.hpp"

using namespace std;

BOOST_AUTO_TEST_SUITE(Test_SHA256)

    BOOST_AUTO_TEST_CASE(hash) {
        std::string message = "1";
        auto hashed_list = Sha256::hash(message);

        bool result = Sha256::toString(hashed_list) == "a4ayc/80/OGda4BO/1o/V0etpOqiLx1JwB5S3beHW0s=";
        BOOST_TEST(result);
    }

    BOOST_AUTO_TEST_CASE(isMatch) {
        std::string message = "Hello World";
        auto encrypt_message = Sha256::hash(message);

        bool result = Sha256::isMatch(message, encrypt_message);
        BOOST_TEST(result);
    }

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Test_Compressor)

    BOOST_AUTO_TEST_CASE(compressDataAnddecompressData) {
        string original = "2013-01-07 00:00:04,0.98644,0.98676 2013-01-07 00:01:19,0.98654,0.98676 2013-01-07 00:01:38,0.98644,0.98696";
        string compressed_data, decompressed_data;

        int compressed_size = Compressor::compressData(original, compressed_data);
        Compressor::decompressData(compressed_data, decompressed_data, compressed_size);

        BOOST_TEST(decompressed_data == original);
    }

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Test_RSA_Sig)
  BOOST_AUTO_TEST_CASE(signAndVerifyMessage) {
    std::string user_pk_cert = R"UPK(-----BEGIN CERTIFICATE-----
MIIDLDCCAhQCBgEZlK1CPjANBgkqhkiG9w0BAQsFADBVMQswCQYDVQQGEwJBVTET
MBEGA1UECAwKU29tZS1TdGF0ZTEhMB8GA1UECgwYSW50ZXJuZXQgV2lkZ2l0cyBQ
dHkgTHRkMQ4wDAYDVQQDDAVHcnV1dDAeFw0xODExMjQxNDEyMTRaFw0xODEyMjQx
NDEyMTRaMF4xCzAJBgNVBAYTAkFVMRMwEQYDVQQIDApTb21lLVN0YXRlMSEwHwYD
VQQKDBhJbnRlcm5ldCBXaWRnaXRzIFB0eSBMdGQxFzAVBgNVBAMMDjEyMDM4MTAy
MzgwMTIzMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA35Jf7Am6eBdy
zg5cmYHr+/tLvgKh8rIK0C9kJBFZ8a/se7XsDWjaF1Fxbm4YCCrY7pYAglBzOtJX
at1mi6TNgE9UGdyvo++R4sE2JSfCErCLEvtxPVV0f09LjOm2Z46Uc3AVXSdTVCas
OJxM3dda20/LlZT0xm7BtBpY7IspU/ZcqN4d2vaNbaZyCIQtzZV403eM6l92AhsA
cusOwlNLdw+7p/RlzjYs99vKyxLhz9mRPvsbnjJIurkRSjYX+C4jjNDvEJMOCCCH
UM2xy8dyYFpJFqqgcdjk6frWBMGbYRTvX4LNG4b2QOy/SAcvTOlQi/bKRLM+3XQG
hGXBXMn75QIDAQABMA0GCSqGSIb3DQEBCwUAA4IBAQCJs2hY8bMIWf4yw+5zbNIF
/aJqQRvu/FoeXkq8dcxxUj2c/s+rlxuoxhPUVnR3Q0fUwVxIN/23Ai3KFxk1nknO
7KkAsUkoCFJcZqYN2+rdIA/NJ6N1Hm8S7zXo5IexKmaNluMk3QoCBwraX+XgjGR6
FpgiTIvKlgMy97Mg/3rl3DyyC8MwsAF4Jpna16zYhkOKsOpB3/6zp10zvSNVaDhZ
dJ6MSWuZ1c6H/ConxqJJ4Ig274L9AYqV4KBslD9BN3+BSgPUOazCYkEkEgbNIBpr
IcLvsK86b2kKeMgNiI32t/M5rw53EzxKQyzg8vFqKtrj4z/UgtuK++G/PS575B9q
-----END CERTIFICATE-----)UPK";

    std::string user_sk_pkcs8 = R"USK(-----BEGIN PRIVATE KEY-----
MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQDfkl/sCbp4F3LO
DlyZgev7+0u+AqHysgrQL2QkEVnxr+x7tewNaNoXUXFubhgIKtjulgCCUHM60ldq
3WaLpM2AT1QZ3K+j75HiwTYlJ8ISsIsS+3E9VXR/T0uM6bZnjpRzcBVdJ1NUJqw4
nEzd11rbT8uVlPTGbsG0GljsiylT9lyo3h3a9o1tpnIIhC3NlXjTd4zqX3YCGwBy
6w7CU0t3D7un9GXONiz328rLEuHP2ZE++xueMki6uRFKNhf4LiOM0O8Qkw4IIIdQ
zbHLx3JgWkkWqqBx2OTp+tYEwZthFO9fgs0bhvZA7L9IBy9M6VCL9spEsz7ddAaE
ZcFcyfvlAgMBAAECggEBAM+JzJuLiFrUwZEAiftCPPMkGvKe9QEbP6h0ZcyJguo1
uhw5C5CDJfkBdH/jmVFznP8VphFSZzVSby3Xqsq0yMN0YIjFcRKIYO+TFhU1rBW3
ZtLPMRaTjlpkHKkJh3boR2xFvr9Dsznp0HOYvE4vDLuLflwz82mFBTGQR74FjO7P
hoR4LIQ+Pk5MuyI7Q7Jn0DZ7hkNrnL1g5g+UCxGy/CDNPlGw48t9Ftpsy+z0gVan
Cgl3fAW0+ufHvm3QyxMlNmK2zuQKNcJSLeTTEtURV8ww5ZuKB9XEMOm2GzMCakRd
eeb1/wPqwyQEMHT9Ll01R2dIjU8u1ICI1x/R7rhL6mECgYEA86+LZnwGS18gY5Ck
6cqCvo38cA9F70CrU4FK/J3BboXME59i5fcUTSUA7l20cXi+hS2fAp81sKUc3nAu
tp0/B6dVeuzgVhfboRatM6BhGYHu+BjfwHAjPbgM7VBzJpVAwguuCO4ia8MWZZAk
Rd78QSbtKtgY+d8bZjxe6Kh/cz0CgYEA6t6f91fBq1mx0UnunduP+rTs/4fhGeIJ
b94P37C/36lkAsucYzZEOPV1YEWxw4OlN40BIco125KhWj6KE3HqrhGeaB0cuwqz
GuPFbO3MdDj51SUYrcGJuj2edXKZyCgsd6wFp5FWSdH8n8amQbk3aHmV1GUsys0V
BKWf0P+elckCgYAuuGhcpMi8KKfYDwJfRJFeoXBVt8frwBVY9EABQOm2G/btiDB4
8K82vzJ3gQW4f7Lfa8jBwu6TSITJbO632lwcRovP/pxgRUC5mNRqQoR7VHsRnAtC
JP3Mtn3b/gGl0xXQXlbmpWl6CbRAkqsxrjfk8eakwTvApHLnXgnAR5Xv7QKBgQCC
kUqahVWr/UwGDjSx2wp6lDQghhhUfD1EzE1EzIyOOSvZBfoliVh51bLv1y7QgxHJ
BQE5GKHCNAyxD41Q7AZLyI2oUW7UaElTTIZHXRdJEReKL3o9thbryy+ZGSF2jSbT
THVER16R4UOwSw3IAcBUuyrZDXnOMB5cG/rxg/lUSQKBgEtqijnRFo/CM4QpjXgH
okpmJkjwucI3JAGcdZ+tNgUxwKnmwhgkbS7NVuLkZlNqVHfRMzbzDIxprPFy9t5T
wCHB5M/bx9u2PbYfUTJq0m+qJyuWOCxbqi4JNRIKk9Lvy5EBBneU4wrCRovlh7Jg
Wcql+Gep9ebzfGArFp7anHE9
-----END PRIVATE KEY-----)USK";

    std::string msg = "Hello, World!";

    std::vector<uint8_t> sig = RSA::doSign(user_sk_pkcs8,msg,true);
    bool result = RSA::doVerify(user_pk_cert,msg,sig,true);

    BOOST_TEST(result);

  }

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Test_RandomNumberGenerator)

    BOOST_AUTO_TEST_CASE(randomize) {
      auto buffer = RandomNumGenerator::randomize(64);

      BOOST_TEST(buffer.size() == 64);
    }

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Test_Hmac)

BOOST_AUTO_TEST_CASE(verifyHMAC) {
  std::vector<uint8_t> key;
  key.resize(32);
  for(int i=0; i<32; i++)
    key[i] = 0xFF;

  std::string msg = "gruut";
  std::vector<uint8_t> hmac = Hmac::generateHMAC(msg, key);

  BOOST_TEST(Hmac::verifyHMAC(msg, hmac, key));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Test_ECDH)

  BOOST_AUTO_TEST_CASE(genrateSharedKey) {

    std::string alice_sk_str = "0x2b0e4495ec270d7875311cd728ea946b11a1e587d1e21b314d5cd6f9831fa434";

    HmacKeyMaker bob_ecdh;
    bob_ecdh.genRandomSecretKey();
    std::pair<std::string,std::string> bob_xy = bob_ecdh.getPublicKey();

    std::string bob_x_str = bob_xy.first;
    std::string bob_y_str = bob_xy.second;

    HmacKeyMaker alice_ecdh;
    alice_ecdh.setSecretKey(alice_sk_str);
    std::pair<std::string,std::string> alice_xy = alice_ecdh.getPublicKey();

    std::string alice_x_str = alice_xy.first;
    std::string alice_y_str = alice_xy.second;

    Botan::secure_vector<uint8_t> alice_ssk = alice_ecdh.getSharedSecretKey(bob_x_str, bob_y_str);
    Botan::secure_vector<uint8_t> bob_ssk = bob_ecdh.getSharedSecretKey(alice_x_str, alice_y_str);

    BOOST_TEST(Botan::hex_encode(alice_ssk) == Botan::hex_encode(bob_ssk));
  }

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(Test_ByteBuilder)

  BOOST_AUTO_TEST_CASE(appendAndGet) {

    BytesBuilder my_builder;
    my_builder.append(std::time(nullptr)); // 8-byte
    my_builder.append("Hello,World!!!!!"); // 16-byte
    my_builder.appendDec("1543323592"); // 8-byte
    my_builder.appendB64("AAAAAAAAAAE="); // 8-byte
    my_builder.appendB64("Sv0pJ9tbpvFJVYCE3HaCRZSKFkX6Z9M8uKaI+Y6LtVg="); // 32-byte
    my_builder.appendHex("9A68C15C0D6E26C8B4A0743E6B87F074864C2FAE5983C88956CB2882D608F5F5"); // 32-byte

    std::vector<uint8_t> b_bytes = my_builder.getBytes();
    std::string b_str = my_builder.getString();

    BOOST_TEST((b_bytes.size() == 104 && b_str.size() == 104));
  }

BOOST_AUTO_TEST_SUITE_END()