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
    {"txcnt", "4"},
    {"txrt",
     "935aa0ed2e29a4b81e0c995c39e06995ecce7ddbebb26ed32d550a72e8200bf5"}};

json transaction1 = {
    {{"txID", "aaaaaaaaa"},
     {"time", "1"},
     {"rID",
      "b6f6991d03df0e2e04dafffcd6bc418aac66049e2cd74b80f14ac86db1e3f0da"},
     {"rSig",
      "17b5038a413f2342390wujeor23238fwaef5c5ee288caa64cfab35a0c01914e"},
     {"type", "certificates"},
     {"content",
      {"AAA1", "certA1", "AAA2", "certA2", "AAA3", "certA3", "AAA4",
       "certA4"}}},
    {{"txID", "bbbbbbbbb"},
     {"time", "2"},
     {"rID", "sdoijus89ajw3djkawnejkfnaweu9ahvwnauine49afu48ja94jrawioejr"},
     {"rSig", "2e89wdnjwnkachwuhcawkjenaw7hrbwjkdndcozsncoizsncdzipsdnc2i"},
     {"type", "certificates"},
     {"content",
      {"BBB1", "certB1", "BBB2", "certB2", "BBB3", "certB3", "BBB4",
       "certB4"}}},
    {{"txID", "ccccccccc"},
     {"time", "3"},
     {"rID", "sdoijus89ajw3djkawnejkfnaweu9ahvwnauine49afu48ja94jrawioejr"},
     {"rSig", "2e89wdnjwnkachwuhcawkjenaw7hrbwjkdndcozsncoizsncdzipsdnc2i"},
     {"type", "digests"},
     {"content",
      {"R1", "T1", "D1", "DD1", "R2", "T2", "D2", "DD2", "R3", "T3", "D3",
       "DD3", "R4", "T4", "D4", "DD4"}}},
    {{"txID", "ddddddddd"},
     {"time", "4"},
     {"rID", "sdoijus89ajw3djkawnejkfnaweu9ahvwnauine49afu48ja94jrawioejr"},
     {"rSig", "2e89wdnjwnkachwuhcawkjenaw7hrbwjkdndcozsncoizsncdzipsdnc2i"},
     {"type", "certificates"},
     {"content",
      {"CCC1", "certC1", "CCC2", "certC2", "CCC3", "certC3", "CCC4",
       "certC4"}}}};

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
    {"txcnt", "8"},
    {"txrt",
     "935aa0ed2e29a4b81e0c995c39e06995ecce7ddbebb26ed32d550a72e8200bf5"}};

json transaction2 = {
    {{"txID", "QQQQaaaaaaaaa"},
     {"time", "1"},
     {"rID",
      "b6f6991d03df0e2e04dafffcd6bc418aac66049e2cd74b80f14ac86db1e3f0da"},
     {"rSig",
      "17b5038a413f2342390wujeor23238fwaef5c5ee288caa64cfab35a0c01914e"},
     {"type", "certificates"},
     {"content",
      {"QAAA1", "certQA1", "QAAA2", "certQA2", "QAAA3", "certQA3", "QAAA4",
       "certQA4"}}},
    {{"txID", "QQQQbbbbbbbbb"},
     {"time", "2"},
     {"rID", "sdoijus89ajw3djkawnejkfnaweu9ahvwnauine49afu48ja94jrawioejr"},
     {"rSig", "2e89wdnjwnkachwuhcawkjenaw7hrbwjkdndcozsncoizsncdzipsdnc2i"},
     {"type", "certificates"},
     {"content",
      {"QBBB1", "certQB1", "QBBB2", "certQB2", "QBBB3", "certQB3", "QBBB4",
       "certQB4"}}},
    {{"txID", "QQQQccccccccc"},
     {"time", "3"},
     {"rID", "sdoijus89ajw3djkawnejkfnaweu9ahvwnauine49afu48ja94jrawioejr"},
     {"rSig", "2e89wdnjwnkachwuhcawkjenaw7hrbwjkdndcozsncoizsncdzipsdnc2i"},
     {"type", "digests"},
     {"content",
      {"QR1", "QT1", "QD1", "QDD1", "QR2", "QT2", "QD2", "QDD2", "QR3", "QT3",
       "QD3", "QDD3", "QR4", "QT4", "QD4", "QDD4"}}},
    {{"txID", "QQQQddddddddd"},
     {"time", "4"},
     {"rID", "sdoijus89ajw3djkawnejkfnaweu9ahvwnauine49afu48ja94jrawioejr"},
     {"rSig", "2e89wdnjwnkachwuhcawkjenaw7hrbwjkdndcozsncoizsncdzipsdnc2i"},
     {"type", "certificates"},
     {"content",
      {"QCCC1", "certQC1", "QCCC2", "certQC2", "QCCC3", "certQC3", "QCCC4",
       "certQC4"}}}};
} // namespace gruut
#endif