#include "app/Application.h"
#include <iostream>

int main(int argc, char* argv[]) {
    int targetMonitor = 1; // Default: 2nd monitor (0-indexed: 0 = 1st, 1 = 2nd)

    if (argc > 1) {
        targetMonitor = std::atoi(argv[1]);
    }

    std::cout << "============================================" << std::endl;
    std::cout << "      Patrón Animation Engine v1.0          " << std::endl;
    std::cout << "============================================" << std::endl;

    Application app;
    if (!app.Initialize(targetMonitor)) {
        std::cerr << "Fatal Error: Failed to initialize application." << std::endl;
        return 1;
    }

    app.Run();

    return 0;
}
