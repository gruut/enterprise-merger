#include <iostream>

#include "src/config/config.hpp"
#include "src/chain/types.hpp"
#include "src/application.hpp"
#include "src/services/setting.hpp"
#include "src/services/argv_parser.hpp"
#include "src/services/storage.hpp"

#include "easy_logging.hpp"
#include "src/utils/crypto.hpp"
#include "src/utils/get_pass.hpp"
#include "src/utils/safe.hpp"

using namespace std;
using namespace gruut;



int main(int argc, char *argv[]) {

  el::Loggers::getLogger("MAIN");

  CLOG(INFO, "MAIN") << "+--------------------------------------------------------------------------";
  CLOG(INFO, "MAIN") << "| " << config::APP_NAME << " (" << config::APP_CODE_NAME << ")";
  CLOG(INFO, "MAIN") << "| build: " << config::APP_BUILD_DATE << " " << config::APP_BUILD_TIME;
  CLOG(INFO, "MAIN") << "+--------------------------------------------------------------------------";

  ArgvParser argv_parser;
  json setting_json = argv_parser.parse(argc,argv);
  if(setting_json.empty()) {
    CLOG(ERROR, "MAIN") << "Setting file is empty or invalid path was given";
    return -1;
  }

  bool clear_db = Safe::getBoolean(setting_json,"dbclear");

  auto setting = Setting::getInstance();

  if(!setting->setJson(setting_json)) {
    CLOG(ERROR, "MAIN") << "Setting file is not a valid JSON";
    return -2;
  }

  if(GemCrypto::isEncPem(setting->getMySK())) {

    if(!setting->getMyPass().empty() && !GemCrypto::isValidPass(setting->getMySK(),setting->getMyPass())) {
      CLOG(ERROR, "MAIN") << "+-------------------------------------------------------------------------+";
      CLOG(ERROR, "MAIN") << "| Wrong Password! :(                                                      |";
      CLOG(ERROR, "MAIN") << "+-------------------------------------------------------------------------+";
      CLOG(ERROR, "MAIN") << "";
      return 1;
    }

    CLOG(INFO, "MAIN") << "";
    CLOG(INFO, "MAIN") << "+-------------------------------------------------------------------------+";
    CLOG(INFO, "MAIN") << "| Good. Merger's signing key is encrypted.                                |";
    CLOG(INFO, "MAIN") << "| To run this merger properly, you must provide a valid password.         |";
    CLOG(INFO, "MAIN") << "+-------------------------------------------------------------------------+";

    std::string user_pass;
    int num_retry = 0;
    do {
      user_pass = getPass::get("Enter the password ", true);
      ++num_retry;
      std::this_thread::sleep_for(std::chrono::seconds(num_retry*num_retry));
    }
    while(!GemCrypto::isValidPass(setting->getMySK(),user_pass));
    setting->setPass(user_pass);

    CLOG(INFO, "MAIN") << "+-------------------------------------------------------------------------+";
    CLOG(INFO, "MAIN") << "| Password is OK. :)                                                      |";
    CLOG(INFO, "MAIN") << "+-------------------------------------------------------------------------+";
    CLOG(INFO, "MAIN") << "";
  }

  if(clear_db) {
    auto storage = Storage::getInstance();
    storage->destroyDB();
    CLOG(INFO, "MAIN") << "THE EXISTING DB HAS BEEN CLEARED.";
  }

  Application::app().setup();
  Application::app().start();
  Application::app().exec();
  Application::app().quit();

  return 0;
}
