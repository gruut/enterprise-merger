#ifndef GRUUT_ENTERPRISE_MERGER_MANAGE_CONNECTION_HPP
#define GRUUT_ENTERPRISE_MERGER_MANAGE_CONNECTION_HPP

#include "../../services/setting.hpp"
#include "../../utils/template_singleton.hpp"
#include "../../utils/type_converter.hpp"
#include <algorithm>
#include <atomic>
#include <map>
#include <mutex>
#include <thread>

namespace gruut {

class ConnManager : public TemplateSingleton<ConnManager> {
public:
  ConnManager() = default;

  void setMergerInfo(MergerInfo &merger_info, bool conn_status = false) {
    std::string merger_id_b64 = TypeConverter::encodeBase64(merger_info.id);
    std::lock_guard<std::mutex> lock(m_merger_mutex);
    m_merger_info[merger_id_b64] = merger_info;
    m_merger_info[merger_id_b64].conn_status = conn_status;
    m_merger_mutex.unlock();
  }

  void setSeInfo(ServiceEndpointInfo &se_info, bool conn_status = false) {
    std::string se_id_b64 = TypeConverter::encodeBase64(se_info.id);
    std::lock_guard<std::mutex> lock(m_se_mutex);
    m_se_info[se_id_b64] = se_info;
    m_se_info[se_id_b64].conn_status = conn_status;
    m_se_mutex.unlock();
  }

  void setMergerStatus(merger_id_type &merger_id, bool status) {
    if (!m_enabled_merger_check)
      return;

    std::string merger_id_b64 = TypeConverter::encodeBase64(merger_id);
    std::lock_guard<std::mutex> lock(m_merger_mutex);
    m_merger_info[merger_id_b64].conn_status = status;
    m_merger_mutex.unlock();
  }

  void setSeStatus(servend_id_type &se_id, bool status) {
    if (!m_enabled_se_check)
      return;

    std::string se_id_b64 = TypeConverter::encodeBase64(se_id);
    std::lock_guard<std::mutex> lock(m_se_mutex);
    m_se_info[se_id_b64].conn_status = status;
    m_se_mutex.unlock();
  }

  void setMergerBlockHgt(merger_id_type &merger_id, uint64_t block_height) {
    m_init_block_hgt_list.push_back({block_height, merger_id});
  }

  MergerInfo getMergerInfo(merger_id_type &merger_id) {
    string merger_id_b64 = TypeConverter::encodeBase64(merger_id);

    return m_merger_info[merger_id_b64];
  }

  ServiceEndpointInfo getSeInfo(servend_id_type &se_id) {
    string se_id_64 = TypeConverter::encodeBase64(se_id);

    return m_se_info[se_id_64];
  }

  std::vector<MergerInfo> getAllMergerInfo() {
    std::vector<MergerInfo> merger_list;
    std::transform(
        m_merger_info.begin(), m_merger_info.end(), back_inserter(merger_list),
        [](std::pair<string, MergerInfo> const &p) { return p.second; });
    return merger_list;
  }

  std::vector<ServiceEndpointInfo> getAllSeInfo() {
    std::vector<ServiceEndpointInfo> se_list;
    std::transform(m_se_info.begin(), m_se_info.end(), back_inserter(se_list),
                   [](std::pair<string, ServiceEndpointInfo> const &p) {
                     return p.second;
                   });
    return se_list;
  }

  std::vector<pair<uint64_t, merger_id_type>> getMaxBlockHgtMergers() {
    std::vector<pair<uint64_t, merger_id_type>> merger_list;
    auto max_height = std::max_element(m_init_block_hgt_list.begin(),
                                       m_init_block_hgt_list.end());

    for (auto &info : m_init_block_hgt_list) {
      if (info.first == max_height->first) {
        merger_list.emplace_back(info);
      }
    }

    return merger_list;
  }
  bool checkMergerInfo(merger_id_type &merger_id) {
    std::string merger_id_b64 = TypeConverter::encodeBase64(merger_id);
    if(m_merger_info.find(merger_id_b64) == m_merger_info.end())
      return false;

    return true;
  }
  bool getMergerStatus(merger_id_type &merger_id) {
    if (!m_enabled_merger_check)
      return true;

    std::string merger_id_b64 = TypeConverter::encodeBase64(merger_id);
    return m_merger_info[merger_id_b64].conn_status;
  }

  bool getSeStatus(servend_id_type &se_id) {
    if (!m_enabled_se_check)
      return true;

    std::string se_id_b64 = TypeConverter::encodeBase64(se_id);
    return m_se_info[se_id_b64].conn_status;
  }

  void disableMergerCheck() { m_enabled_merger_check = false; }

  void disableSECheck() { m_enabled_se_check = false; }

  void clearBlockHgtList() { m_init_block_hgt_list.clear(); }

private:
  std::map<string, MergerInfo> m_merger_info;
  std::map<string, ServiceEndpointInfo> m_se_info;
  std::vector<pair<uint64_t, merger_id_type>> m_init_block_hgt_list;
  std::mutex m_merger_mutex;
  std::mutex m_se_mutex;

  std::atomic<bool> m_enabled_merger_check{true};
  std::atomic<bool> m_enabled_se_check{true};
};

} // namespace gruut

#endif // GRUUT_ENTERPRISE_MERGER_MANAGE_CONNECTION_HPP