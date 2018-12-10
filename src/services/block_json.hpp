#ifndef GRUUT_ENTERPRISE_MERGER_BLOCK_JSON_HPP
#define GRUUT_ENTERPRISE_MERGER_BLOCK_JSON_HPP

#include "../../../include/nlohmann/json.hpp"

namespace gruut {
using nlohmann::json;
using namespace std;

json block_header1 = {
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
    {"mSig",
     "e6452a2cb71aa864aaa959e647e7a4726a22e640560f199f79b56b5502114c37"},
    {"prevbID",
     "b6f6991d03df0e2e04dafffcd6bc418aac66049e2cd74b80f14ac86db1e3f0da"},
    {"prevH",
     "00000000000007d0f98d9edca880a6c124e25095712df8952e0439ac7409738a"},
    {"txrt",
     "935aa0ed2e29a4b81e0c995c39e06995ecce7ddbebb26ed32d550a72e8200bf5"}};

json block_body1 = {
    {"tx",
     {
         {{"txID", "a"},
          {"time", "a"},
          {"rID", "a"},
          {"rSig", "a"},
          {"type", "certificates"},
          {"content",
           {"a1", "certa1", "a2", "certa2", "a3", "certa3", "a4", "certa4"}}},
         {{"txID", "b"},
          {"time", "b"},
          {"rID", "b"},
          {"rSig", "b"},
          {"type", "certificates"},
          {"content",
           {"b1", "certb1", "b2", "certb2", "b3", "certb3", "b4", "certb4"}}},
         {{"txID", "c"},
          {"time", "c"},
          {"rID", "c"},
          {"rSig", "c"},
          {"type", "certificates"},
          {"content",
           {"c1", "certc1", "c2", "certc2", "c3", "certc3", "c4", "certc4"}}},
         {{"txID", "d"},
          {"time", "d"},
          {"rID", "d"},
          {"rSig", "d"},
          {"type", "digests"},
          {"content",
           {"dd1", "dd2", "dd3", "dd4", "dd5", "dd6", "dd7", "dd8"}}},
     }},
    //{"mtree", {"ee", "ff"}}, 4096*2-1개라서 Test 파일에서 반복문 돌려
    //생성하였음
    {"txCnt", "4"}};

json block_header2 = {
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
    {"mSig",
     "e6452a2cb71aa864aaa959e647e7a4726a22e640560f199f79b56b5502114c37"},
    {"prevbID",
     "b6f6991d03df0e2e04dafffcd6bc418aac66049e2cd74b80f14ac86db1e3f0da"},
    {"prevH",
     "00000000000007d0f98d9edca880a6c124e25095712df8952e0439ac7409738a"},
    {"txrt",
     "935aa0ed2e29a4b81e0c995c39e06995ecce7ddbebb26ed32d550a72e8200bf5"}};

json block_body2 = {
    {"tx",
     {
         {{"txID", "Qa"},
          {"time", "a"},
          {"rID", "a"},
          {"rSig", "a"},
          {"type", "certificates"},
          {"content",
           {"Qa1", "certQa1", "Qa2", "certQa2", "Qa3", "certQa3", "Qa4",
            "certQa4"}}},
         {{"txID", "Qb"},
          {"time", "b"},
          {"rID", "b"},
          {"rSig", "b"},
          {"type", "certificates"},
          {"content",
           {"Qb1", "certQb1", "Qb2", "certQb2", "Qb3", "certQb3", "Qb4",
            "certQb4"}}},
         {{"txID", "Qc"},
          {"time", "c"},
          {"rID", "c"},
          {"rSig", "c"},
          {"type", "certificates"},
          {"content",
           {"Qc1", "certQc1", "Qc2", "certQc2", "Qc3", "certQc3", "Qc4",
            "certQc4"}}},
         {{"txID", "Qd"},
          {"time", "d"},
          {"rID", "d"},
          {"rSig", "d"},
          {"type", "digests"},
          {"content",
           {"dd1", "dd2", "dd3", "dd4", "dd5", "dd6", "dd7", "dd8"}}},
     }},
    //{"mtree", {"ee", "ff"}}, 4096*2-1개라서 Test 파일에서 반복문 돌려
    //생성하였음
    {"txCnt", "4"}};
} // namespace gruut
#endif