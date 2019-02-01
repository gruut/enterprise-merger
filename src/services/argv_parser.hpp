#ifndef GRUUT_ENTERPRISE_MERGER_ARGV_PARSER_HPP
#define GRUUT_ENTERPRISE_MERGER_ARGV_PARSER_HPP

#include <iostream>

#include "cxxopts.hpp"
#include "easy_logging.hpp"
#include "nlohmann/json.hpp"

#include "../chain/types.hpp"
#include "../config/config.hpp"
#include "../services/setting.hpp"
#include "../services/storage.hpp"
#include "../utils/crypto.hpp"
#include "../utils/file_io.hpp"
#include "../utils/get_pass.hpp"
#include "../utils/safe.hpp"

namespace gruut {

class ArgvParser {
public:
  ArgvParser() { el::Loggers::getLogger("ARGV"); };

  bool parse(int argc, char *argv[]) {

    json setting_json;
    // clang-format off
    cxxopts::Options options(argv[0], config::APP_NAME + "\n");
    options.add_options("basic")
    ("help", "Print help description")
    ("in", "Location of setting file", cxxopts::value<string>()->default_value("./setting.json"))
    ("pass", "Password to decrypt secret key", cxxopts::value<string>()->default_value(""))
    ("port", "Port number", cxxopts::value<string>()->default_value(""))
    ("dbpath", "Location where LevelDB stores data", cxxopts::value<string>()->default_value(config::DEFAULT_DB_PATH))
    ("dbclear", "To wipe out the existing LevelDB")
    ("dbcheck", "To perform DB health check before running")
    ("disableTK", "Not to access to the tracker")
    ("txforward", "To forward MSG_TX to appropriate merger");
    // clang-format on

    if (argc == 1) {
      std::cout << options.help({"", "basic"}) << std::endl;
      exit(1);
    }

    try {

      auto result = options.parse(argc, argv);

      if (result.count("help")) {
        std::cout << options.help({"", "basic"}) << std::endl;
        exit(1);
      }

      string setting_json_str = FileIo::file2str(result["in"].as<string>());

      if (setting_json_str.empty()) {
        CLOG(ERROR, "ARGV")
            << "Failed to open setting files " << result["in"].as<string>();
        return false;
      }

      setting_json = json::parse(setting_json_str); // try-catch below

      setting_json["pass"] = result["pass"].as<string>();

      auto parsed_port_num = result["port"].as<string>();

      if (!parsed_port_num.empty()) {
        setting_json["Self"]["port"] = parsed_port_num; // override
      }

      auto parsed_db_path = result["dbpath"].as<std::string>();
      if (!parsed_db_path.empty()) {
        setting_json["dbpath"] = result["dbpath"].as<std::string>(); // override
      }

      auto setting = Setting::getInstance();

      if (!setting->setJson(setting_json)) {
        CLOG(ERROR, "ARGV") << "Setting file is not a valid JSON";
        return false;
      }

      if (GemCrypto::isEncPem(setting->getMySK())) {

        if (!setting->getMyPass().empty() &&
            !GemCrypto::isValidPass(setting->getMySK(), setting->getMyPass())) {
          // clang-format off
          CLOG(ERROR, "ARGV") << "+-------------------------------------------------------------------------+";
          CLOG(ERROR, "ARGV") << "| Wrong Password! :(                                                      |";
          CLOG(ERROR, "ARGV") << "+-------------------------------------------------------------------------+";
          CLOG(ERROR, "ARGV") << "";
          // clang-format on
          return false;
        }

        // clang-format off
        CLOG(INFO, "ARGV") << "";
        CLOG(INFO, "ARGV") << "+-------------------------------------------------------------------------+";
        CLOG(INFO, "ARGV") << "| Good. Merger's signing key is encrypted.                                |";
        CLOG(INFO, "ARGV") << "| To run this merger properly, you must provide a valid password.         |";
        CLOG(INFO, "ARGV") << "+-------------------------------------------------------------------------+";
        // clang-format on

        std::string user_pass;
        int num_retry = 0;
        do {
          user_pass = getPass::get("Enter the password ", true);
          ++num_retry;
          std::this_thread::sleep_for(
              std::chrono::seconds(num_retry * num_retry));
        } while (!GemCrypto::isValidPass(setting->getMySK(), user_pass));
        setting->setPass(user_pass);

        // clang-format off
        CLOG(INFO, "ARGV") << "+-------------------------------------------------------------------------+";
        CLOG(INFO, "ARGV") << "| Password is OK. :)                                                      |";
        CLOG(INFO, "ARGV") << "+-------------------------------------------------------------------------+";
        CLOG(INFO, "ARGV") << "";
        // clang-format on
      }

      if (result.count("dbclear")) {
        Storage::getInstance()->destroyDB();
        CLOG(INFO, "ARGV") << "EXISTING DB HAS BEEN CLEARED.";
      }

      if (result.count("dbcheck")) {
        setting->setDBCheck();
        CLOG(INFO, "ARGV") << "DB HEALTH CHECKING IS ENABLED.";
      }

      if (result.count("disableTK")) {
        setting->setDisableTracker();
        CLOG(INFO, "ARGV") << "MERGER DOES NOT ACCESS TO TRACKER.";
      }

      if (result.count("txforward")) {
        setting->setTxForward();
        CLOG(INFO, "ARGV") << "MSG_TX FORWARD IS ENABLED.";
      }

    } catch (json::parse_error &e) {
      CLOG(ERROR, "ARGV") << "Failed to pars setting files - " << e.what();
      return false;
    } catch (const cxxopts::OptionException &e) {
      CLOG(ERROR, "ARGV") << "Failed to parse arguments - " << e.what();
      return false;
    }

    return true;
  }
};
} // namespace gruut

#endif