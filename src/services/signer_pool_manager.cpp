#include <algorithm>
#include <random>

#include "signer_pool_manager.hpp"

namespace gruut {
    const unsigned int REQUEST_NUM_OF_SIGNER = 5;

    SignerPool SignerPoolManager::getSigners() {
        SignerPool random_signers;

        const auto requested_signers_size = min(static_cast<unsigned int>(m_signer_pool.size()), REQUEST_NUM_OF_SIGNER);
        auto chosen_signers_index_set = generateRandomNumbers(requested_signers_size);

        for(auto index : chosen_signers_index_set)
            random_signers.emplace_back(m_signer_pool[index]);

        return random_signers;
    }

    void SignerPoolManager::putSigner(Signer &&s) {
        m_signer_pool.emplace_back(s);
    }

    RandomSignerIndices SignerPoolManager::generateRandomNumbers(unsigned int size) {
        // Generate random number in range(0, size)
        mt19937 mt;
        mt.seed(random_device()());

        RandomSignerIndices number_set;
        while(number_set.size() < size) {
            uniform_int_distribution<mt19937::result_type> dist(0, size - 1);
            int random_number = static_cast<int>(dist(mt));
            number_set.insert(random_number);
        }

        return number_set;
    }
}