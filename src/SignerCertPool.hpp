//
// Created by ms on 2018-11-12.
//

#pragma once

#include <iostream>
#include <vector>

namespace gruut {
    struct SignerInfo {
        std::string signer_id;
        std::string cert;
        int last_update{0};
        int status{0};
    };
}

namespace gruut{
    class SignerCertPool
    {
    public:
        void push(SignerInfo &entity)
        {
            m_pool.emplace_back(entity);
        }

        void push(std::string &signer_id, std::string &cert, int last_update, int status)
        {
            SignerInfo temp;
            temp.signer_id = signer_id;
            temp.cert = cert;
            temp.last_update = last_update;
            temp.status = status;
            m_pool.emplace_back(temp);
        }

        std::string getCert(std::string signer_id)
        {
            return getCertCore(signer_id);
        }

        std::string getCert(std::string &signer_id)
        {
            return getCertCore(signer_id);
        }

    private:
        std::vector<SignerInfo> m_pool;

        std::string getCertCore(std::string &signer_id)
        {
            std::string ret_cert;
            for(size_t i = 0; i < m_pool.size(); ++i)
            {
                if(m_pool[i].signer_id == signer_id)
                {
                    ret_cert = m_pool[i].cert;
                    break;
                }
            }
            return ret_cert;
        }
    };
}