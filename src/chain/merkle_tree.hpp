#pragma once

#include <map>
#include <vector>

#include "../../include/base64.hpp"
#include "../chain/transaction.hpp"
#include "../config/config.hpp"
#include "../utils/bytes_builder.hpp"
#include "../utils/sha256.hpp"
#include "types.hpp"

using namespace std;
using namespace gruut::config;

namespace gruut {

const std::map<std::string, std::string> HASH_LOOKUP = {
    {"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
     "AAAAAAAAAAAAA==",
     "9aX9QtFqIDAnmO9u0wmXm0MAPSMg2fDo6pgxqSdZ+0s="}, // 1
    {"9aX9QtFqIDAnmO9u0wmXm0MAPSMg2fDo6pgxqSdZ+"
     "0v1pf1C0WogMCeY727TCZebQwA9IyDZ8OjqmDGpJ1n7Sw==",
     "21YRTgD91MH4XIkr81rJqJKJquyx69CpbN5ganSLXXE="}, // 2
    {"21YRTgD91MH4XIkr81rJqJKJquyx69CpbN5ganSLXXHbVhFOAP3UwfhciSvzWsmokomq7LHr0"
     "Kls3mBqdItdcQ==",
     "x4AJ/fB/xWoR8SI3BlijU6qlQu1j5ExLwV/0zRBaszw="}, // 3
    {"x4AJ/fB/xWoR8SI3BlijU6qlQu1j5ExLwV/0zRBaszzHgAn98H/"
     "FahHxIjcGWKNTqqVC7WPkTEvBX/TNEFqzPA==",
     "U22Yg38t0WWlXV7q6RSFlURy1W8kbfJWvzyuGTUqEjw="}, // 4
    {"U22Yg38t0WWlXV7q6RSFlURy1W8kbfJWvzyuGTUqEjxTbZiDfy3RZaVdXurpFIWVRHLVbyRt8"
     "la/PK4ZNSoSPA==",
     "nv3gUqoVQp+uBbrU0LHXxk2mTQPXoYVKWIwsuEMMDTA="}, // 5
    {"nv3gUqoVQp+uBbrU0LHXxk2mTQPXoYVKWIwsuEMMDTCe/"
     "eBSqhVCn64FutTQsdfGTaZNA9ehhUpYjCy4QwwNMA==",
     "2I3f7tQAqHVVlrIZQsFJfhFMMC5hGCkPkeZ3KXYEH6E="}, // 6
    {"2I3f7tQAqHVVlrIZQsFJfhFMMC5hGCkPkeZ3KXYEH6HYjd/"
     "u1ACodVWWshlCwUl+EUwwLmEYKQ+R5ncpdgQfoQ==",
     "h+sN26V+NfbShmc4AqSvWXXiJQbHz0xku2vl7hFSfyw="}, // 7
    {"h+sN26V+"
     "NfbShmc4AqSvWXXiJQbHz0xku2vl7hFSfyyH6w3bpX419tKGZzgCpK9ZdeIlBsfPTGS7a+"
     "XuEVJ/LA==",
     "JoRkdv1fxUpdQzhRZ8lRRPJkP1M8yFu50Wt4L419sZM="}, // 8
    {"JoRkdv1fxUpdQzhRZ8lRRPJkP1M8yFu50Wt4L419sZMmhGR2/V/FSl1DOFFnyVFE8mQ/"
     "UzzIW7nRa3gvjX2xkw==",
     "UG2GWC0lJAW4QAGHksrSvxJZ8e9apfiH4Tyy8AlPUeE="}, // 9
    {"UG2GWC0lJAW4QAGHksrSvxJZ8e9apfiH4Tyy8AlPUeFQbYZYLSUkBbhAAYeSytK/"
     "Elnx71ql+IfhPLLwCU9R4Q==",
     "//8K1+ZZdy+VNMGVyBXvxAFO8eHa7UQEwGOF0RGS6Ss="}, // 10
    {"//8K1+ZZdy+VNMGVyBXvxAFO8eHa7UQEwGOF0RGS6Sv//wrX5ll3L5U0wZXIFe/"
     "EAU7x4drtRATAY4XREZLpKw==",
     "bPBBJ9sFRBzYMxB6Ur6FKGiJDkMX5qAqtHaDqnWWQiA="} // 11
};

class MerkleTree {
public:
  MerkleTree() {
    m_merkle_tree.resize(MAX_MERKLE_LEAVES * 2 - 1);

    for (auto it_map = HASH_LOOKUP.begin(); it_map != HASH_LOOKUP.end();
         ++it_map) {

      std::string sha256_str = macaron::Base64::Decode(
          std::string(it_map->second.begin(), it_map->second.end()));
      m_hash_lookup.emplace(it_map->first,
                            sha256(sha256_str.begin(), sha256_str.end()));
    }
  }

  void generate(vector<sha256> &tx_digests) {
    const bytes dummy_leaf(32, 0); // for SHA-256

    auto min_addable_size = min(MAX_MERKLE_LEAVES, tx_digests.size());

    for (size_t i = 0; i < min_addable_size; ++i)
      m_merkle_tree[i] = tx_digests[i];

    for (size_t i = min_addable_size; i < MAX_MERKLE_LEAVES; ++i)
      m_merkle_tree[i] = dummy_leaf;

    size_t parent_pos = MAX_MERKLE_LEAVES;
    for (size_t i = 0; i < MAX_MERKLE_LEAVES * 2 - 3; i += 2) {
      m_merkle_tree[parent_pos] =
          makeParent(m_merkle_tree[i], m_merkle_tree[i + 1]);
      ++parent_pos;
    }
  }

  void generate(vector<Transaction> &transactions) {
    vector<sha256> tx_digests;
    generateTxDigests(tx_digests, transactions);
    generate(tx_digests);
  }

  vector<sha256> getMerkleTree() { return m_merkle_tree; }

private:
  sha256 makeParent(sha256 left, sha256 &right) {
    left.insert(left.cend(), right.cbegin(), right.cend());
    std::string lookup_key =
        macaron::Base64::Encode(std::string(left.begin(), left.end()));

    auto it_map = m_hash_lookup.find(lookup_key);
    if (it_map != m_hash_lookup.end()) {
      return it_map->second;
    } else {
      return Sha256::hash(left);
    }
  }

  void generateTxDigests(vector<sha256> &tx_digests,
                         vector<Transaction> &transactions) {
    transform(transactions.begin(), transactions.end(),
              back_inserter(tx_digests), [](Transaction &t) {
                BytesBuilder tx_digest_builder;

                tx_digest_builder.append(t.transaction_id);
                tx_digest_builder.append(t.sent_time);
                tx_digest_builder.append(t.requestor_id);

                string transaction_type_str;
                if (t.transaction_type == TransactionType::CERTIFICATE)
                  transaction_type_str = "CERTIFICATE";
                else
                  transaction_type_str = "DIGESTS";
                tx_digest_builder.append(transaction_type_str);

                for_each(t.content_list.begin(), t.content_list.end(),
                         [&tx_digest_builder](content_type &content) {
                           tx_digest_builder.append(content);
                         });

                tx_digest_builder.append(t.signature);

                auto tx_digest_bytes = tx_digest_builder.getBytes();
                return Sha256::hash(tx_digest_bytes);
              });
  }

  vector<sha256> m_merkle_tree;
  std::map<std::string, sha256> m_hash_lookup;
};
} // namespace gruut