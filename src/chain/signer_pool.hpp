//
// Created by JeonilKang on 2018-11-26.
//

#ifndef GRUUT_ENTERPRISE_MERGER_SIGNER_POOL_HPP
#define GRUUT_ENTERPRISE_MERGER_SIGNER_POOL_HPP

#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <ctime>

#include "../utils/template_singleton.hpp"

enum SSTATUS {
  UNKNOWN,
  GOOD,
  LINK_ERROR
};

struct SignerEntry{
  uint64_t user_id{0};
  std::string pk_cert;
  std::vector<uint8_t> hmac_key;
  std::time_t last_update{0};
  int stat{SSTATUS::UNKNOWN};
};

namespace gruut {

class SignerPool : TemplateSingleton<SignerPool>{

private:
  std::vector<SignerEntry> m_signer_pool;

public:

  void pushSigner(uint64_t user_id, std::string &pk_cert, std::vector<uint8_t> &hmac_key, int stat = SSTATUS::UNKNOWN) {

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
        entry = tmp_entry;
        break;
      }
    }

    if(!is_in)
      m_signer_pool.emplace_back(tmp_entry);

  }

  bool updatePkCert(uint64_t user_id, std::string &pk_cert){
    bool is_in = false;
    for(SignerEntry &entry : m_signer_pool) {
      if(entry.user_id == user_id) {
        is_in = true;
        entry.pk_cert = pk_cert;
        entry.last_update = std::time(nullptr);
        break;
      }
    }

    return is_in;
  }

  bool updateHmacKey(uint64_t user_id, std::vector<uint8_t> &hmac_key){
    bool is_in = false;
    for(SignerEntry &entry : m_signer_pool) {
      if(entry.user_id == user_id) {
        is_in = true;
        entry.hmac_key = hmac_key;
        entry.last_update = std::time(nullptr);
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
        entry.stat = stat;
        entry.last_update = std::time(nullptr);
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
        m_signer_pool.erase(m_signer_pool.begin() + i);
        break;
      }
    }

    return is_in;
  }

  std::vector<uint8_t> getHmacKey(uint64_t user_id) {
    std::vector<uint8_t> ret_hmac_key;
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

  // TODO: May be we should do more operation, but we cannot recognize what operations are required.

};

}

#endif //GRUUT_ENTERPRISE_MERGER_SIGNER_POOL_HPP
