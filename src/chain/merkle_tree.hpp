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
  void generate(vector<sha256> &ids);

private:
  void makeParentNode(const sha256 &l, const sha256 &r);
};
} // namespace gruut
#endif
