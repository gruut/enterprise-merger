#pragma once

#include <iostream>

#include "cxxopts.hpp"
#include "nlohmann/json.hpp"

#include "../utils/file_io.hpp"

using namespace nlohmann;

namespace gruut {
class ArgvParser {
public:
  ArgvParser() = default;

  json parse(int argc, char *argv[]) {

    json setting_json;

    cxxopts::Options options(argv[0],
                             "Merger for Gruut Enterprise Networks (C++)\n");
    options.add_options("basic")("help", "Print help description")(
        "in", "Setting file",
        cxxopts::value<string>()->default_value("./setting.json"))(
        "pass", "Password to decrypt secret key",
        cxxopts::value<string>()->default_value(""))(
        "port", "Port number", cxxopts::value<string>()->default_value(""))(
        "dbpath", "DB path", cxxopts::value<string>()->default_value("./db"));

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
        cout << "error opening setting files " << result["in"].as<string>()
             << endl;
        return setting_json;
      }

      setting_json = json::parse(setting_json_str);

      setting_json["pass"] = result["pass"].as<string>();

      auto parsed_port_num = result["port"].as<string>();
      if (!parsed_port_num.empty())
        setting_json["Self"]["port"] = parsed_port_num; // overwrite

      setting_json["dbpath"] = result["dbpath"].as<string>();
      // boost::filesystem::create_directories(parsed_db_path);

    } catch (json::parse_error &e) {
      cout << "error parsing setting files: " << e.what() << endl;
    } catch (const cxxopts::OptionException &e) {
      cout << "error parsing arguments: " << e.what() << endl;
    }

    return setting_json;
  }
};
} // namespace gruut