//-----------------------------------------------------------------------------
#ifndef DataConversionH
#define DataConversionH DataConversionH
//-----------------------------------------------------------------------------
#include <string>

#if ( defined(linux) || defined(__linux) || defined(__linux__) ) && ( defined(__x86_64__) || defined(__powerpc64__) ) // -m64 makes GCC define __powerpc64__
#   define MY_FMT_I64 "%ld"
#   define MY_FMT_I64_0_PADDED "%020ld"
#else
#   define MY_FMT_I64 "%lld"
#   define MY_FMT_I64_0_PADDED "%020lld"
#endif // #if ( defined(linux) || defined(__linux) || defined(__linux__) ) && ( defined(__x86_64__) || defined(__powerpc64__) ) // -m64 makes GCC define __powerpc64__

std::string charToFormat( int c );
std::string charToType( int c );

#endif // DataConversionH
