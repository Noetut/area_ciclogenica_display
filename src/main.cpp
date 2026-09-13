#include "app/Application.h"
#include <iostream>
#include <string>
#include <cstdlib>

int main(int argc, char* argv[]) {
    int targetMonitor = 1;      // Default: 2nd monitor (0-indexed: 0 = 1st, 1 = 2nd)
    bool enableBlink = false;   // Default: Blinking OFF
    double blinkInterval = 1.0; // Default interval in seconds

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--display" || arg == "-d") {
            if (i + 1 < argc) {
                targetMonitor = std::atoi(argv[++i]);
            }
        } else if (arg == "--blink" || arg == "-b") {
            enableBlink = true;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                try {
                    blinkInterval = std::stod(argv[++i]);
                } catch (...) {
                    blinkInterval = 1.0;
                }
            }
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: ./PatronAnimation [options]\n\n"
                      << "Options:\n"
                      << "  --display <id>, -d <id>    Target monitor index (0 = 1st, 1 = 2nd, default: 1)\n"
                      << "  --blink <sec>,  -b <sec>   Enable full blink loop with given interval in seconds\n"
                      << "                             (e.g., --blink 0.5). If omitted, blinking stays OFF.\n"
                      << "  --help, -h                 Show this help message\n\n"
                      << "Examples:\n"
                      << "  ./PatronAnimation --display 1\n"
                      << "  ./PatronAnimation --display 1 --blink 0.5\n";
            return 0;
        } else {
            // Positional fallback (e.g., ./PatronAnimation 1)
            if (isdigit(static_cast<unsigned char>(arg[0])) || 
               (arg[0] == '-' && arg.length() > 1 && isdigit(static_cast<unsigned char>(arg[1])))) {
                targetMonitor = std::atoi(arg.c_str());
            }
        }
    }

    std::cout << "============================================" << std::endl;
    std::cout << "      Patrón Animation Engine v1.0          " << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << "Config: Target Monitor: #" << targetMonitor << " | Blinking: "
              << (enableBlink ? ("ON (" + std::to_string(blinkInterval) + "s)") : "OFF") << std::endl;
    std::cout << "Usage: ./PatronAnimation --display <id> [--blink <sec>]" << std::endl;
    std::cout << "============================================" << std::endl;

    Application app;
    if (!app.Initialize(targetMonitor, enableBlink, blinkInterval)) {
        std::cerr << "Fatal Error: Failed to initialize application." << std::endl;
        return 1;
    }

    app.Run();

    return 0;
}
