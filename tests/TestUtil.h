#pragma once

#include <cstdio>
#include <cinttypes>
#include <string>

inline int g_Failures = 0;
inline int g_Checks = 0;

#define CHECK(cond)                                                              \
    do {                                                                         \
        g_Checks++;                                                              \
        if (!(cond)) {                                                           \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);               \
            g_Failures++;                                                        \
        }                                                                        \
    } while (0)

#define CHECK_EQ(actual, expected)                                               \
    do {                                                                         \
        g_Checks++;                                                              \
        auto a_ = (actual);                                                      \
        auto e_ = (expected);                                                    \
        if (a_ != e_) {                                                          \
            printf("FAIL %s:%d: %s\n  actual   %" PRId64 "\n  expected %" PRId64 "\n", \
                   __FILE__, __LINE__, #actual, (int64_t)a_, (int64_t)e_);       \
            g_Failures++;                                                        \
        }                                                                        \
    } while (0)

#define CHECK_EQ_STR(actual, expected)                                           \
    do {                                                                         \
        g_Checks++;                                                              \
        std::string a_ = (actual);                                               \
        std::string e_ = (expected);                                             \
        if (a_ != e_) {                                                          \
            printf("FAIL %s:%d: %s\n  actual   '%s'\n  expected '%s'\n",         \
                   __FILE__, __LINE__, #actual, a_.c_str(), e_.c_str());         \
            g_Failures++;                                                        \
        }                                                                        \
    } while (0)

inline int TestSummary(const char* name) {
    printf("%s: %d checks, %d failures\n", name, g_Checks, g_Failures);
    return g_Failures ? 1 : 0;
}
