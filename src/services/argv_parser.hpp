#pragma once

#include <iostream>

#include "cxxopts.hpp"
#include "nlohmann/json.hpp"

#include "../config/config.hpp"
#include "../utils/file_io.hpp"

#include "easy_logging.hpp"

using namespace nlohmann;

namespace gruut {
class ArgvParser {
public:
  ArgvParser() { el::Loggers::getLogger("ARGV"); };

  json parse(int argc, char *argv[]) {

    json setting_json;

    cxxopts::Options options(argv[0],
                             "Merger for Gruut Enterprise Networks (C++)\n");
    options.add_options("basic")("help", "Print help description")(
        "in", "Setting file",
        cxxopts::value<string>()->default_value("./setting.json"))(
        "pass", "Password to decrypt secret key",
        cxxopts::value<string>()->default_value(""))(
        "port", "Port number",
        cxxopts::value<string>()->default_value(config::DEFAULT_PORT_NUM))(
        "dbpath", "DB path",
        cxxopts::value<string>()->default_value(config::DEFAULT_DB_PATH));

    if (argc == 1) {
      cout << options.help({"basic"}) << endl;
      return setting_json;
    }

    try {

      auto result = options.parse(argc, argv);

      if (result.count("help")) {
        cout << options.help({"", "basic"}) << endl;
        return setting_json;
      }

      string setting_json_str = FileIo::file2str(result["in"].as<string>());

      if (setting_json_str.empty()) {
        CLOG(ERROR, "ARGV")
            << "Failed to open setting files " << result["in"].as<string>();
        return setting_json;
      }

      setting_json = json::parse(setting_json_str);

      setting_json["pass"] = result["pass"].as<string>();

      auto parsed_port_num = result["port"].as<string>();
      if (!parsed_port_num.empty() &&
          parsed_port_num != setting_json["Self"]["port"].get<std::string>()) {
        setting_json["Self"]["port"] = parsed_port_num; // override
      }

      auto parsed_db_path = result["dbpath"].as<std::string>();
      if (!parsed_db_path.empty()) {
        setting_json["dbpath"] = result["dbpath"].as<std::string>(); // override
      }

      // boost::filesystem::create_directories(parsed_db_path);

    } catch (json::parse_error &e) {
      CLOG(ERROR, "ARGV") << "Failed to pars setting files - " << e.what();
    } catch (const cxxopts::OptionException &e) {
      CLOG(ERROR, "ARGV") << "Failed to parse arguments - " << e.what();
    }

    return setting_json;
  }
};
} // namespace gruut