#ifndef IOL_DETAIL_CONFIG_HPP
#define IOL_DETAIL_CONFIG_HPP

#if !defined(NDEBUG)
#include <cassert>
#endif

#if defined(__GNUC__) || defined(__clang__)
#define IOL_UNREACHABLE() __builtin_unreachable()
#else
#define IOL_UNREACHABLE() (void)0
#endif

#if defined __GNUC__
#define IOL_LIKELY(EXPR) __builtin_expect(!!(EXPR), 1)
#else
#define IOL_LIKELY(EXPR) (!!(EXPR))
#endif

#if defined NDEBUG
#define IOL_ASSERT(CHECK) void(0)
#else
#define IOL_ASSERT(CHECK) (IOL_LIKELY(CHECK) ? void(0) : [] { assert(!#CHECK); }())
#endif

#if defined(__clang__)
#define IOL_CLANG 1
#elif defined(__GNUC__) || defined(__GNUG__)
#define IOL_GCC 1
#endif

#ifndef IOL_CLANG
#define IOL_CLANG 0
#endif

#ifndef IOL_GCC
#define IOL_GCC 0
#endif

#if IOL_CLANG

#if __clang_major__ >= 7
#define IOL_SYMMETRIC_TRANSFER 1
#endif

#elif IOL_GCC

// GCC supports symmetric transfer on optimized builds
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=100897
// Not sure what version has the initial support, but it works on 11.1
#if __GNUC__ >= 11 && __GNUC_MINOR__ >= 1 && defined(NDEBUG)
#define IOL_SYMMETRIC_TRANSFER 1
#endif

#endif

#ifndef IOL_SYMMETRIC_TRANSFER
#define IOL_SYMMETRIC_TRANSFER 0
#endif

#endif  // IOL_DETAIL_CONFIG_HPP
