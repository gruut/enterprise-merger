#include <iostream>
#include <vector>

#include "src/application.hpp"
#include "src/modules/module.hpp"
#include "src/modules/communication/communication.hpp"
#include "src/modules/signature_requester/signature_requester.hpp"
#include "src/modules/transaction_collector/transaction_collector.hpp"

using namespace gruut;
using namespace std;

int main() {
    vector<shared_ptr<Module>> module_vector;
//    module_vector.push_back(shared_ptr<Communication>(new Communication()));
    module_vector.push_back(shared_ptr<TransactionCollector>(new TransactionCollector()));

    Application::app().start(move(module_vector));
    Application::app().exec();
    Application::app().quit();

    return 0;
}