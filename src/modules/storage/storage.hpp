#ifndef GRUUT_ENTERPRISE_MERGER_STORAGE_HPP
#define GRUUT_ENTERPRISE_MERGER_STORAGE_HPP

#include <map>

#include "leveldb/db.h"
#include "leveldb/options.h"
#include "leveldb/write_batch.h"
#include "nlohmann/json.hpp"

namespace gruut {
    using namespace std;
    using namespace nlohmann;

    class Storage {
    public:
        Storage();

        ~Storage();

        void openDB(const string &path);

        void destroyDB();

        bool write(json &val, const string &what, string blk_id = "");

        bool write(const string &key, const string &value);

        json find(const string &key, const string &field);

        json find(const string &key);

        int findTxIdPos(const string &blk_id, const string &tx_id);

        void findSilbing(const string &tx_id);

    private:
        string m_path;
        leveldb::Options m_options;
        leveldb::DB *m_db;
        leveldb::WriteBatch m_writeBatch;
        vector<string> silbing;
    };
}
#endif
