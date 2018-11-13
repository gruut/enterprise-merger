//
// Created by ms on 2018-11-12.
//

#pragma once

#include "application.hpp"
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
        void push(SignerInfoManager &entity) {
            m_pool_manager.emplace(entity);
        }

        void push(std::string &signer_id, std::string &cert, int last_update, int status){
            SignerInfoManager temp;
            temp.signer_id = signer_id;
            temp.cert = cert;
            temp.last_update = last_update;
            temp.status = status;
            m_pool_manager.emplace(temp);
        }

        void getSingerInfo(){
            auto signer_info = gruut::Application::app().getSignerInfo();
            if(!signer_info->empty()){
                m_pool_manager.emplace(signer_info->front());
                signer_info->pop();
            }
        }
    private:
        std::queue<SignerInfoManager> m_pool_manager;
    };
}