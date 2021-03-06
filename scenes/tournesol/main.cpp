#include "tournesol.hpp"

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <Windows.h>

int main() {
    timeBeginPeriod(1);
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