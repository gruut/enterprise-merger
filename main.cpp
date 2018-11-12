#include <iostream>
#include <vector>

#include "src/application.hpp"
#include "src/module.hpp"

using namespace gruut;
using namespace std;

int main() {
    vector<Module*> module_vector;
    Application::app().start(move(module_vector));
    Application::app().exec();
    Application::app().quit();

    return 0;
}