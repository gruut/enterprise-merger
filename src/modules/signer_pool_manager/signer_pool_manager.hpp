#ifndef GRUUT_HANA_MERGER_SIGNER_MANAGER_HPP
#define GRUUT_HANA_MERGER_SIGNER_MANAGER_HPP

#include <random>
#include <set>
#include <list>
#include <algorithm>

#include "../module.hpp"
#include "../../application.hpp"
#include "../../chain/signer.hpp"

namespace gruut {
    using namespace std;

    const unsigned int REQUEST_NUM_OF_SIGNER = 5;

    class SignerPoolManager {
        using SignerPool = vector<Signer>;
        using RandomSignerIndices = set<int>;

    public:
        vector<Signer> getSigners() {
            vector<Signer> random_signers;

            const auto requested_signers_size = min(static_cast<unsigned int>(m_signer_pool.size()), REQUEST_NUM_OF_SIGNER);
            auto chosen_signers_index_set = generateRandomNumbers(requested_signers_size);

            for(auto index : chosen_signers_index_set)
                random_signers.emplace_back(m_signer_pool[index]);

            return random_signers;
        }

        void putSigner(Signer&& s) {
            m_signer_pool.emplace_back(s);
        }

    private:
        RandomSignerIndices generateRandomNumbers(unsigned int size) {
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
        SignerPool m_signer_pool;
    };
}
#endif
