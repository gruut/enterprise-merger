#include "storage.hpp"
#include <iostream>

namespace gruut {
    using namespace std;
    using namespace nlohmann;

    Storage::Storage() : m_db(nullptr) {
        m_options.create_if_missing = true;
    }

    Storage::~Storage() {
        delete m_db;
        m_db = nullptr;
    }

    void Storage::openDB(const string &path) {
        m_path = path;
        auto status = leveldb::DB::Open(m_options, m_path, &m_db);
        if (!status.ok())
            cout << "DB 열기 실패 : " << path << endl;
        else
            cout << "DB 열기 성공 : " << path << endl;
    }

    void Storage::destroyDB() {
        auto status = leveldb::DestroyDB(m_path, m_options);
        if (!status.ok())
            cout << "DB 삭제 실패 : " << m_path << endl;
        else
            cout << "DB 삭제 성공 : " << m_path << endl;
    }

    bool Storage::write(json &block, const string &what, string blk_id) {
        string key;
        string new_key;
        string new_val;
        bool result = true;
        if (what == "block") {
            key = block["bID"];
            for (json::iterator it = block.begin(); it != block.end(); ++it) {
                if (it.key() == "bID") continue;
                if (it.key() == "sSig") {
                    for (unsigned idx = 0; idx < block["sSig"].size(); ++idx) {
                        new_key = key + '_' + it.key() + '_' + "sID" + '_' + to_string(idx);
                        new_val = block["sSig"][idx]["sID"];
                        m_writeBatch.Put(new_key, new_val);
                        new_key = key + '_' + it.key() + '_' + "sig" + '_' + to_string(idx);
                        new_val = block["sSig"][idx]["sig"];
                        m_writeBatch.Put(new_key, new_val);
                    }
                    new_key = key + '_' + it.key();
                    new_val = to_string(block["sSig"].size());
                    m_writeBatch.Put(new_key, new_val);
                } else {
                    new_key = key + '_' + it.key();
                    new_val = it.value().get<string>();
                    m_writeBatch.Put(new_key, new_val);
                }
                auto status = m_db->Write(leveldb::WriteOptions(), &m_writeBatch);
                if (!status.ok()) {
                    cout << "저장 실패" << endl;
                    result = false;
                    break;
                }
            }
        } else if (what == "tx") {
            unsigned idx;
            for (idx = 0; idx < block.size() - 1; ++idx) {
                block[idx]["bID"] = blk_id;
                //block[idx]["mPos"] = to_string(idx); //머클순서....
                string tx_id = block[idx]["txID"];
                string tx_list;
                for (json::iterator it = block[idx].begin(); it != block[idx].end(); ++it) {
                    if (it.key() == "txID") {
                        tx_list += block[idx]["txID"].dump();
                        tx_list += '_';
                        continue;
                    }
                    new_key = tx_id + '_' + it.key();
                    new_val = it.value();
                    m_writeBatch.Put(new_key, new_val);
                    auto status = m_db->Write(leveldb::WriteOptions(), &m_writeBatch);
                    if (!status.ok()) {
                        cout << "저장 실패" << endl;
                        result = false;
                    }
                }

                /*//머클트리 저장
                //new_key = 머클트리 저장,, 두개...
                new_key = blk_id + '_' + to_string(idx);
                //new_val = hash(tx_id);
                new_val = idx % 2 == 0 ? to_string(idx + 1) : to_string(idx - 1);

                m_writeBatch.Put(new_key, new_val);
                auto status = m_db->Write(leveldb::WriteOptions(), &m_writeBatch);
                if (!status.ok()) {
                    cout << "저장 실패" << endl;
                    result = false;
                }*/
            }

            // 머클트리..순서 반대로 저장.. (시블링 계산 쉽게하기 위해)
            // 해시가 7개면..
            // 1, h1234 / 2, h34 / 3. h12 / .... / 7. h1
            cout << "머클" << block[idx]["mtree"].size() << endl;
            for (unsigned int i = block[idx]["mtree"].size(); i > 0; --i) {
                new_key = blk_id + '_' + to_string(i);
                new_val = block[idx]["mtree"][block[idx]["mtree"].size() - i];
                m_writeBatch.Put(new_key, new_val);
                auto status = m_db->Write(leveldb::WriteOptions(), &m_writeBatch);
                if (!status.ok()) {
                    cout << "저장 실패" << endl;
                    result = false;
                }
            }
        } else if (what == "blkmeta") {
            for (json::iterator it = block.begin(); it != block.end(); ++it) {
                if (it.key() == "sSig") {
                    for (unsigned idx = 0; idx < block["sSig"].size(); ++idx) {
                        new_key = it.key() + '_' + "sID" + '_' + to_string(idx);
                        new_val = block["sSig"][idx]["sID"];
                        m_writeBatch.Put(new_key, new_val);
                        new_key = it.key() + '_' + "sig" + '_' + to_string(idx);
                        new_val = block["sSig"][idx]["sig"];
                        m_writeBatch.Put(new_key, new_val);
                    }
                    new_key = it.key();
                    new_val = to_string(block["sSig"].size());
                    m_writeBatch.Put(new_key, new_val);
                } else {
                    new_key = it.key();
                    new_val = it.value();
                    m_writeBatch.Put(new_key, new_val);
                }
                auto status = m_db->Write(leveldb::WriteOptions(), &m_writeBatch);
                if (!status.ok()) {
                    cout << "저장 실패" << endl;
                    result = false;
                    break;
                }
            }
        }
        return result;
    }

