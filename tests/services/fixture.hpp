#ifndef GRUUT_ENTERPRISE_MERGER_FIXTURE_HPP
#define GRUUT_ENTERPRISE_MERGER_FIXTURE_HPP

#include "../../src/chain/types.hpp"
#include "../../src/services/signer_pool.hpp"
#include "block_json.hpp"
using namespace gruut;

struct SignerPoolFixture {
  SignerPoolFixture() {
    id_int = 1;
    id = TypeConverter::integerToBytes(id_int);
    test_cert = "MIIC7zCCAdegAwIBAgIBADANBgkqhkiG9w0BAQsFADBhMRQwEgYDVQQDEwt0aGVWYXVsdGVyczELMAkGA1UEBhMCS1IxCTAHBgNVBAgTADEQMA4GA1UEBxMHSW5jaGVvbjEUMBIGA1UEChMLdGhlVmF1bHRlcnMxCTAHBgNVBAsTADAeFw0xODExMjgwNTM1NDFaFw0xOTExMjgwNTM1NDFaMBUxEzARBgNVBAMMCkdSVVVUX0FVVEgwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDV2RKC+oo6sBeAoSJn55ZZJ+U9bRh4z/TOsc4V/92NsV5qXpiWhUMTqPfNGHTjR7ScI57ZH9lqltcBJ2mcqhBWY1A/lQfdWAJf+3/eh+H/ZvDcZW8s9PFeuJcftmEDtUMlh9xMUoL5a74dS5lhrdbH0tXRMfhB3w02fmkuvqW+MCsUubhL7mu0PDbJeWjqqu8P+c+6PWO0CRgkMmry1f1VksXTzp54wARW2O3Zut6Z56VknrMOP2f4IYGiLy8zC/oO/JRCPCFvW1cM5UDdjVaq8UkIZ7B/z4zqFjwT3gXHHdMp+RLS8t+tA15rhZ2iRtKPcSwlpV95BTBG3Jpbm7xTAgMBAAEwDQYJKoZIhvcNAQELBQADggEBAEAzwa2yTQMR6nRUgafc/v0z7PylG4ohkXljBzMxfFipYju1/AKse1PCBq2H9DSnSbeL/BI4lQjsXXJLCDSnWXNnJSt1oatOHv4JeJ2Ob88EBVkx7G0aK2S2yijfMx5Bpptp8FIYxZX0QuOJ2oNK73j1Dx9Xax+5ZkBE8wxYYXpsZ0R/BGw8Es1bNFyFcbNYWd3iQOwoXOenWWa6YOyzRhZ2EAw+l7C7LB6I68xIIAP0BBSMTOfq4Smdizdd3qWYJyouUcv83AZn8KWBJjRKNJgHQvnYzCCGnhOwekbh9WlrGVEUvr/b6yV/aXX6kMqsCAfLhloqQ7Ai24QvOfdOAEQ=";
    vector<uint8_t> secret_key(32, 0);
    secret_key_vector = TypeConverter::toSecureVector(secret_key);
  }

  void push() {
    signer_pool.pushSigner(id, test_cert, secret_key_vector, SignerStatus::GOOD);
    ++id_int;
    id = TypeConverter::integerToBytes(id_int);
  }

  SignerPool signer_pool;

  signer_id_type id;
  uint64_t id_int;
  string test_cert;
  Botan::secure_vector<uint8_t> secret_key_vector;
};

class StorageFixture {
public:
  Storage *m_storage;
  bool m_save_status1;
  bool m_save_status2;
  StorageFixture() {
    m_storage = Storage::getInstance();
    m_save_status1 = m_storage->saveBlock(block_raw_sample1, block_header_sample1, block_body_sample1);
    m_save_status2 = m_storage->saveBlock(block_raw_sample2, block_header_sample2, block_body_sample2);
    if(m_save_status1&&m_save_status2)
      cout<<"DB 저장 성공"<<endl;
    else
      cout<<"DB 저장 실패"<<endl;
  }
  ~StorageFixture(){
    m_storage->deleteAllDirectory();
  }
};
#endif
