#ifndef GRUUT_HANA_MERGER_STORAGE_HPP
#define GRUUT_HANA_MERGER_STORAGE_HPP

#include <vector>
#include <iostream>

#include "leveldb/write_batch.h"
#include "leveldb/options.h"
#include "leveldb/db.h"
#include "../include/nlohmann/json.hpp"

namespace gruut {
    using namespace std;
    using namespace nlohmann;

    class Storage {
    public:
        Storage() {
            string path = "./hi";
            auto status = leveldb::DB::Open(m_options, path, &m_db);
            if (status.ok())
                cout << "DB open" << endl;
        }

        ~Storage() {
            delete m_db;
            m_db = nullptr;
        }

        json read(const string &key) {
            string data;
            auto status = m_db->Get(m_readOptions, key, &data);

            if (status.ok())
                cout << "read " << key << "성공" << endl;

            return data;
        }

        json readAll() {
            multimap<string, string> map;
            unique_ptr<leveldb::Iterator> it(m_db->NewIterator(m_readOptions));
            for (it->SeekToFirst(); it->Valid(); it->Next())
                map.insert({it->key().ToString(), it->value().ToString()});
            if (it->status().ok())
                cout << "read 성공" << std::endl;

            return json(map);
        }

    private:
        leveldb::Options m_options;
        leveldb::DB *m_db;
        leveldb::ReadOptions m_readOptions;
    };
}
#endif
