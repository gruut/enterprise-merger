#include <iostream>

#include "src/application.hpp"
#include "src/services/setting.hpp"
#include "src/services/argv_parser.hpp"

using namespace std;
using namespace gruut;
using namespace nlohmann;

int main(int argc, char *argv[]) {

  ArgvParser argv_parser;
  json setting_json = argv_parser.parse(argc,argv);
  if(setting_json.empty()) {
    cout << "Setting file is empty or invalid path was given." << endl;
    return -1;
  }

  auto setting = Setting::getInstance();

  if(!setting->setJson(setting_json)) {
    cout << "Setting file is not a valid json " << endl;
    return -2;
  }

  Application::app().setup();
  Application::app().start();
  Application::app().exec();
  Application::app().quit();

  return 0;
}
