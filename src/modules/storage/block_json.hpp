#ifndef GRUUT_ENTERPRISE_MERGER_BLOCK_JSON_HPP
#define GRUUT_ENTERPRISE_MERGER_BLOCK_JSON_HPP

#include "../../../include/nlohmann/json.hpp"

namespace gruut {
using nlohmann::json;
using namespace std;

json block_header = {
    {"bID", "1"},
    {"ver", "4"},
    {"cID", "76a914b97f902c2ddb92ad728dd23d940d64f6ba31192f88ac"},
    {"time", "1231232323"},
    {"hgt", "1"},
    {"SSig",
     {
         {
             {"sID", "0000000000000538200a48202ca6340e983646ca088c7618ae82d68e0c76ef5a"},
             {"sig", "asdf8u9h34ee"}},
         {
             {"sID", "AsdasdasdasdasdASDASDasdasdasdiojef5a"},
             {"sig", "6+5ads564asd89asd1564see"}},
         {
             {"sID", "xcfsdfijk90sisdiof"},
             {"sig", "wre78wer8ewr4568eqq3q123"}
         }
     }
    },
    {"mID", "76a914641ad5051edd97029a003fe9efb29359fcee409d88ac"},
    {"mSig", "e6452a2cb71aa864aaa959e647e7a4726a22e640560f199f79b56b5502114c37"},
    {"prevbID", "b6f6991d03df0e2e04dafffcd6bc418aac66049e2cd74b80f14ac86db1e3f0da"},
    {"prevH", "00000000000007d0f98d9edca880a6c124e25095712df8952e0439ac7409738a"},
    {"txcnt", "4"},
    {"txrt", "935aa0ed2e29a4b81e0c995c39e06995ecce7ddbebb26ed32d550a72e8200bf5"}
};

json transaction = {
    {
        {"txID", "aaaaaaaaa"},
        {"time", "234234234"},
        {"rID", "b6f6991d03df0e2e04dafffcd6bc418aac66049e2cd74b80f14ac86db1e3f0da"},
        {"rSig", "17b5038a413f2342390wujeor23238fwaef5c5ee288caa64cfab35a0c01914e"},
        {"type", "certificate"},
        {"content",
         {
             {"nID", "aax"},
             {"x509", "aaaaaaaaaaaaaaaaaaaaaaa"}
         }
        }
    },
    {
        {"txID", "bbbbbbbbb"},
        {"time", "123123123"},
        {"rID", "sdoijus89ajw3djkawnejkfnaweu9ahvwnauine49afu48ja94jrawioejr"},
        {"rSig", "2e89wdnjwnkachwuhcawkjenaw7hrbwjkdndcozsncoizsncdzipsdnc2i"},
        {"type", "checksum"},
        {"content",
         {
             {"time", "111111"},
             {"dID", "23902389jwui"},
             {"digest", "bbbbbbbbb"}
         }
        }
    },
    {
        {"txID", "ccccccccc"},
        {"time", "123123123"},
        {"rID", "sdoijus89ajw3djkawnejkfnaweu9ahvwnauine49afu48ja94jrawioejr"},
        {"rSig", "2e89wdnjwnkachwuhcawkjenaw7hrbwjkdndcozsncoizsncdzipsdnc2i"},
        {"type", "checksum"},
        {"content",
         {
             {"time", "222222"},
             {"dID", "awfer"},
             {"digest", "gdffdgb"}
         }
        }
    },
    {
        {"txID", "ddddddddd"},
        {"time", "123123123"},
        {"rID", "sdoijus89ajw3djkawnejkfnaweu9ahvwnauine49afu48ja94jrawioejr"},
        {"rSig", "2e89wdnjwnkachwuhcawkjenaw7hrbwjkdndcozsncoizsncdzipsdnc2i"},
        {"type", "certificate"},
        {"content",
         {
             {"nID", "ddx"},
             {"x509", "ddddddddddddddddddddddddddddddd"}
         }
        }
    },
    {
        {"mtree",
         {"h1", "h2", "h3", "h4", "h12", "h34", "h1234"}
        }
    }
};
}
#endif