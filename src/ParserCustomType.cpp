#include "shared/ParserCustomType.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "custom-types/shared/coroutine.hpp"
#include "shared/TestPathParser.hpp"

Logger& getLogger() noexcept;

DEFINE_TYPE(Tester, ParserCustomType);

namespace Tester {
    std::optional<custom_types::Helpers::Coroutine> matchInMap(std::string const& line, TestPathParser::MapT parseMap, std::smatch& matches, ParserCustomType* inst) {
        for (auto regexPair : parseMap) {
            // For each defined regexPair parse, apply it first
            if (std::regex_search(line, matches, std::regex(regexPair.first))) {
                return regexPair.second(matches, inst);
                // As soon as we match even a SINGLE time, we break.
            }
        }
        // Fail to match anything returns false
        return std::nullopt;
    }

    custom_types::Helpers::Coroutine parserCoro(ParserCustomType* inst) {
        std::ifstream str(inst->parser->path);
        if (str.is_open()) {
            // Try parse the file
            // If we fail to parse the file for any reason we ret false
            std::string line;
            while (std::getline(str, line)) {
                if (line.length() <= 1) {
                    // A line that is whitespace is ignored.
                    continue;
                }
                else if (line.starts_with('#')) {
                    // A line that starts with a # is ignored.
                    getLogger().info("Skipping commented line: %s", line.c_str());
                    continue;
                }
                std::smatch matches;
                auto opt = matchInMap(line, inst->parser->customParses, matches, inst);
                if (opt) {
                    getLogger().info("Match found for line: %s", line.c_str());
                    co_yield reinterpret_cast<custom_types::Helpers::enumeratorT*>(custom_types::Helpers::CoroutineHelper::New(std::move(*opt)));
                    // TODO: Do we want to impose delays between lines here? Probably not.
                } else if ((opt = matchInMap(line, inst->parser->builtInParses, matches, inst))) {
                    getLogger().info("Fallback match found for line: %s", line.c_str());
                    co_yield reinterpret_cast<custom_types::Helpers::enumeratorT*>(custom_types::Helpers::CoroutineHelper::New(std::move(*opt)));
                } else {
                    // Fail to parse line!
                    // Log line and return false.
                    getLogger().error("Failed to parse line: %s", line.c_str());
                    co_return;
                }
            }
            co_return;
        }
        else {
            getLogger().error("FAILED TO OPEN FILE: %s", inst->parser->path.c_str());
            co_return;
        }
    }

    void ParserCustomType::Start() {
        il2cpp_utils::RunMethod(this, "StartCoroutine", custom_types::Helpers::CoroutineHelper::New(parserCoro(this)));
    }
}