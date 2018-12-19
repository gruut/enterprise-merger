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
    "in", "Setting file",
    cxxopts::value<string>()->default_value("./setting.json"))
    ("pass","Password to decrypt secret key",cxxopts::value<string>()->default_value(""));

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

  // stage 0  modules
  shared_ptr<BootStraper> bootstraper = make_shared<BootStraper>();
  shared_ptr<Communication> moudle_communication = make_shared<Communication>();
  shared_ptr<OutMessageFetcher> module_out_message_fetcher = make_shared<OutMessageFetcher>();

  // stage 1 modules
  shared_ptr<MessageFetcher> module_message_fetcher = make_shared<MessageFetcher>();

  Application::app().regModule(moudle_communication, 0);
  Application::app().regModule(module_out_message_fetcher, 0);
  Application::app().regModule(bootstraper, 0, true);

  Application::app().regModule(module_message_fetcher, 1);

  Application::app().start();
  Application::app().exec();
  Application::app().quit();

  return 0;
}
