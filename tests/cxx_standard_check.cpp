#if defined(_MSVC_LANG)
#  define CXX_STANDARD _MSVC_LANG
#else
#  define CXX_STANDARD __cplusplus
#endif

#if CXX_STANDARD != PARSLEY_TESTS_CXX_STANDARD
#error "Compiler standard does not match PARSLEY_TESTS_CXX_STANDARD"
#endif
