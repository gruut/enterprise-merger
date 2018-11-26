#include "storage.hpp"
#include <iostream>

namespace gruut {
    using namespace std;
    using namespace nlohmann;

    Storage::Storage() {
        m_options.create_if_missing = true;
        m_write_options.sync = true;
    }

    Storage::~Storage() {
        delete m_db;
        m_db = nullptr;
    }

    void Storage::openDB(const string &path) {
        m_path = path;
        auto status = leveldb::DB::Open(m_options, m_path, &m_db);
        handleCriticalError(status);
        cout << "DB 열기 성공 : " << path << endl;
    }

    void Storage::write(json &data, const string &what, string &blk_id) {
        string key;

        if (what == "block") {
            key = data["bID"];
            for (auto it = data.cbegin(); it != data.cend(); ++it) {
                if (it.key() == "bID")
                    continue;

                auto new_key = "block_" + key + "_" + it.key();
                string new_value;

                if (it.value().is_array()) {
                    new_value = it.value().dump();
                } else {
                    new_value = it.value().get<string>();
                }

                auto status = m_db->Put(m_write_options, new_key, new_value);

                handleTrivialError(status);
            }
        } else if (what == "transactions") {
            unsigned int transaction_iterator_index;
            for (transaction_iterator_index = 0;
                 transaction_iterator_index < data.size() - 1;
                 ++transaction_iterator_index) {
                data[transaction_iterator_index]["bID"] = blk_id;

                string transaction_id = data[transaction_iterator_index]["txID"];
                for (auto it = data[transaction_iterator_index].cbegin();
                     it != data[transaction_iterator_index].cend(); ++it) {

                    auto new_key = "transactions_" + transaction_id + '_' + it.key();
                    string new_value;

                    if (it.value().is_object()) {
                      new_value = it.value().dump();
                    } else {
                      new_value = it.value().get<string>();
                    }
                    auto status =
                            m_db->Put(m_write_options, new_key, new_value);

                    handleTrivialError(status);
                }
            }
        } else if (what == "latest_block") {
            string value = data.dump();
            auto status = m_db->Put(m_write_options, "latest_block", value);

            handleTrivialError(status);
        } else if (what == "cert") {
            auto key = "cert_" + blk_id;
            auto value = data["x.509"].get<string>();
            auto status = m_db->Put(m_write_options, key, value);

            handleTrivialError(status);
        } else if (what == "block_header_hash") {
            auto key = "block_header_hash_" + blk_id;
            auto value = data["block_header_hash"].get<string>();
            auto status = m_db->Put(m_write_options, key, value);

            handleTrivialError(status);
        }
    }

    json Storage::findBy(const string &what, const string &id = "",
                         const string &field = "") {
        string key, value;
        leveldb::Status status;

        // If id is empty string, block_meta
        if (id == "") {
            key = "latest_block";
        } else {
            key = field == "" ? what + '_' + id : what + '_' + id + '_' + field;
        }

        status = m_db->Get(m_read_options, key, &value);
        if (!status.ok())
            return json({});

        return json(value);
    }

    void Storage::handleCriticalError(const leveldb::Status &status) {
        if (status.ok())
            return;
        const std::string errmsg = "Fatal LevelDB error: " + status.ToString();
        // TOOD: Logger
        cout << "You can use -debug=leveldb to get more complete diagnostic messages"
             << endl;
        throw;
    }

    void Storage::handleTrivialError(const leveldb::Status &status) {
        if (!status.ok()) {
            // TODO : logger
            cout << "저장 실패" << endl;
            return;
        }
    }
} // namespace gruut