#pragma once

// Goal: Create methods here that are capable of being used to deduce particular characteristics about the environment
// This MOD should only ever be used assuming we are in a development environment
// This should simplify development in general drastically.
#include <unordered_map>
#include <regex>
#include <dlfcn.h>
#include "beatsaber-hook/shared/config/config-utils.hpp"
#include <fstream>
#include "custom-types/shared/coroutine.hpp"

namespace Tester {
    struct ParserCustomType;

    struct TestPathParser {
        constexpr static const char* testPathSymbolName = "testparses";
        const char* goName = "Tester";
        using CallbackT = custom_types::Helpers::Coroutine (std::smatch const&, ParserCustomType*);
        // Strings are used for regexes here
        using MapT = std::unordered_map<std::string, std::function<CallbackT>>;
        MapT customParses;
        MapT builtInParses;
        using SignatureT = void (*)(MapT*);

        std::string path;

        bool try_add(void* handle) {
            // Look for specific symbol in handle.
            auto symb = dlsym(handle, testPathSymbolName);
            if (symb) {
                // Call it and populate it with our custom functions
                reinterpret_cast<SignatureT>(symb)(&customParses);
                return true;
            }
            return false;
        }

        bool load(std::string path_, Logger& logger) {
            path = path_;
            if (!fileexists(path)) {
                logger.error("THERE IS NO TEST PATH FILE TO LOAD!");
                logger.error("PLEASE ENSURE A FILE EXISTS AT: %s", path.c_str());
                return false;
            }
            logger.info("Loaded test file at: %s", path.c_str());
            return true;
        }
    };

}