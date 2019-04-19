/** @addtogroup utility
 *  @{
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "cident.h"

#define KFR_VERSION_MAJOR 3
#define KFR_VERSION_MINOR 0
#define KFR_VERSION_PATCH 9
#define KFR_VERSION_LABEL ""

#define KFR_VERSION_STRING                                                                                   \
    CMT_STRINGIFY(KFR_VERSION_MAJOR)                                                                         \
    "." CMT_STRINGIFY(KFR_VERSION_MINOR) "." CMT_STRINGIFY(KFR_VERSION_PATCH) KFR_VERSION_LABEL
#define KFR_VERSION (KFR_VERSION_MAJOR * 10000 + KFR_VERSION_MINOR * 100 + KFR_VERSION_PATCH)

#if defined DEBUG || defined KFR_DEBUG
#define KFR_DEBUG_STR " debug"
#elif defined NDEBUG || defined KFR_NDEBUG
#define KFR_DEBUG_STR " optimized"
#else
#define KFR_DEBUG_STR ""
#endif

#define KFR_NATIVE_INTRINSICS 1

#if defined CMT_COMPILER_CLANG && !defined CMT_DISABLE_CLANG_EXT
#define CMT_CLANG_EXT
#endif

#ifdef KFR_NATIVE_INTRINSICS
#define KFR_BUILD_DETAILS_1 " +in"
#else
#define KFR_BUILD_DETAILS_1 ""
#endif

#ifdef CMT_CLANG_EXT
#define KFR_BUILD_DETAILS_2 " +ve"
#else
#define KFR_BUILD_DETAILS_2 ""
#endif

#define KFR_VERSION_FULL                                                                                     \
    "KFR " KFR_VERSION_STRING KFR_DEBUG_STR                                                                  \
    " " CMT_STRINGIFY(CMT_ARCH_NAME) " " CMT_ARCH_BITNESS_NAME " (" CMT_COMPILER_FULL_NAME "/" CMT_OS_NAME   \
                                     ")" KFR_BUILD_DETAILS_1 KFR_BUILD_DETAILS_2

#ifdef __cplusplus
namespace kfr
{
/// @brief KFR version string
constexpr const char version_string[] = KFR_VERSION_STRING;

constexpr int version_major = KFR_VERSION_MAJOR;
constexpr int version_minor = KFR_VERSION_MINOR;
constexpr int version_patch = KFR_VERSION_PATCH;
constexpr int version       = KFR_VERSION;

/// @brief KFR version string including architecture and compiler name
constexpr const char version_full[] = KFR_VERSION_FULL;
} // namespace kfr
#endif

#define KFR_INTRINSIC CMT_INTRINSIC
#define KFR_MEM_INTRINSIC CMT_MEM_INTRINSIC
#define KFR_FUNCTION CMT_FUNCTION
