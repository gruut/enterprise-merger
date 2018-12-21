#include <iostream>

#include "cxxopts.hpp"

#include "src/application.hpp"
#include "src/services/setting.hpp"
#include "src/services/input_queue.hpp"
#include "src/services/output_queue.hpp"
#include "src/modules/message_fetcher/message_fetcher.hpp"
#include "src/modules/message_fetcher/out_message_fetcher.hpp"
#include "src/modules/communication/communication.hpp"

#include "src/utils/file_io.hpp"

using namespace std;
using namespace gruut;
using namespace nlohmann;

json parseArg(int argc, char *argv[]){

  cxxopts::Options options(argv[0],
                           "Merger for Gruut Enterprise Networks (C++)\n");
  options.add_options("basic")("help", "Print help description")(
    "in", "Setting file", cxxopts::value<string>()->default_value("./setting.json"))
    ("pass","Password to decrypt secret key",cxxopts::value<string>()->default_value(""))
    ("port", "Port number", cxxopts::value<string>()->default_value(""))
    ("dbpath", "DB path", cxxopts::value<string>()->default_value("./db")
    );

  if (argc == 1) {
    cout << options.help({"basic"}) << endl;
    exit(1);
  }

  try {

    auto result = options.parse(argc, argv);

    if (result.count("help")) {
      cout << options.help({"", "basic"}) << endl;
      exit(1);
    }

    string setting_json_str = FileIo::file2str(result["in"].as<string>());

    if(setting_json_str.empty()) {
      cout << "error opening setting files " << result["in"].as<string>() << endl;
      exit(1);
    }

    json setting_json = json::parse(setting_json_str);

    setting_json["pass"] = result["pass"].as<string>();
    auto parsed_port_num = result["port"].as<string>();
    if(!parsed_port_num.empty())
      setting_json["Self"]["port"] = parsed_port_num;

    auto parsed_db_path = result["dbpath"].as<string>();
    setting_json["dbpath"] = parsed_db_path;
    boost::filesystem::create_directories(parsed_db_path);

    return setting_json;

  } catch (json::parse_error &e) {
    cout << "error parsing setting files: " << e.what() << endl;
    exit(1);
  } catch (const cxxopts::OptionException &e) {
    cout << "error parsing arguments: " << e.what() << endl;
    exit(1);
  }
};


int main(int argc, char *argv[]) {

  json setting_json = parseArg(argc,argv);

  auto setting = Setting::getInstance();
  if(!setting->setJson(setting_json)) {
    cout << "Setting file is not a valid json " << endl;
    return 1;
  }

  Application::app().setup();
  Application::app().start();
  Application::app().exec();
  Application::app().quit();

  return 0;
}
