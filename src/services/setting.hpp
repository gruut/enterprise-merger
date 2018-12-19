#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "../../include/json-schema.hpp"
#include "../../include/nlohmann/json.hpp"
#include "../chain/types.hpp"
#include "../utils/template_singleton.hpp"
#include "../utils/type_converter.hpp"

namespace gruut {
using namespace nlohmann;
using namespace nlohmann::json_schema_draft4;

struct GruutAuthorityInfo {
  id_type id;
  std::string address;
  std::string cert;
};

struct ServiceEndpointInfo {
  id_type id;
  std::string address;
  std::string port;
  std::string cert;
};

struct MergerInfo {
  id_type id;
  std::string address;
  std::string port;
  std::string cert;
};

const json SCHEMA_SETTING = R"(
{
  "title": "Setting",
  "description": "Self, LocalChain, GA, SE, MG의 기본 Setting",
  "type": "object",
  "properties": {
    "Self": {
      "description": "Self의 id, address, port, sk",
      "type":"object",
      "properties" : {
        "id" : {"type":"string"},
        "address" : {"type":"string"},
        "port" : {"type":"string"},
        "sk" : {"type":"array"}
      },
      "required": [
        "id",
        "address",
        "port",
        "sk"
      ]
    },
    "LocalChain": {
      "description": "LocalChain의 id, name",
      "type":"object",
      "properties" : {
        "id" : {"type":"string"},
        "name" : {"type":"string"}
      },
      "required": [
        "id",
        "name"
      ]
    },
    "GA": {
      "description": "Gruut Authority의 id, address, cert",
      "type":"object",
      "properties" : {
        "id" : {"type":"string"},
        "address" : {"type":"string"},
        "cert" : {"type":"array"}
      },
      "required": [
        "id",
        "address",
        "cert"
      ]
    },
    "SE": {
      "description": "Service Endpoint의 id, address, cert",
      "type":"array",
      "items":{
        "type":"object",
        "properties" : {
          "id" : {"type":"string"},
          "address" : {"type":"string"},
          "cert" : {"type":"array"}
        },
        "required": [
          "id",
          "address",
          "cert"
        ]
      }
    },
    "MG": {
      "description": "Merger의 id, address, port, cert",
      "type":"array",
      "items":{
        "type":"object",
        "properties" : {
          "id" : {"type":"string"},
          "address" : {"type":"string"},
          "port" : {"type":"string"},
          "cert" : {"type":"array"}
        },
        "required": [
          "id",
          "address",
          "port",
          "cert"
        ]
      }
    }
  },
  "required": [
    "Self",
    "LocalChain",
    "GA",
    "SE",
    "MG"
  ]
}
)"_json;

class Setting : public TemplateSingleton<Setting> {
private:
  id_type m_id;
  std::string m_sk;
  std::string m_sk_pass;
  std::string m_cert;
  std::string m_address;
  std::string m_port;
  local_chain_id_type m_localchain_id;
  std::string m_localchain_name;

  GruutAuthorityInfo m_gruut_authority;
  std::vector<ServiceEndpointInfo> m_service_endpoints;
  std::vector<MergerInfo> m_mergers;

public:
  Setting() {}

  Setting(json &setting_json) {
    if (!setJson(setting_json)) {
      std::cout << "error on parsing setting json" << std::endl;
    }
  }

  ~Setting() {
    // TODO :: wipe-out m_sk securely
  }

  bool setJson(json &setting_json) {

    if (!validateSchema(setting_json))
      return false;

    m_id = getIdFromJson(setting_json["Self"]["id"]);
    m_address = setting_json["Self"]["address"].get<std::string>();
    m_port = setting_json["Self"]["port"].get<std::string>();
    m_sk = joinMultiLine(setting_json["Self"]["sk"]);
    m_localchain_id = getChainIdFromJson(setting_json["LocalChain"]["id"]);
    m_localchain_name = setting_json["LocalChain"]["name"].get<std::string>();

    m_gruut_authority.id = getIdFromJson(setting_json["GA"]["id"]);
    m_gruut_authority.address =
        setting_json["GA"]["address"].get<std::string>();
    m_gruut_authority.cert = joinMultiLine(setting_json["cert"]);

    m_service_endpoints.clear();
    for (size_t i = 0; i < setting_json["SE"].size(); ++i) {
      ServiceEndpointInfo tmp_info;
      tmp_info.id = getIdFromJson(setting_json["SE"][i]["id"]);
      tmp_info.address = setting_json["SE"][i]["address"].get<std::string>();
      tmp_info.cert = joinMultiLine(setting_json["SE"][i]["cert"]);
      m_service_endpoints.emplace_back(tmp_info);
    }

    m_mergers.clear();
    for (size_t i = 0; i < setting_json["MG"].size(); ++i) {
      MergerInfo tmp_info;
      tmp_info.id = getIdFromJson(setting_json["MG"][i]["id"]);
      tmp_info.address = setting_json["MG"][i]["address"].get<std::string>();
      tmp_info.port = setting_json["MG"][i]["port"].get<std::string>();
      tmp_info.cert = joinMultiLine(setting_json["MG"][i]["cert"]);

      if (tmp_info.id == m_id) {
        m_cert = tmp_info.cert;
      }

      m_mergers.emplace_back(tmp_info);
    }

    m_sk_pass = setting_json["pass"];

    setting_json.clear();

    return true;
  }

  inline id_type getMyId() { return m_id; }

  inline std::string getMyAddress() { return m_address; }

  inline std::string getMyPort() { return m_port; }

  inline std::string getMyCert() { return m_cert; }

  inline std::string getMySK() { return m_sk; }

  inline std::string getMyPass() { return m_sk_pass; }

  inline local_chain_id_type getLocalChainId() { return m_localchain_id; }

  inline GruutAuthorityInfo getGruutAuthorityInfo() {
    return m_gruut_authority;
  }

  inline std::vector<MergerInfo> getMergerInfo() { return m_mergers; }

  inline std::vector<ServiceEndpointInfo> getServiceEndpointInfo() {
    return m_service_endpoints;
  }

private:
  local_chain_id_type getChainIdFromJson(json &t_json) {
    std::string id_b64 = t_json.get<std::string>();
    bytes id_bytes = TypeConverter::decodeBase64(id_b64);
    return (local_chain_id_type)TypeConverter::bytesToArray<CHAIN_ID_TYPE_SIZE>(
        id_bytes);
  }

  id_type getIdFromJson(json &t_json) {
    std::string id_b64 = t_json.get<std::string>();
    return (id_type)TypeConverter::decodeBase64(id_b64);
  }
  std::string joinMultiLine(json &mline_json) {
    std::string ret_str;
    for (size_t i = 0; i < mline_json.size(); ++i) {
      ret_str += mline_json[i].get<std::string>();
      if (i != mline_json.size() - 1) {
        ret_str += "\n";
      }
    }
    return ret_str;
  }

  bool validateSchema(json tmp) {
    json_validator schema_validator;
    schema_validator.set_root_schema(SCHEMA_SETTING);
    try {
      schema_validator.validate(tmp);
      std::cout << "Validation succeeded" << std::endl;
      return true;
    } catch (const std::exception &e) {
      std::cout << "Validation failed : " << e.what() << std::endl;
      return false;
    }
  }
};
} // namespace gruut