//
// Created by ms on 2018-11-12.
//

#pragma once

#include "application.hpp"
#include <iostream>
#include <queue>

namespace gruut{
    struct Transaction{
        std::string u_id;
        std::string time;
        std::string txid;
        std::string type;
        std::string content;
        std::string u_sig;
    };
}

namespace gruut{

    class Txcollector{
    public:
        void setTimer(){

        }

        void push(Transaction &entity) {
            m_tx_collect.emplace(entity);
        }

        void push(std::string &u_id, std::string &time, std::string &txid,
                std::string &type, std::string &content, std::string &u_sig){
            Transaction temp;
            temp.u_id = u_id;
            temp.time = time;
            temp.txid = txid;
            temp.type = type;
            temp.content = content;
            temp.u_sig = u_sig;
            m_tx_collect.emplace(temp);
        }

        void getTxInfo(){
            auto block_info = gruut::Application::app().getStorageInfo();
            auto tx_info = gruut::Application::app().getTxInfo();
            if(!tx_info->empty()){
                m_tx_collect.emplace(tx_info->front());
                tx_info->pop();
            }
        }

    private:
        std::queue<Transaction> m_tx_collect;
    };
}