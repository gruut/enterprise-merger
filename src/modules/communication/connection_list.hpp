#ifndef GRUUT_ENTERPRISE_MERGER_CONNECTION_LIST_HPP
#define GRUUT_ENTERPRISE_MERGER_CONNECTION_LIST_HPP

#include "../../services/setting.hpp"
#include "../../utils/template_singleton.hpp"
#include "../../utils/type_converter.hpp"
#include <atomic>
#include <map>
#include <mutex>
#include <thread>

namespace gruut {

class ConnectionList : public TemplateSingleton<ConnectionList> {
public:
  ConnectionList() {
    Setting *setting = Setting::getInstance();
    merger_id_type my_id = setting->getMyId();
    auto merger_info_list = setting->getMergerInfo();
    auto se_info_list = setting->getServiceEndpointInfo();

    for (auto &merger_info : merger_info_list) {
      if (my_id == merger_info.id)
        continue;
      std::string merger_id_b64 = TypeConverter::encodeBase64(merger_info.id);
      m_merger_list[merger_id_b64] = false;
    }

    for (auto &se_info : se_info_list) {
      std::string se_id_b64 = TypeConverter::encodeBase64(se_info.id);
      m_se_list[se_id_b64] = false;
    }
  }

  void setMergerStatus(merger_id_type &merger_id, bool status) {
    if (!m_enabled_merger_check)
      return;

    std::string merger_id_b64 = TypeConverter::encodeBase64(merger_id);
    m_merger_list[merger_id_b64] = status;
  }

  void setSeStatus(servend_id_type &se_id, bool status) {
    if (!m_enabled_se_check)
      return;

    std::string se_id_b64 = TypeConverter::encodeBase64(se_id);
    m_se_list[se_id_b64] = status;
  }

  bool getMergerStatus(merger_id_type &merger_id) {
    if (!m_enabled_merger_check)
      return true;

    std::string merger_id_b64 = TypeConverter::encodeBase64(merger_id);
    return m_merger_list[merger_id_b64];
  }

  bool getSeStatus(servend_id_type &se_id) {
    if (!m_enabled_se_check)
      return true;

    std::string se_id_b64 = TypeConverter::encodeBase64(se_id);
    return m_se_list[se_id_b64];
  }

  void disableMergerCheck() { m_enabled_merger_check = false; }

  void disableSECheck() { m_enabled_se_check = false; }

private:
  std::map<std::string, std::atomic<bool>> m_merger_list;
  std::map<std::string, std::atomic<bool>> m_se_list;
  std::atomic<bool> m_enabled_merger_check{true};
  std::atomic<bool> m_enabled_se_check{false};
};

} // namespace gruut

#endif