#pragma once
/// @file test_framework.h
/// @brief Lightweight test framework — no external dependencies.
///
/// Usage:
///   TEST(MySuite_TestName) {
///       ASSERT_TRUE(1 + 1 == 2);
///       ASSERT_EQ(42, 42);
///   }
///
/// Each test_*.cpp defines TEST cases.  test_main.cpp calls test::runAll().
/// All TEST macros register into a single global vector via inline variables (C++17).

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace test {

// ---------------------------------------------------------------------------
// Test registry
// ---------------------------------------------------------------------------

struct TestCase {
    std::string            name;
    std::function<void()>  func;
};

/// Global test vector — shared across all TUs via C++17 inline variable.
inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

/// Auto-registrar: static instances register tests before main().
struct Registrar {
    Registrar(const char* name, std::function<void()> func) {
        registry().push_back({name, std::move(func)});
    }
};

// ---------------------------------------------------------------------------
// Counters (shared across TUs)
// ---------------------------------------------------------------------------

inline int         failCount  = 0;
inline int         passCount  = 0;
inline std::string currentTest;

// ---------------------------------------------------------------------------
// Macros
// ---------------------------------------------------------------------------

/// Define a test case.  The body follows in braces.
#define TEST(name)                                                  \
    static void test_fn_##name();                                   \
    static ::test::Registrar reg_##name(#name, test_fn_##name);     \
    static void test_fn_##name()

/// Assert that `cond` is true.
#define ASSERT_TRUE(cond)                                           \
    do {                                                            \
        if (!(cond)) {                                              \
            std::cerr << "    FAIL: " << #cond                      \
                      << "  (" << __FILE__ << ":" << __LINE__       \
                      << ")\n";                                     \
            ++::test::failCount;                                    \
            return;                                                 \
        }                                                           \
    } while (0)

/// Assert that `cond` is false.
#define ASSERT_FALSE(cond) ASSERT_TRUE(!(cond))

/// Assert `a == b`, printing both values on failure.
#define ASSERT_EQ(a, b)                                             \
    do {                                                            \
        auto _va = (a);                                             \
        auto _vb = (b);                                             \
        if (_va != _vb) {                                           \
            std::cerr << "    FAIL: " << #a << " == " << #b        \
                      << "  (" << _va << " != " << _vb << ")"      \
                      << "  (" << __FILE__ << ":" << __LINE__       \
                      << ")\n";                                     \
            ++::test::failCount;                                    \
            return;                                                 \
        }                                                           \
    } while (0)

/// Assert `a != b`.
#define ASSERT_NE(a, b)                                             \
    do {                                                            \
        auto _va = (a);                                             \
        auto _vb = (b);                                             \
        if (_va == _vb) {                                           \
            std::cerr << "    FAIL: " << #a << " != " << #b        \
                      << "  (both " << _va << ")"                   \
                      << "  (" << __FILE__ << ":" << __LINE__       \
                      << ")\n";                                     \
            ++::test::failCount;                                    \
            return;                                                 \
        }                                                           \
    } while (0)

/// Assert `a >= b`.
#define ASSERT_GE(a, b)                                             \
    do {                                                            \
        auto _va = (a);                                             \
        auto _vb = (b);                                             \
        if (_va < _vb) {                                            \
            std::cerr << "    FAIL: " << #a << " >= " << #b        \
                      << "  (" << _va << " < " << _vb << ")"       \
                      << "  (" << __FILE__ << ":" << __LINE__       \
                      << ")\n";                                     \
            ++::test::failCount;                                    \
            return;                                                 \
        }                                                           \
    } while (0)

/// Assert `a <= b`.
#define ASSERT_LE(a, b)                                             \
    do {                                                            \
        auto _va = (a);                                             \
        auto _vb = (b);                                             \
        if (_va > _vb) {                                            \
            std::cerr << "    FAIL: " << #a << " <= " << #b        \
                      << "  (" << _va << " > " << _vb << ")"       \
                      << "  (" << __FILE__ << ":" << __LINE__       \
                      << ")\n";                                     \
            ++::test::failCount;                                    \
            return;                                                 \
        }                                                           \
    } while (0)

/// Assert pointer is null.
#define ASSERT_NULL(ptr)     ASSERT_TRUE((ptr) == nullptr)

/// Assert pointer is non-null.
#define ASSERT_NOT_NULL(ptr) ASSERT_TRUE((ptr) != nullptr)

/// Assert that `expr` throws an exception of type `ExType`.
#define ASSERT_THROWS(expr, ExType)                                 \
    do {                                                            \
        bool _caught = false;                                       \
        try { expr; }                                               \
        catch (const ExType&) { _caught = true; }                   \
        catch (...) {}                                              \
        if (!_caught) {                                             \
            std::cerr << "    FAIL: expected " << #ExType           \
                      << " from: " << #expr                         \
                      << "  (" << __FILE__ << ":" << __LINE__       \
                      << ")\n";                                     \
            ++::test::failCount;                                    \
            return;                                                 \
        }                                                           \
    } while (0)

// ---------------------------------------------------------------------------
// Runner
// ---------------------------------------------------------------------------

/// Run all registered tests and print results.  Returns 0 on success.
inline int runAll() {
    failCount = 0;
    passCount = 0;

    std::cout << "\n"
              << "╔══════════════════════════════════════╗\n"
              << "║   Custom Memory Allocator Tests      ║\n"
              << "║   Running " << registry().size() << " tests"
              << std::string(25 - std::to_string(registry().size()).size(), ' ')
              << "║\n"
              << "╚══════════════════════════════════════╝\n\n";

    for (auto& tc : registry()) {
        currentTest   = tc.name;
        int prevFails = failCount;

        std::cout << "  [ RUN  ]  " << tc.name << "\n";
        try {
            tc.func();
        } catch (const std::exception& e) {
            std::cerr << "    EXCEPTION: " << e.what() << "\n";
            ++failCount;
        } catch (...) {
            std::cerr << "    UNKNOWN EXCEPTION\n";
            ++failCount;
        }

        if (failCount == prevFails) {
            ++passCount;
            std::cout << "  [ PASS ]  " << tc.name << "\n";
        } else {
            std::cout << "  [ FAIL ]  " << tc.name << "\n";
        }
    }

    std::cout << "\n"
              << "══════════════════════════════════════\n"
              << "  Results:  " << passCount << " passed,  "
              << failCount << " failed\n"
              << "══════════════════════════════════════\n\n";

    return failCount > 0 ? 1 : 0;
}

} // namespace test
