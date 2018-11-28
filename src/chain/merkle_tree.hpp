/**
 * @file merkle_tree.hpp
 * @brief 트랜잭션의 머클트리를 만드는 클래스
 */

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
