#ifndef GRUUT_ENTERPRISE_MERGER_SETTING_HPP
#define GRUUT_ENTERPRISE_MERGER_SETTING_HPP

#include "json-schema.hpp"
#include "nlohmann/json.hpp"

#include "../chain/types.hpp"
#include "../config/config.hpp"
#include "../utils/safe.hpp"
#include "../utils/template_singleton.hpp"
#include "../utils/type_converter.hpp"

#include "easy_logging.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace gruut {
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
  "type": "object",
  "properties": {
    "Self": {
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
  std::string m_db_path;

  GruutAuthorityInfo m_gruut_authority;
  std::vector<ServiceEndpointInfo> m_service_endpoints;
  std::vector<MergerInfo> m_mergers;

public:
  Setting()
      : m_port(config::DEFAULT_PORT_NUM), m_db_path(config::DEFAULT_DB_PATH) {
    el::Loggers::getLogger("SETT");
  }

  Setting(json &setting_json) {
    el::Loggers::getLogger("SETT");
    if (!setJson(setting_json)) {
      CLOG(ERROR, "SETT") << "Failed to parse setting JSON";
    }
  }

  ~Setting() {
    // TODO :: wipe-out m_sk securely
  }

  bool setJson(json &setting_json) {
    if (!validateSchema(setting_json))
      return false;

    m_id = Safe::getBytesFromB64<id_type>(setting_json["Self"], "id");
    m_address = Safe::getString(setting_json["Self"], "address");
    m_port = Safe::getString(setting_json["Self"], "port");

    if(m_port.empty()){
      m_port = config::DEFAULT_PORT_NUM;
    }

    m_sk = joinMultiLine(setting_json["Self"]["sk"]);
    m_localchain_id = getChainIdFromJson(setting_json["LocalChain"]["id"]);
    m_localchain_name = Safe::getString(setting_json["LocalChain"], "name");
    m_db_path = Safe::getString(setting_json, "dbpath");

    m_gruut_authority.id =
        Safe::getBytesFromB64<id_type>(setting_json["GA"], "id");
    m_gruut_authority.address = Safe::getString(setting_json["GA"], "address");
    m_gruut_authority.cert = joinMultiLine(setting_json["cert"]);

    m_service_endpoints.clear();
    for (size_t i = 0; i < setting_json["SE"].size(); ++i) {
      ServiceEndpointInfo tmp_info;
      tmp_info.id = Safe::getBytesFromB64<id_type>(setting_json["SE"][i], "id");
      tmp_info.address = Safe::getString(setting_json["SE"][i], "address");
      tmp_info.port = Safe::getString(setting_json["SE"][i], "port");
      tmp_info.cert = joinMultiLine(setting_json["SE"][i]["cert"]);
      m_service_endpoints.emplace_back(tmp_info);
    }

    m_mergers.clear();
    for (size_t i = 0; i < setting_json["MG"].size(); ++i) {
      MergerInfo tmp_info;
      tmp_info.id = Safe::getBytesFromB64<id_type>(setting_json["MG"][i], "id");
      tmp_info.address = Safe::getString(setting_json["MG"][i], "address");
      tmp_info.port = Safe::getString(setting_json["MG"][i], "port");
      tmp_info.cert = joinMultiLine(setting_json["MG"][i]["cert"]);

      if (tmp_info.id == m_id) {
        m_cert = tmp_info.cert;
      }

      m_mergers.emplace_back(tmp_info);
    }

    m_sk_pass = Safe::getString(setting_json, "pass");

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

  inline std::string getMyDbPath() { return m_db_path; }

  inline GruutAuthorityInfo getGruutAuthorityInfo() {
    return m_gruut_authority;
  }

  inline std::vector<MergerInfo> getMergerInfo() { return m_mergers; }

  inline std::vector<ServiceEndpointInfo> getServiceEndpointInfo() {
    return m_service_endpoints;
  }

private:
  local_chain_id_type getChainIdFromJson(json &t_json) {
    std::string id_b64 = Safe::getString(t_json);
    bytes id_bytes = TypeConverter::decodeBase64(id_b64);
    return static_cast<local_chain_id_type>(
        TypeConverter::bytesToArray<CHAIN_ID_TYPE_SIZE>(id_bytes));
  }

  std::string joinMultiLine(json &mline_json) {
    std::string ret_str;
    for (size_t i = 0; i < mline_json.size(); ++i) {
      ret_str += Safe::getString(mline_json[i]);
      if (i != mline_json.size() - 1) {
        ret_str += "\n";
      }
    }
    return ret_str;
  }

  bool validateSchema(json &tmp) {
    json_validator schema_validator;
    schema_validator.set_root_schema(SCHEMA_SETTING);
    try {
      schema_validator.validate(tmp);
      return true;
    } catch (const std::exception &e) {
      return false;
    }
  }
};
} // namespace gruut

#endif