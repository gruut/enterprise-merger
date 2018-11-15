//
// Created by ms on 2018-11-12.
//

#pragma once

#include <iostream>
#include <vector>

namespace gruut {
    struct SignerInfoManager {
        std::string signer_id;
        std::string cert;
        int last_update{0};
        int status{0};
    };
}

namespace gruut{

    class SignerPoolManager{

    public:
        bool push(SignerInfoManager &entity) {
            m_pool_manager.emplace_back(entity);
        }

        bool push(std::string &signer_id, std::string &cert, int last_update, int status){
            SignerInfoManager temp;
            temp.signer_id = signer_id;
            temp.cert = cert;
            temp.last_update = last_update;
            temp.status = status;
            m_pool_manager.emplace_back(temp);
        }
    private:
        std::vector<SignerInfoManager> m_pool_manager;
    };
}
