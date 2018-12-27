#pragma once

#include "../../utils/type_converter.hpp"
#include "../../utils/template_singleton.hpp"
#include "../../services/setting.hpp"
#include <map>
#include <mutex>
#include <thread>

namespace gruut{

class ConnectionList : public TemplateSingleton<ConnectionList> {
public:
  ConnectionList(){
	Setting *setting = Setting::getInstance();
	merger_id_type  my_id = setting->getMyId();
	auto merger_info_list = setting->getMergerInfo();
	auto se_info_list = setting->getServiceEndpointInfo();

	for(auto &merger_info : merger_info_list){
	  if(my_id == merger_info.id)
	    continue;
	  std::string merger_id_b64 = TypeConverter::toBase64Str(merger_info.id);
	  std::string address = merger_info.address + ":" + merger_info.port;
	  m_merger_list[merger_id_b64] = false;
	}

	for(auto &se_info : se_info_list){
	  std::string se_id_b64 = TypeConverter::toBase64Str(se_info.id);
	  std::string address = se_info.address + ":" + se_info.port;
	  m_se_list[se_id_b64] = false ;
	}
  }

  void setMergerStatus(merger_id_type &merger_id, bool status){
	std::string merger_id_b64 = TypeConverter::toBase64Str(merger_id);
	std::lock_guard<std::mutex> lock(m_merger_mutex);
	m_merger_list[merger_id_b64] = status;
	m_merger_mutex.unlock();
  }

  void setSeStatus(servend_id_type &se_id, bool status){
	std::string se_id_b64 = TypeConverter::toBase64Str(se_id);
	std::lock_guard<std::mutex> lock(m_se_mutex);
	m_se_list[se_id_b64] = status;
	m_se_mutex.unlock();
  }

  bool getMergerStatus(merger_id_type &merger_id){
	std::string merger_id_b64 = TypeConverter::toBase64Str(merger_id);
	return m_merger_list[merger_id_b64];
  }

  bool getSeStatus(servend_id_type &se_id){
	std::string se_id_b64 = TypeConverter::toBase64Str(se_id);
	return  m_se_list[se_id_b64];
  }

private:
  std::map<std::string, bool> m_merger_list;
  std::map<std::string, bool> m_se_list;
  std::mutex m_merger_mutex;
  std::mutex m_se_mutex;

};

}