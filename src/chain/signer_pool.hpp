//
// Created by JeonilKang on 2018-11-26.
//

#ifndef GRUUT_ENTERPRISE_MERGER_SIGNER_POOL_HPP
#define GRUUT_ENTERPRISE_MERGER_SIGNER_POOL_HPP

#include <iostream>
#include <string>
#include <deque>
#include <array>
#include <ctime>

#include <thread>
#include <mutex>

#include <botan/secmem.h>

#include "../utils/template_singleton.hpp"


enum SSTATUS {
  UNKNOWN,
  GOOD,
  LINK_ERROR
};

typedef struct _SignerEntry {
  uint64_t user_id{0};
  std::string pk_cert;
  Botan::secure_vector<uint8_t> hmac_key;
  std::time_t last_update{0};
  int stat{SSTATUS::UNKNOWN};
} SignerEntry;

namespace gruut {

class SignerPool : public TemplateSingleton<SignerPool>{

private:
  std::deque<SignerEntry> m_signer_pool;
  std::mutex m_push_mutex;
  std::mutex m_update_mutex;
public:

  void pushSigner(uint64_t user_id, std::string &pk_cert, Botan::secure_vector<uint8_t> &hmac_key, int stat = SSTATUS::UNKNOWN) {

    SignerEntry tmp_entry;
    tmp_entry.user_id = user_id;
    tmp_entry.pk_cert = pk_cert;
    tmp_entry.hmac_key = hmac_key;
    tmp_entry.stat = stat;
    tmp_entry.last_update = std::time(nullptr);

    bool is_in = false;
    for(SignerEntry &entry : m_signer_pool) {
      if(entry.user_id == user_id) {
        is_in = true;

        std::lock_guard<std::mutex> guard(m_push_mutex);
        entry = tmp_entry;
        m_push_mutex.unlock();

        break;
      }
    }

    if(!is_in) {
      std::lock_guard<std::mutex> guard(m_push_mutex);
      m_signer_pool.emplace_back(tmp_entry);
      m_push_mutex.unlock();
    }

  }

  bool updatePkCert(uint64_t user_id, std::string &pk_cert){
    bool is_in = false;
    for(SignerEntry &entry : m_signer_pool) {
      if(entry.user_id == user_id) {
        is_in = true;

        std::lock_guard<std::mutex> guard(m_update_mutex);
        entry.pk_cert = pk_cert;
        entry.last_update = std::time(nullptr);
        m_update_mutex.unlock();

        break;
      }
    }

    return is_in;
  }

  bool updateHmacKey(uint64_t user_id, Botan::secure_vector<uint8_t> &hmac_key){
    bool is_in = false;
    for(SignerEntry &entry : m_signer_pool) {
      if(entry.user_id == user_id) {
        is_in = true;

        std::lock_guard<std::mutex> guard(m_update_mutex);
        entry.hmac_key = hmac_key;
        entry.last_update = std::time(nullptr);
        m_update_mutex.unlock();

        break;
      }
    }

    return is_in;
  }

  bool updateStatus(uint64_t user_id, int stat = SSTATUS::GOOD) {
    bool is_in = false;
    for(SignerEntry &entry : m_signer_pool) {
      if(entry.user_id == user_id) {
        is_in = true;

        std::lock_guard<std::mutex> guard(m_update_mutex);
        entry.stat = stat;
        entry.last_update = std::time(nullptr);
        m_update_mutex.unlock();

        break;
      }
    }

    return is_in;
  }

  bool removeSigner(uint64_t user_id) {
    bool is_in = false;
    for(size_t i = 0; i < m_signer_pool.size(); ++i) {
      if(m_signer_pool[i].user_id == user_id) {
        is_in = true;

        std::lock_guard<std::mutex> guard(m_push_mutex);
        m_signer_pool.erase(m_signer_pool.begin() + i);
        m_push_mutex.unlock();

        break;
      }
    }

    return is_in;
  }

  Botan::secure_vector<uint8_t> getHmacKey(uint64_t user_id) {
    Botan::secure_vector<uint8_t> ret_hmac_key;
    for(SignerEntry &entry : m_signer_pool) {
      if(entry.user_id == user_id) {
        ret_hmac_key = entry.hmac_key;
        break;
      }
    }

    return ret_hmac_key;
  }

  std::string getPkCert(uint64_t user_id) {
    std::string ret_pk_cert;

    for(SignerEntry &entry : m_signer_pool) {
      if(entry.user_id == user_id) {
        ret_pk_cert = entry.pk_cert;
        break;
      }
    }

    return ret_pk_cert;
  }

  size_t getNumSigner(int stat = SSTATUS::GOOD){

    if(stat < 0) {
      return m_signer_pool.size();
    }

    size_t num_good_signer = 0;
    for(SignerEntry &entry : m_signer_pool) {
      if(entry.stat == stat) {
        ++num_good_signer;
      }
    }

    return num_good_signer;
  }

  std::vector<uint64_t> getGoodSignerList(){
    std::vector<uint64_t> ret_list;
    for(SignerEntry &entry : m_signer_pool) {
      if(entry.stat == SSTATUS::GOOD) {
        ret_list.push_back(entry.user_id);
      }
    }

    return ret_list;
  }

  void emptyPool(){
    std::lock_guard<std::mutex> guard(m_push_mutex);
    m_signer_pool.clear();
    m_push_mutex.unlock();
  }

  // TODO: May be we should do more operation, but we cannot recognize what operations are required.

};

}

#endif //GRUUT_ENTERPRISE_MERGER_SIGNER_POOL_HPP
