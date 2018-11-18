#include "src/application.hpp"
//#include "src/modules/communication/communication.hpp"

using namespace gruut;
using namespace std;

int main() {
    vector<shared_ptr<Module>> module_vector;
//    module_vector.push_back(shared_ptr<Communication>(new Communication()));
    module_vector.push_back(make_shared<TransactionCollector>());

    Application::app().start(move(module_vector));
    Application::app().exec();
    Application::app().quit();

    return 0;
}