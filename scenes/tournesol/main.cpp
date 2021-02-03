#include "tournesol.hpp"

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>

int main() {
    tournesol::Tournesol tournesol{};
    try {
        tournesol.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}