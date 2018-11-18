#ifndef GRUUT_HANA_MERGER_MESSAGE_FETCHER_HPP
#define GRUUT_HANA_MERGER_MESSAGE_FETCHER_HPP

namespace gruut {
    class MessageFetcher {
    public:
        template <typename MessageType>
        static MessageType fetch() {
            // TODO: MessageType에 따라 다르게 리턴시키도록 구현해야함
            MessageType temp;

            do_nothing();

            return temp;
        }

    private:
        static void do_nothing() {}
    };
}
#endif
