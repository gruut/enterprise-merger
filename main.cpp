#include <iostream>
#include "src/application.hpp"

int main() {
    gruut::Application::app().get_io_service();

    return 0;
}