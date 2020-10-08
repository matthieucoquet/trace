#include "ingenuity.hpp"

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>

int main() {
    ingenuity::Ingenuity ingenuity{};
    try {
        ingenuity.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}