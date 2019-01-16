#ifndef GRUUT_ENTERPRISE_MERGER_SAFE_HPP
#define GRUUT_ENTERPRISE_MERGER_SAFE_HPP

#include "../utils/type_converter.hpp"
#include "nlohmann/json.hpp"
#include "type_converter.hpp"

class Safe {
public:
  static nlohmann::json parseJson(std::string &&json_str) {
    return parseJson(json_str);
  }

  static nlohmann::json parseJson(const std::string &json_str) {
    nlohmann::json ret_json;

    if (!json_str.empty()) {

      try {
        ret_json = nlohmann::json::parse(json_str);
      } catch (...) {
        /* do nothing */
      }
    }

    return ret_json;
  }

  static nlohmann::json parseJsonAsArray(const std::string &&json_str) {
    return parseJsonAsArray(json_str);
  }

  static nlohmann::json parseJsonAsArray(const std::string &json_str) {
    nlohmann::json ret_json = parseJson(json_str);

    if (!ret_json.is_array())
      ret_json = nlohmann::json::array();

    return ret_json;
  }

  static std::string getString(nlohmann::json &&json_obj,
                               const std::string &key) {
    return getString(json_obj, key);
  }

  static std::string getString(nlohmann::json &json_obj,
                               const std::string &key) {
    if (json_obj.find(key) != json_obj.end()) {
      return getString(json_obj[key]);
    }
    return std::string();
  }

  static std::string getString(nlohmann::json &&json_obj) {
    return getString(json_obj);
  }

  static std::string getString(nlohmann::json &json_obj) {
    std::string ret_str;
    if (!json_obj.empty()) {
      try {
        ret_str = json_obj.get<std::string>();
      } catch (...) {
        /* do nothing */
      }
    }

    return ret_str;
  }

  static size_t getSize(const std::string &size_str) {
    if (size_str.empty())
      return 0;

    return static_cast<uint64_t>(stoll(size_str));
  }

  template <typename T=nlohmann::json, typename K=std::string>
  static uint64_t getInt(T&& json_obj, K&& key) {
    std::string dec_str = getString(json_obj, key);
    if (dec_str.empty())
      return 0;
    return static_cast<uint64_t>(stoll(dec_str));
  }

  template <typename T=nlohmann::json, typename K=std::string>
  static gruut::timestamp_type getTime(T&& json_obj, K&& key) {
    return getTime(getString(json_obj, key));
  }

  template <typename T=std::string>
  static gruut::timestamp_type getTime(T&& dec_str) {
    if(dec_str.empty()) {
      return 0;
    }
    return static_cast<gruut::timestamp_type>(stoll(dec_str));
  }

  static bool getBoolean(nlohmann::json &&json_obj, const std::string &key) {
    return getBoolean(json_obj, key);
  }

  static bool getBoolean(nlohmann::json &json_obj, const std::string &key) {
    if (json_obj.find(key) != json_obj.end()) {
      if (!json_obj[key].is_boolean()) {
        return false;
      }

      return json_obj[key].get<bool>();
    }

    return false;
  }

  template <typename T = gruut::bytes>
  static T getBytesFromB64(nlohmann::json &&json_obj, const std::string &key) {
    return getBytesFromB64<T>(json_obj, key);
  }

  template <typename T = gruut::bytes>
  static T getBytesFromB64(nlohmann::json &json_obj, const std::string &key) {
    std::string parse_str = getString(json_obj, key);
    if (parse_str.empty())
      return T();
    return static_cast<T>(TypeConverter::decodeBase64(parse_str));
  }
};

#endif