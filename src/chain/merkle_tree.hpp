#ifndef GRUUT_ENTERPRISE_MERGER_MERKLE_TREE_HPP
#define GRUUT_ENTERPRISE_MERGER_MERKLE_TREE_HPP

#include <vector>
#include "types.hpp"

namespace gruut {
    class MerkleTree {
    public:
        sha256 generate(std::vector<sha256> ids);
    private:
        sha256 make_parent_node(const sha256& l, const sha256& r);
    };
}
#endif
