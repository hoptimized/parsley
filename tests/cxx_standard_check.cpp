#if defined(_MSVC_LANG)
#  define CXX_STANDARD _MSVC_LANG
#else
#  define CXX_STANDARD __cplusplus
#endif

static_assert(
    CXX_STANDARD == PARSLEY_TESTS_CXX_STANDARD,
    "Compiler C++ standard does not match PARSLEY_TESTS_CXX_STANDARD"
);