    bool Storage::write(const string &key, const string &value) {
        m_writeBatch.Put(key, value);
        auto status = m_db->Write(leveldb::WriteOptions(), &m_writeBatch);
        if (!status.ok()) {
            cout << "저장 실패" << endl;
            return false;
        }
        return true;
    }

    json Storage::find(const string &key, const string &field) {
        string str;
        leveldb::Status status;
        if (field == "sSig") {
            status = m_db->Get(leveldb::ReadOptions(), key + '_' + field, &str);
            multimap<string, string> map;

            if (str == "")
                return json({});

            for (int idx = 0; idx < stoi("1"); ++idx) {
                string str1, str2;
                status = m_db->Get(leveldb::ReadOptions(), key + '_' + field + '_' + "sID" + '_' + to_string(idx),
                                   &str1);
                status = m_db->Get(leveldb::ReadOptions(), key + '_' + field + '_' + "sig" + '_' + to_string(idx),
                                   &str2);
                map.insert(pair<string, string>(str1, str2));
            }
            if (status.ok()) {
                cout << key << "의 " << field << " 정보 읽어오기 성공" << endl;
                return json(map);
            } else {
                cout << key << "의 " << field << " 정보 읽어오기 실패" << endl;
                return json({});
            }
        } else {
            status = m_db->Get(leveldb::ReadOptions(), key + '_' + field, &str);
            if (status.ok()) {
                cout << key << "의 " << field << " 정보 읽어오기 성공" << endl;
                return json(str);
            } else {
                cout << key << "의 " << field << " 정보 읽어오기 실패" << endl;
                return json({});
            }
        }
    }

    json Storage::find(const string &key) {
        string ret;
        leveldb::Status status;
        if (key == "sSig") {
            status = m_db->Get(leveldb::ReadOptions(), key, &ret);
            multimap<string, string> map;
            for (int idx = 0; idx < stoi(ret); ++idx) {
                string str1, str2;
                status = m_db->Get(leveldb::ReadOptions(), key + '_' + "sID" + '_' + to_string(idx),
                                   &str1);
                status = m_db->Get(leveldb::ReadOptions(), key + '_' + "sig" + '_' + to_string(idx),
                                   &str2);
                map.insert(pair<string, string>(str1, str2));
            }
            if (status.ok()) {
                cout << key << " 정보 읽어오기 성공" << endl;
                return json(map);
            } else {
                cout << key << " 정보 읽어오기 실패" << endl;
                return json({});
            }
        } else {
            status = m_db->Get(leveldb::ReadOptions(), key, &ret);
            if (!status.ok()) {
                cout << key << "의 정보 읽어오기 실패" << endl;
                return json({});
            }
            cout << key << "의 정보 읽어오기 성공" << endl;
        }
        return json(ret);
    }

    // tx의 위치 : 1~4096
    int Storage::findTxIdPos(const string &blk_id, const string &tx_id) {
        cout << endl;
        cout << "블록ID " << blk_id << "의 TxID " << tx_id << "는 몇번째?" << endl;
        string str;
        if (m_path == "./db/secure") {
            m_path = "./db/blk";
        }
        auto status = m_db->Get(leveldb::ReadOptions(), blk_id + '_' + "txList", &str);
        if (!status.ok()) {
            cout << "찾기 실패" << endl;
        }
        int res = 0;
        int pos = 0;
        std::string token;
        while ((pos = str.find('_')) != string::npos) {
            token = str.substr(0, pos);
            if (token == tx_id) {
                break;
            }
            str.erase(0, pos + 1);
            res++;
        }
        return res + 1;
    }

    void Storage::findSilbing(const string &tx_id) {
        cout << "===시블링 찾기===" << endl;
        string blk_id = find(tx_id, "bID").get<string>();
        int tx_pos = findTxIdPos(blk_id, tx_id);
        //string tx_cnt = find(blk_id, "txCnt").get<string>();

        /*int num = stoi(tx_cnt);
        cout<<"머지"<<num<<endl;
        int sum=0;
        while(num>0){
            sum+=num;
            num/=2;
        }

        cout<<"ㄱㄱㄱㄱ"<<sum<<endl;*/
    }
    //int Storage::findMerkleNum(const string& blk, ){

}