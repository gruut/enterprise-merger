#ifndef GRUUT_ENTERPRISE_MERGER_MERKLE_TREE_HPP
#define GRUUT_ENTERPRISE_MERGER_MERKLE_TREE_HPP

#include "types.hpp"
#include <unordered_map>
#include <utility>
#include <vector>

using namespace std;

namespace gruut {
class MerkleTree {
public:
  sha256 generate(vector<sha256> &ids);
  const unordered_map<sha256, pair<sha256, sha256>> &getTree() const;

private:
  sha256 makeParentNode(const sha256 &l, const sha256 &r);
  unordered_map<sha256, pair<sha256, sha256>> m_tree;
};
}
#endif
