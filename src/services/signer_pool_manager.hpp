#ifndef GRUUT_ENTERPRISE_MERGER_SIGNER_MANAGER_HPP
#define GRUUT_ENTERPRISE_MERGER_SIGNER_MANAGER_HPP

#include <set>
#include <vector>

#include "../chain/signer.hpp"

namespace gruut {
    using SignerPool = std::vector<Signer>;
    using RandomSignerIndices = std::set<int>;

    class SignerPoolManager {
    public:
        SignerPool getSigners();
        SignerPool getSelectedSigners();
        void putSigner(Signer&& s);
    private:
        RandomSignerIndices generateRandomNumbers(unsigned int size);
        SignerPool m_signer_pool;
        shared_ptr<SignerPool> m_selected_signers_pool;
    };
}
#endif
