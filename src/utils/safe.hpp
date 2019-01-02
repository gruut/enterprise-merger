#pragma once

#include "nlohmann/json.hpp"

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

  static std::string getString(nlohmann::json &&json_obj, const std::string &key) {
    return getString(json_obj,key);
  }

  static std::string getString(nlohmann::json &json_obj, const std::string &key) {
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
};