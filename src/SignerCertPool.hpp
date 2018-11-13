//
// Created by ms on 2018-11-12.
//

#pragma once

#include <iostream>
#include <vector>
#include "SignerPoolManager.hpp"

namespace gruut {
    struct SignerInfo {
        std::string signer_id;
        std::string cert;
        int last_update{0};
        int status{0};
    };
}

namespace gruut{
    class SignerCertPool : public SignerPoolManager
    {
    public:
        void push(SignerInfo &entity) {
            m_pool.emplace(entity);
        }

        void push(std::string &signer_id, std::string &cert, int last_update, int status) {
            SignerInfo temp;
            temp.signer_id = signer_id;
            temp.cert = cert;
            temp.last_update = last_update;
            temp.status = status;
            m_pool.emplace(temp);
        }

        void getSignerInfo(){
            auto signer_info = gruut::Application::app().getSignerInfo();
            if(!signer_info->empty()){
                m_pool.emplace(signer_info->front());
                signer_info->pop();
            }
        }

    private:
        std::queue<SignerInfo> m_pool;
    };
}