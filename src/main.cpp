#include "app/Application.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>

namespace {

void printUsage() {
    std::puts(
        "Usage: FluidBrushCG [--validation] [--frames N]\n"
        "  --validation  enable the Khronos validation layer (default in Debug builds)\n"
        "  --frames N    exit after N presented frames (smoke testing)");
}

bool parseArguments(int argc, char** argv, ApplicationOptions& options) {
#ifndef NDEBUG
    options.validation = true;
#endif
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--validation") == 0) {
            options.validation = true;
        } else if (std::strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
            char* end = nullptr;
            options.maxFrames = std::strtoull(argv[++i], &end, 10);
            if (end == argv[i] || *end != '\0') {
                return false;
            }
        } else {
            return false;
        }
    }
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    ApplicationOptions options;
    if (!parseArguments(argc, argv, options)) {
        printUsage();
        return 2;
    }

    try {
        Application application(options);
        application.run();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "Fatal error: %s\n", error.what());
        return 1;
    }
    return 0;
}
