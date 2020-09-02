#include "engine.hpp"

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>

int main() {
    Engine engine{};
    try {
        engine.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}