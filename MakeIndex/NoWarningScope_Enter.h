//  Push the current diagnostic state and silence *all* warnings on
//  MSVC, Clang and GCC.  Pair with NoWarningScope_Leave.h.

#if defined(_MSC_VER)

#  pragma warning( push )

#  pragma warning( disable : 4291 4996 4068 4100 4127 4189 4201 4244 \
                                4251 4275 4355 4365 4456 4457 4458 4464 \
                                4505 4514 4571 4623 4625 4626 4668 4710 \
                                4711 4820 4826 4868 4917 5026 5027 5031 \
                                5039 5045 5262 5264 )

#elif defined(__clang__)

#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Weverything"

#elif defined(__GNUC__)

#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wall"
#  pragma GCC diagnostic ignored "-Wextra"
#  pragma GCC diagnostic ignored "-Wpedantic"

#endif
