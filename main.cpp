#include <iostream>

#include "src/config/config.hpp"
#include "src/chain/types.hpp"
#include "src/application.hpp"
#include "src/services/setting.hpp"
#include "src/services/argv_parser.hpp"

#include "easy_logging.hpp"

using namespace std;
using namespace gruut;

int main(int argc, char *argv[]) {

  el::Loggers::getLogger("MAIN");

  CLOG(INFO, "MAIN") << config::APP_NAME << " (" << config::APP_CODE_NAME << ")";
  CLOG(INFO, "MAIN") << "build: " << config::APP_BUILD_DATE << " " << config::APP_BUILD_TIME;
  CLOG(INFO, "MAIN") << "===========================================================================";

  ArgvParser argv_parser;
  json setting_json = argv_parser.parse(argc,argv);
  if(setting_json.empty()) {
    CLOG(ERROR, "MAIN") << "Setting file is empty or invalid path was given";
    return -1;
  }

  auto setting = Setting::getInstance();

  if(!setting->setJson(setting_json)) {
    CLOG(ERROR, "MAIN") << "Setting file is not a valid JSON";
    return -2;
  }

  Application::app().setup();
  Application::app().start();
  Application::app().exec();
  Application::app().quit();

  return 0;
}
