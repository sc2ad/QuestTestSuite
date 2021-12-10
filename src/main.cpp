// Mod should have a single hook on game initialization/restart
// It should log that it exists and SHOULD NOT EXIST!
// Perhaps some specific opt-in to actually make it do something? Maybe some sort of file must exist?

// Main level hook will perform any testing operations that have been registered.
// For example, if we want to move to the main menu, we will do that
// If we want to move to the main menu and press some buttons and whatnot, we will do that as well.


// IDEA
// We have a file that we parse
// We have THIS MOD call an otherwise unused extern function from each loaded mod
// If we find it, we call it and collect the return as specialized handlers
// If we don't, then we just parse the file as normal
#include "modloader/shared/modloader.hpp"
#include "beatsaber-hook/shared/utils/logging.hpp"
#include "shared/TestPathParser.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "shared/ParserCustomType.hpp"

static ModInfo modInfo;
constexpr const ModInfo& getInfo() noexcept {
    return modInfo;
}

Logger& getLogger() noexcept {
    static Logger* logger = new Logger(modInfo, LoggerOptions(false, true));
    return *logger;
}

extern "C" void setup(ModInfo& info) {
    info.id = ID;
    info.version = VERSION;
    modInfo = info;
}

Tester::TestPathParser parser;
static bool toLoad;

#define DECLARE_BUILTIN(name, regex) \
static auto name##_rgx = regex; \
static custom_types::Helpers::Coroutine name(std::smatch const& matches, Tester::ParserCustomType* inst)

#define BUILTIN(name) \
std::make_pair(name##_rgx, std::function(name))

DECLARE_BUILTIN(StartGame, "start the game") {
    // A built in that ensures we have started the game. The hook to be registered here would be the health and safety one.
    // This can simply do nothing.
    co_return;
}

DECLARE_BUILTIN(FindComponentAndCallMethod, "find the component at path \"([^\"]+)\" of type namespace \"([^\"]+)\" and name \"([^\"]+)\" and call method \"([^\"]+)\"") {
    auto path = matches[1].str();
    auto typeNamespace = matches[2].str();
    auto typeName = matches[3].str();
    auto m = matches[4].str();
    auto instance = CRASH_UNLESS(il2cpp_utils::RunMethodUnsafe("UnityEngine", "GameObject", "Find", il2cpp_utils::newcsstr(path)));
    co_yield nullptr;
    auto sysType = CRASH_UNLESS(il2cpp_utils::GetSystemType(typeNamespace, typeName));
    auto matchingComp = CRASH_UNLESS(il2cpp_utils::RunMethodUnsafe(instance, "GetComponent", sysType));
    CRASH_UNLESS(matchingComp);
    co_yield nullptr;
    auto methodToInvoke = CRASH_UNLESS(il2cpp_utils::FindMethodUnsafe(instance, m, 0));
    CRASH_UNLESS(il2cpp_utils::RunMethod(matchingComp, methodToInvoke));
    co_return;
}

extern "C" void init() {
    toLoad = parser.load(getDataDir(modInfo) + "testpath.feature", getLogger());
    if (!toLoad) {
        return;
    }
    // Add our built in regexes here.
    parser.builtInParses.push_back(BUILTIN(StartGame));
    parser.builtInParses.push_back(BUILTIN(FindComponentAndCallMethod));
    // This is called early, but after dlopen, so the logs will be in a reasonable location.
    for (auto itr : Modloader::getMods()) {
        if (itr.second.get_loaded()) {
            if (parser.try_add(itr.second.handle)) {
                getLogger().info("Added custom parsing functionality from mod: %s", itr.first.c_str());
            }
        }
    }
}

MAKE_HOOK_FIND_CLASS_UNSAFE_INSTANCE(Initialization_HWVC_DidActivate, "", "HealthWarningViewController", "DidActivate", void,
            Il2CppObject* self, bool first, bool added, bool screenSystem) {
    // Call orig first, then we perform our operations.
    // As far as our operations go, we might need to offload each pass to a coroutine, since otherwise it is all one long blocking chain.
    Initialization_HWVC_DidActivate(self, first, added, screenSystem);
    static auto goName = il2cpp_utils::newcsstr<il2cpp_utils::CreationType::Manual>(parser.goName);
    auto inst = CRASH_UNLESS(il2cpp_utils::New("UnityEngine", "GameObject", goName));
    il2cpp_utils::RunMethod("UnityEngine", "Object", "DontDestroyOnLoad", inst);
    // Then we add the component for it
    auto* parserInst = CRASH_UNLESS(il2cpp_utils::RunMethod(inst, "AddComponent", csTypeOf(Tester::ParserCustomType*)));
    reinterpret_cast<Tester::ParserCustomType*>(parserInst)->Init(parser);
}

extern "C" void load() {
    // On load, we install our hooks to unity init and then we do all of our stuff then
    // BUT we only do this if we have successfully parsed our test file.
    getLogger().warning("**************************************************************");
    getLogger().warning("IF YOU HAVE THIS MOD INSTALLED, YOU SHOULD BE USING TEST CODE!");
    getLogger().warning("IF YOU ARE NOT, PLEASE CONSIDER UNINSTALLING THIS MOD!");
    getLogger().warning("**************************************************************");
    if (toLoad) {
        getLogger().info("TEST CODE FOUND!");
        // TODO: Some day we will parse the given expression in order to deduce which hook we want to install to.
        // For now, we will always start assuming we are at the health and safety screen and will move forward from there.
        custom_types::Register::AutoRegister();
        // Install hooks here.
        INSTALL_HOOK(getLogger(), Initialization_HWVC_DidActivate)
    }
}