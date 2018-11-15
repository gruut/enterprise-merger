//
// Created by ms on 2018-11-12.
//

#pragma once

#include "application.hpp"
#include <iostream>
#include <queue>

namespace gruut{
    struct Transaction{
        string u_id;
        string time;
        string txid;
        string type;
        string content;
        string u_sig;
    };

    class Txcollector{
    public:
        void setTimer(){

        }

        void push(Transaction &entity) {
            m_tx_collect.emplace(entity);
        }

        void push(string &u_id, string &time, string &txid, string &type, string &content, string &u_sig){
            m_tx_collect.emplace(u_id, time, txid, type, content, u_sig);
        }

        void getTxInfo(){

        }

    private:
        std::queue<Transaction> m_tx_collect;
    };
}