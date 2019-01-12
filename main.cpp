#include <iostream>

#include "src/config/config.hpp"
#include "src/chain/types.hpp"
#include "src/application.hpp"
#include "src/services/setting.hpp"
#include "src/services/argv_parser.hpp"
#include "src/services/storage.hpp"

#include "easy_logging.hpp"

using namespace std;
using namespace gruut;



int main(int argc, char *argv[]) {

  el::Loggers::getLogger("MAIN");

  // clang-format off
  CLOG(INFO, "MAIN") << "+--------------------------------------------------------------------------";
  CLOG(INFO, "MAIN") << "| " << config::APP_NAME << " (" << config::APP_CODE_NAME << ")";
  CLOG(INFO, "MAIN") << "| build: " << config::APP_BUILD_DATE << " " << config::APP_BUILD_TIME;
  CLOG(INFO, "MAIN") << "+--------------------------------------------------------------------------";
  // clang-format on

  ArgvParser argv_parser;
  if(!argv_parser.parse(argc,argv)) {
    CLOG(ERROR, "MAIN") << "Error on loading system";
    return -1;
  }

  auto setting = Setting::getInstance();

  // clang-format off
  CLOG(INFO, "MAIN") << "+--------------------------------------------------------------------------";
  CLOG(INFO, "MAIN") << "| ID : " << TypeConverter::encodeBase64(setting->getMyId());
  CLOG(INFO, "MAIN") << "| Local Chain : " << TypeConverter::encodeBase64(setting->getLocalChainId());
  CLOG(INFO, "MAIN") << "| Address : " << setting->getMyAddress() << ":" << setting->getMyPort();
  CLOG(INFO, "MAIN") << "| DB Path : " << setting->getMyDbPath();
  CLOG(INFO, "MAIN") << "+--------------------------------------------------------------------------";
  // clang-format on


  Application::app().setup();
  Application::app().start();
  Application::app().exec();
  Application::app().quit();

  return 0;
}
