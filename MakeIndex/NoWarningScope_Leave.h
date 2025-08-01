//  Restore diagnostic state that was pushed in NoWarningScope_Enter.h

#if defined(_MSC_VER)

#  pragma warning( pop )

#elif defined(__clang__)

#  pragma clang diagnostic pop

#elif defined(__GNUC__)

#  pragma GCC diagnostic pop

#endif
