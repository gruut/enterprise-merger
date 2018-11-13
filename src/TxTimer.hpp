//
// Created by ms on 2018-11-13.
//

#pragma once

#include <iostream>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace gruut {
    class Txtimer {
    public:
        Txtimer(boost::asio::deadline_timer &timer) : t(timer) {
            wait();
        }

        void timeout(const boost::system::error_code &e) {
            if (e)
                return;
            std::cout << "tick" << std::endl;
            wait();
        }

        void cancel() {
            t.cancel();
        }

    private:
        void wait() {
            t.expires_from_now(boost::posix_time::seconds(5));
            t.async_wait(boost::bind(&Txtimer::timeout, this, boost::asio::placeholders::error));
        }

        boost::asio::deadline_timer &t;
    };

    class CancelTxTimer {
    public:
        CancelTxTimer(Txtimer &d) : dl(d) {}

        void operator()() {
            std::string cancel;
            std::cin >> cancel;
            dl.cancel();
            return;
        }

    private:
        Txtimer &dl;
    };
}