#pragma once

// Evita que un assert fallido del CRT de MSVC abra un dialogo modal
// bloqueante durante las pruebas (que se queda colgado en una corrida
// automatizada/CI). En su lugar, el mensaje se manda a stderr.
#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
struct TestRuntimeGuard {
    TestRuntimeGuard() {
        _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
        _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
    }
};
#else
struct TestRuntimeGuard {
    TestRuntimeGuard() = default;
};
#endif

static const TestRuntimeGuard g_testRuntimeGuard;
