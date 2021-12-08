#pragma once
#include "custom-types/shared/macros.hpp"
#include "custom-types/shared/coroutine.hpp"
#include "TestPathParser.hpp"

DECLARE_CLASS(Tester, ParserCustomType, "UnityEngine", "MonoBehaviour", sizeof(Il2CppObject) + sizeof(void*), 
    TestPathParser* parser;
    constexpr void Init(TestPathParser& p) {
        parser = &p;
    }

    DECLARE_INSTANCE_METHOD(void, Start);
)