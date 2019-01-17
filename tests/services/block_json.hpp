#ifndef GRUUT_ENTERPRISE_MERGER_BLOCK_JSON_HPP
#define GRUUT_ENTERPRISE_MERGER_BLOCK_JSON_HPP

#include "nlohmann/json.hpp"
#include "../../src/utils/sha256.hpp"
#include "../../src/chain/types.hpp"
namespace gruut {
using namespace std;
json block_header_sample1 = {
    {"bID", "bbbbbbbbbID1"},
    {"ver", "4"},
    {"cID", "76a914b97f902c2ddb92ad728dd23d940d64f6ba31192f88ac"},
    {"time", "1231232323"},
    {"hgt", "1"},
    {"SSig",
     {{{"sID",
        "0000000000000538200a48202ca6340e983646ca088c7618ae82d68e0c76ef5a"},
       {"sig", "asdf8u9h34ee"}},
      {{"sID", "AsdasdasdasdasdASDASDasdasdasdiojef5a"},
       {"sig", "6+5ads564asd89asd1564see"}},
      {{"sID", "xcfsdfijk90sisdiof"}, {"sig", "wre78wer8ewr4568eqq3q123"}}}},
    {"mID", "76a914641ad5051edd97029a003fe9efb29359fcee409d88ac"},
    {"prevbID",
     "b6f6991d03df0e2e04dafffcd6bc418aac66049e2cd74b80f14ac86db1e3f0da"},
    {"prevH",
     "00000000000007d0f98d9edca880a6c124e25095712df8952e0439ac7409738a"},
    {"txrt",
     "935aa0ed2e29a4b81e0c995c39e06995ecce7ddbebb26ed32d550a72e8200bf5"},
    {"txids", {"a", "b", "c"}}};

json block_body_sample1 = {
    {"tx",
     {
         {{"txID", "a"},
          {"time", "a"},
          {"rID", "a"},
          {"rSig", "a"},
          {"type", "CERTIFICATES"},
          {"content",
              // a1 : 20171210~20181210, a2 : 20160303~20170303
           {"a1",
            "-----BEGIN CERTIFICATE-----\n"
            "MIIDXTCCAkWgAwIBAgIJAJeEA17SFCGLMA0GCSqGSIb3DQEBCwUAMEUxCzAJBgNV\n"
            "BAYTAkFVMRMwEQYDVQQIDApTb21lLVN0YXRlMSEwHwYDVQQKDBhJbnRlcm5ldCBX\n"
            "aWRnaXRzIFB0eSBMdGQwHhcNMTcxMjEwMDQyNTAzWhcNMTgxMjEwMDQyNTAzWjBF\n"
            "MQswCQYDVQQGEwJBVTETMBEGA1UECAwKU29tZS1TdGF0ZTEhMB8GA1UECgwYSW50\n"
            "ZXJuZXQgV2lkZ2l0cyBQdHkgTHRkMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIB\n"
            "CgKCAQEApcYkflgKmtX9ZhAkHnn47F7lkdJv23tpdomZsg0eAsDskb1m9cDqN07O\n"
            "tm7c2/ajMLQGP2p19O3mlNJeG6ox+t+xGPR6YRLhCK9avkIZJuT3vtfAhWhTnQN9\n"
            "MMynIFB3yFbkn8mLAb7RaokDiJc7DAPiBPS0+almHjnloA+Yv64CbQ6QpLYMFjP1\n"
            "tNYO/n0Drlb4S587HhVzLa1qKlz8s+PRtWz8tqSsCwrCht+TdlnDfJWpi9/njXEh\n"
            "rrRE/T5epXhGTGJKX3tcMHqRqKvA9ImrQu7JYHjbRlcAaBlTZsh6Ty00OHv7DzPf\n"
            "nW7dQLiR3JfjThYL6JWbAfaI4f2w8wIDAQABo1AwTjAdBgNVHQ4EFgQUR4nACjxK\n"
            "58x7z5GlsuG1W06qQjswHwYDVR0jBBgwFoAUR4nACjxK58x7z5GlsuG1W06qQjsw\n"
            "DAYDVR0TBAUwAwEB/zANBgkqhkiG9w0BAQsFAAOCAQEAi0crtWanFcorOotJLOim\n"
            "Wwe89H+lU/49FR1pB1vszqb1QgAT+YHOo2I3NQ8ZjzFm+DGe6XTQLzAI5a2UNSIb\n"
            "oV0FKlWGI3bEwo9uWq7igV1lDqgyZrm7DQglW4O17dI7Yxa3saBAFYCYt4DbIRRG\n"
            "geTMX971KkN9es+djX367jUlCzW0AttjgahIkwfK/VCkok88TAscZCNuvcto2EUD\n"
            "H8Rnpg4es4nSPfbqAba8bXfNQU86hjmVyk+38IEHWctazf98TxAn0gGqEU+36gfv\n"
            "argbxR2j1auP5foPQTKz2DtpBqIzIY1eg9M0KEmaqzLA6L6qpU+l+Zw2tkM8yhUt\n"
            "WA==\n"
            "-----END CERTIFICATE-----",
            "a2",
            "-----BEGIN CERTIFICATE-----\n"
            "MIIDXTCCAkWgAwIBAgIJALV2n/vUEHSGMA0GCSqGSIb3DQEBCwUAMEUxCzAJBgNV\n"
            "BAYTAkFVMRMwEQYDVQQIDApTb21lLVN0YXRlMSEwHwYDVQQKDBhJbnRlcm5ldCBX\n"
            "aWRnaXRzIFB0eSBMdGQwHhcNMTYwMzAzMDQyNjA5WhcNMTcwMzAzMDQyNjA5WjBF\n"
            "MQswCQYDVQQGEwJBVTETMBEGA1UECAwKU29tZS1TdGF0ZTEhMB8GA1UECgwYSW50\n"
            "ZXJuZXQgV2lkZ2l0cyBQdHkgTHRkMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIB\n"
            "CgKCAQEApcYkflgKmtX9ZhAkHnn47F7lkdJv23tpdomZsg0eAsDskb1m9cDqN07O\n"
            "tm7c2/ajMLQGP2p19O3mlNJeG6ox+t+xGPR6YRLhCK9avkIZJuT3vtfAhWhTnQN9\n"
            "MMynIFB3yFbkn8mLAb7RaokDiJc7DAPiBPS0+almHjnloA+Yv64CbQ6QpLYMFjP1\n"
            "tNYO/n0Drlb4S587HhVzLa1qKlz8s+PRtWz8tqSsCwrCht+TdlnDfJWpi9/njXEh\n"
            "rrRE/T5epXhGTGJKX3tcMHqRqKvA9ImrQu7JYHjbRlcAaBlTZsh6Ty00OHv7DzPf\n"
            "nW7dQLiR3JfjThYL6JWbAfaI4f2w8wIDAQABo1AwTjAdBgNVHQ4EFgQUR4nACjxK\n"
            "58x7z5GlsuG1W06qQjswHwYDVR0jBBgwFoAUR4nACjxK58x7z5GlsuG1W06qQjsw\n"
            "DAYDVR0TBAUwAwEB/zANBgkqhkiG9w0BAQsFAAOCAQEAceNOuaasbU1idUKnU3zE\n"
            "TLDjhEZd8n8EqMa9T+lffH4hmo6WKScXjfNyB7IfncXxg6FZ+8X5CTRDRvY8iJLW\n"
            "YLggNCkyn7FUJm3zab5FlhM6ylCnRgFBoFy2EAb0+KNK/cVAfwqiAsKxG8Kzb7UW\n"
            "uX6D9/j82nEma45L4soGfljMyDcEt3Z52qPvzE+mdupb1AVe+jdr6zgE243mOJOW\n"
            "Bjvbj+AFVRyJLeynitoKKn5ck2PZDG5xjnwTYEIPvfdnd9RM2zwov9DMUxa6nL+G\n"
            "cEjnEbNinHq8SuajL3Qun3ziD3fEw5B/C6fP6bTjcmnrNTjusD/KP1Z1JwGV/FBh\n"
            "+g==\n"
            "-----END CERTIFICATE-----"}}},
         {{"txID", "b"},
          {"time", "b"},
          {"rID", "b"},
          {"rSig", "b"},
          {"type", "DIGESTS"},
          {"content",
           {"dig01", "dig02", "dig03", "dig04", "dig11", "dig12", "dig13",
            "dig14"}}},
         {{"txID", "c"},
          {"time", "c"},
          {"rID", "c"},
          {"rSig", "c"},
          {"type", "DIGESTS"},
          {"content",
           {"dig01", "dig02", "dig03", "dig04", "dig11", "dig12", "dig13",
            "dig14"}}},
     }},
    {"mtree",
     {"nv3gUqoVQp+uBbrU0LHXxk2mTQPXoYVKWIwsuEMMDTA=", "nv3gUqoVQp+uBbrU0LHXxk2mTQPXoYVKWIwsuEMMDTA=", "U22Yg38t0WWlXV7q6RSFlURy1W8kbfJWvzyuGTUqEjw="}},
    {"txCnt", "3"}};

json block_header_sample2 = {
    {"bID", "bbbbbbbbbID2"},
    {"ver", "4"},
    {"cID", "76a914b97f902c2ddb92ad728dd23d940d64f6ba31192f88ac"},
    {"time", "1231232323"},
    {"hgt", "2"},
    {"SSig",
     {{{"sID",
        "0000000000000538200a48202ca6340e983646ca088c7618ae82d68e0c76ef5a"},
       {"sig", "asdf8u9h34ee"}},
      {{"sID", "AsdasdasdasdasdASDASDasdasdasdiojef5a"},
       {"sig", "6+5ads564asd89asd1564see"}},
      {{"sID", "xcfsdfijk90sisdiof"}, {"sig", "wre78wer8ewr4568eqq3q123"}}}},
    {"mID", "76a914641ad5051edd97029a003fe9efb29359fcee409d88ac"},
    {"prevbID",
     "b6f6991d03df0e2e04dafffcd6bc418aac66049e2cd74b80f14ac86db1e3f0da"},
    {"prevH",
     "00000000000007d0f98d9edca880a6c124e25095712df8952e0439ac7409738a"},
    {"txrt",
     "935aa0ed2e29a4b81e0c995c39e06995ecce7ddbebb26ed32d550a72e8200bf5"},
    {"txids", {"Qa", "Qb", "Qc"}}};

json block_body_sample2 = {
    {"tx",
     {
         {{"txID", "Qa"},
          {"time", "Qa"},
          {"rID", "Qa"},
          {"rSig", "Qa"},
          {"type", "CERTIFICATES"},
          {"content",
              // a1 : 20180201~20190201, a2 : 20170505~20180505
           {"a1",
            "-----BEGIN CERTIFICATE-----\n"
            "MIIDXTCCAkWgAwIBAgIJAJoknRxDWC2cMA0GCSqGSIb3DQEBCwUAMEUxCzAJBgNV\n"
            "BAYTAkFVMRMwEQYDVQQIDApTb21lLVN0YXRlMSEwHwYDVQQKDBhJbnRlcm5ldCBX\n"
            "aWRnaXRzIFB0eSBMdGQwHhcNMTgwMjAxMDQyNzMxWhcNMTkwMjAxMDQyNzMxWjBF\n"
            "MQswCQYDVQQGEwJBVTETMBEGA1UECAwKU29tZS1TdGF0ZTEhMB8GA1UECgwYSW50\n"
            "ZXJuZXQgV2lkZ2l0cyBQdHkgTHRkMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIB\n"
            "CgKCAQEApcYkflgKmtX9ZhAkHnn47F7lkdJv23tpdomZsg0eAsDskb1m9cDqN07O\n"
            "tm7c2/ajMLQGP2p19O3mlNJeG6ox+t+xGPR6YRLhCK9avkIZJuT3vtfAhWhTnQN9\n"
            "MMynIFB3yFbkn8mLAb7RaokDiJc7DAPiBPS0+almHjnloA+Yv64CbQ6QpLYMFjP1\n"
            "tNYO/n0Drlb4S587HhVzLa1qKlz8s+PRtWz8tqSsCwrCht+TdlnDfJWpi9/njXEh\n"
            "rrRE/T5epXhGTGJKX3tcMHqRqKvA9ImrQu7JYHjbRlcAaBlTZsh6Ty00OHv7DzPf\n"
            "nW7dQLiR3JfjThYL6JWbAfaI4f2w8wIDAQABo1AwTjAdBgNVHQ4EFgQUR4nACjxK\n"
            "58x7z5GlsuG1W06qQjswHwYDVR0jBBgwFoAUR4nACjxK58x7z5GlsuG1W06qQjsw\n"
            "DAYDVR0TBAUwAwEB/zANBgkqhkiG9w0BAQsFAAOCAQEAjV3LJG4ppme0LcOeG4SU\n"
            "jeQ/6QkOxKSXDspoCg5u1FWU4KKdnXOOgePvRX+Je2KGMTosxBTCdm+RNrPFHuH1\n"
            "rlab/N9fL3VmfGuHSJ6jrwVeUThumzhZ2nHKMofFqSJmAyoSaadCiZ+lsja5MQmV\n"
            "feRqFnjkqcnAJAKkD7rSZzCRuk3R5IAE3iAfeTZzVaccr4lrEDzxeeonXwdUFBCd\n"
            "El7L5juFHnQTPeOcfGU80cPdCl8/hXx6RVCT28clkUiz9kzn5WOvWdUorSI8liQX\n"
            "2DEeNEcaBHwCycf9aNBd71T5gGtvnV/nHpTZefNu7Jh5ZNW6jP4P7p4l/9to8r/j\n"
            "0g==\n"
            "-----END CERTIFICATE-----",
            "a2",
            "-----BEGIN CERTIFICATE-----\n"
            "MIIDXTCCAkWgAwIBAgIJAKKfBrtyBzGgMA0GCSqGSIb3DQEBCwUAMEUxCzAJBgNV\n"
            "BAYTAkFVMRMwEQYDVQQIDApTb21lLVN0YXRlMSEwHwYDVQQKDBhJbnRlcm5ldCBX\n"
            "aWRnaXRzIFB0eSBMdGQwHhcNMTcwNTA1MDQyODAzWhcNMTgwNTA1MDQyODAzWjBF\n"
            "MQswCQYDVQQGEwJBVTETMBEGA1UECAwKU29tZS1TdGF0ZTEhMB8GA1UECgwYSW50\n"
            "ZXJuZXQgV2lkZ2l0cyBQdHkgTHRkMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIB\n"
            "CgKCAQEApcYkflgKmtX9ZhAkHnn47F7lkdJv23tpdomZsg0eAsDskb1m9cDqN07O\n"
            "tm7c2/ajMLQGP2p19O3mlNJeG6ox+t+xGPR6YRLhCK9avkIZJuT3vtfAhWhTnQN9\n"
            "MMynIFB3yFbkn8mLAb7RaokDiJc7DAPiBPS0+almHjnloA+Yv64CbQ6QpLYMFjP1\n"
            "tNYO/n0Drlb4S587HhVzLa1qKlz8s+PRtWz8tqSsCwrCht+TdlnDfJWpi9/njXEh\n"
            "rrRE/T5epXhGTGJKX3tcMHqRqKvA9ImrQu7JYHjbRlcAaBlTZsh6Ty00OHv7DzPf\n"
            "nW7dQLiR3JfjThYL6JWbAfaI4f2w8wIDAQABo1AwTjAdBgNVHQ4EFgQUR4nACjxK\n"
            "58x7z5GlsuG1W06qQjswHwYDVR0jBBgwFoAUR4nACjxK58x7z5GlsuG1W06qQjsw\n"
            "DAYDVR0TBAUwAwEB/zANBgkqhkiG9w0BAQsFAAOCAQEAoxC78x09lXsOOLNfb4Gs\n"
            "IvsXkiK/k5AWpWJfEjhSj4hO8nnbtdwsZ+ycrOMqCgZdBxCcI51u6HoFw1hDMsbU\n"
            "L4iT1oDtqf731KM7qIQLLvmnUn32T7Y2zWfzRIGiAt+mDA8sLzmx3QL2Y7hUg2av\n"
            "7AcjEWiC1ckfQ63sd7rGDoZ5D6gfzlcPzP0QiqG/GZ8PO0K+GZCUheKea/I3Rm9q\n"
            "nJwWLOAaPCkJ97cIVXxT1/Zr0kJgcE5diSKcACpwQ793F43yN55fU13fv9j5Kh5s\n"
            "aChaqtEd9TOz8PJeAqqsvrxWwxSKhYRBxExYmyvEzbGsZubwFGyUSBwKLYeELwRQ\n"
            "gA==\n"
            "-----END CERTIFICATE-----"}}},
         {{"txID", "Qb"},
          {"time", "Qb"},
          {"rID", "Qb"},
          {"rSig", "Qb"},
          {"type", "DIGESTS"},
          {"content",
           {"dig01", "dig02", "dig03", "dig04", "dig11", "dig12", "dig13",
            "dig14"}}},
         {{"txID", "Qc"},
          {"time", "Qc"},
          {"rID", "Qc"},
          {"rSig", "Qc"},
          {"type", "DIGESTS"},
          {"content",
           {"dig01", "dig02", "dig03", "dig04", "dig11", "dig12", "dig13",
            "dig14"}}},
     }},
    {"mtree",
     {"9aX9QtFqIDAnmO9u0wmXm0MAPSMg2fDo6pgxqSdZ+0s=", "9aX9QtFqIDAnmO9u0wmXm0MAPSMg2fDo6pgxqSdZ+0s=", "9aX9QtFqIDAnmO9u0wmXm0MAPSMg2fDo6pgxqSdZ+0s="}},
    {"txCnt", "3"}};

std::string block_raw_sample1_bytes = block_header_sample1.dump();
std::string block_raw_sample2_bytes = block_header_sample2.dump();
std::string block_raw_sample1_b64 = TypeConverter::encodeBase64(block_raw_sample1_bytes);
std::string block_raw_sample2_b64 = TypeConverter::encodeBase64(block_raw_sample2_bytes);
sha256 block_hash_2 = Sha256::hash(block_raw_sample2_bytes);
} // namespace gruut

#endif