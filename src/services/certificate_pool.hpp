#ifndef GRUUT_ENTERPRISE_MERGER_CERTIFICATE_POOL_HPP
#define GRUUT_ENTERPRISE_MERGER_CERTIFICATE_POOL_HPP

#include "../utils/template_singleton.hpp"
#include "../utils/type_converter.hpp"
#include "../chain/types.hpp"

#include <map>

namespace gruut {

class CertificatePool : public TemplateSingleton<CertificatePool> {
private:
  std::map<std::string, std::string> m_cert_map;
  std::mutex m_push_mutex;
public:
  template<typename T=id_type>
  std::string getCert(T &&t_id) {
    std::string ret_cert;
    std::string id_b64 = TypeConverter::encodeBase64(t_id);
    auto it_map = m_cert_map.find(id_b64);
    if(it_map != m_cert_map.end())
      ret_cert = it_map->second;

    return ret_cert;
  }

  template<typename T=id_type, typename S=std::string>
  void pushCert(T &&t_id, S &&cert) {
    std::lock_guard<std::mutex> guard(m_push_mutex);
    auto insert_result = m_cert_map.insert({TypeConverter::encodeBase64(t_id),cert});
    if(!insert_result.second)
      insert_result.frist->second = cert; // update
    m_push_mutex.unlock();
  }

};

}

#endif //GRUUT_ENTERPRISE_MERGER_CERTIFICATE_POOL_HPP
