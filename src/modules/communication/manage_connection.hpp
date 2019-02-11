#ifndef GRUUT_ENTERPRISE_MERGER_MANAGE_CONNECTION_HPP
#define GRUUT_ENTERPRISE_MERGER_MANAGE_CONNECTION_HPP

#include "../../services/setting.hpp"
#include "../../utils/template_singleton.hpp"
#include "../../utils/type_converter.hpp"

#include <algorithm>
#include <atomic>
#include <boost/filesystem.hpp>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>

using namespace boost::filesystem;

namespace gruut {

class ConnManager : public TemplateSingleton<ConnManager> {
public:
  ConnManager() {
    m_setting = Setting::getInstance();
    m_merger_info_path =
        m_setting->getMyDbPath() + "/" + config::DB_SUB_DIR_MERGER_INFO;
  }

  void setTrackerInfo(TrackerInfo &tracker_info, bool conn_status = false) {
    std::string tk_id_b64 = TypeConverter::encodeBase64(tracker_info.id);
    std::lock_guard<std::mutex> lock(m_tk_mutex);
    m_tracker_info = tracker_info;
    m_tracker_info.conn_status = conn_status;
    m_tk_mutex.unlock();
  }

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

  void setTrackerStatus(bool status) {
    std::lock_guard<std::mutex> lock(m_tk_mutex);
    m_tracker_info.conn_status = status;
    m_tk_mutex.unlock();
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

  void setMultiMergerInfo(json &merger_list) {

    auto cert_pool = CertificatePool::getInstance();

    std::string my_id_b64 = TypeConverter::encodeBase64(m_setting->getMyId());
    for (auto &merger : merger_list) {

      std::string merger_id_b64 = Safe::getString(merger, "mID");
      merger_id_type merger_id = TypeConverter::decodeBase64(merger_id_b64);

      if (merger_id_b64 == my_id_b64)
        continue;

      MergerInfo merger_info;
      merger_info.id = merger_id;
      merger_info.address = Safe::getString(merger, "ip");
      merger_info.port = Safe::getString(merger, "port");
      merger_info.cert = Safe::getString(merger, "mCert");

      cert_pool->pushCert(merger_id, merger_info.cert);
      setMergerInfo(merger_info, true);

      if (merger.find("merger") != merger.end()) {
        auto block_height = Safe::getInt(merger, "hgt");
        setMergerBlockHgt(merger_id, block_height);
      }
    }
  }

  void setMultiSeInfo(json &se_list) {

    auto cert_pool = CertificatePool::getInstance();

    for (auto &se : se_list) {
      ServiceEndpointInfo se_info;
      string se_id_b64 = Safe::getString(se, "seID");
      servend_id_type se_id = TypeConverter::decodeBase64(se_id_b64);

      if (hasSeInfo(se_id))
        continue;

      se_info.id = se_id;
      se_info.address = Safe::getString(se, "ip");
      se_info.port = Safe::getString(se, "port");
      se_info.cert = Safe::getString(se, "seCert");

      cert_pool->pushCert(se_id, se_info.cert);
      setSeInfo(se_info, true);
    }
  }

  void setMergerBlockHgt(merger_id_type &merger_id, uint64_t block_height) {
    m_init_block_hgt_list.emplace_back(merger_id, block_height);
  }

  json getAllMergerInfoJson() {
    json merger_list = json::array();

    for (auto &merger : m_merger_info) {
      json merger_data;
      merger_data["mID"] = TypeConverter::encodeBase64(merger.second.id);
      merger_data["ip"] = merger.second.address;
      merger_data["port"] = merger.second.port;
      merger_data["mCert"] = merger.second.cert;

      merger_list.emplace_back(merger_data);
    }
    return merger_list;
  }

  json getAllSeInfoJson() {
    json se_list = json::array();

    for (auto &se : m_se_info) {
      json se_data;
      se_data["seID"] = TypeConverter::encodeBase64(se.second.id);
      se_data["ip"] = se.second.address;
      se_data["port"] = se.second.port;
      se_data["seCert"] = se.second.cert;

      se_list.emplace_back(se_data);
    }
    return se_list;
  }

  TrackerInfo getTrackerInfo() { return m_tracker_info; }

  MergerInfo getMergerInfo(merger_id_type &merger_id) {
    std::string merger_id_b64 = TypeConverter::encodeBase64(merger_id);

    return m_merger_info[merger_id_b64];
  }

  ServiceEndpointInfo getSeInfo(servend_id_type &se_id) {
    std::string se_id_64 = TypeConverter::encodeBase64(se_id);

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

  std::pair<block_height_type, std::vector<id_type>> getMaxBlockHgtMergers() {

    if (m_init_block_hgt_list.empty()) {
      return {0, {}};
    }

    std::vector<id_type> merger_list;

    auto max_height = std::max_element(
        m_init_block_hgt_list.begin(), m_init_block_hgt_list.end(),
        [this](merger_height_type &a, merger_height_type &b) {
          return (a.height < b.height);
        });

    for (auto &info : m_init_block_hgt_list) {
      if (info.height == max_height->height) {
        merger_list.emplace_back(info.merger_id);
      }
    }

    return {max_height->height, merger_list};
  }
  bool hasMergerInfo(merger_id_type &merger_id) {
    std::string merger_id_b64 = TypeConverter::encodeBase64(merger_id);
    return m_merger_info.find(merger_id_b64) != m_merger_info.end();
  }

  bool hasSeInfo(servend_id_type &se_id) {
    std::string se_id_b64 = TypeConverter::encodeBase64(se_id);
    return m_se_info.find(se_id_b64) != m_se_info.end();
  }

  bool getTrackerStatus() {
    if (!m_enabled_tracker)
      return false;

    return m_tracker_info.conn_status;
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
  void disableTracker() { m_enabled_tracker = false; }

  void disableMergerCheck() { m_enabled_merger_check = false; }

  void disableSECheck() { m_enabled_se_check = false; }

  void clearBlockHgtList() { m_init_block_hgt_list.clear(); }

  void readMergerInfo() {

    if (!exists(m_merger_info_path))
      return;

    json merger_list = json::array();
    for (const auto &entry : directory_iterator(m_merger_info_path)) {
      std::ifstream ifs(entry.path().string());
      json merger_info = json::parse(ifs);

      merger_list.emplace_back(merger_info);
    }
    setMultiMergerInfo(merger_list);
  }

  void writeMergerInfo(json &merger_info) {

    if (!exists(m_merger_info_path))
      create_directories(m_merger_info_path);

    std::string merger_id_b64 = Safe::getString(merger_info, "mID");
    std::string file_name = m_merger_info_path + "/" + merger_id_b64 + ".json";

    std::ofstream ofs(file_name);
    ofs << std::setw(4) << merger_info << std::endl;
  }

private:
  Setting *m_setting;
  std::string m_merger_info_path;

  TrackerInfo m_tracker_info;
  std::map<string, MergerInfo> m_merger_info;
  std::map<string, ServiceEndpointInfo> m_se_info;
  std::vector<merger_height_type> m_init_block_hgt_list;
  std::mutex m_merger_mutex;
  std::mutex m_se_mutex;
  std::mutex m_tk_mutex;

  std::atomic<bool> m_enabled_tracker{true};
  std::atomic<bool> m_enabled_merger_check{true};
  std::atomic<bool> m_enabled_se_check{true};
};

} // namespace gruut

#endif // GRUUT_ENTERPRISE_MERGER_MANAGE_CONNECTION_HPP