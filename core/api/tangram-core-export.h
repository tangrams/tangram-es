#ifndef _TANGRAM_CORE_EXPORT_H_
#define _TANGRAM_CORE_EXPORT_H_

#if defined _WIN32 || defined __CYGWIN__
  #ifdef TANGRAM_CORE_EXPORT_NEEDED
    #ifdef __GNUC__
      #define TANGRAM_CORE_EXPORT __attribute__ ((dllexport))
    #else
      #define TANGRAM_CORE_EXPORT __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
    #endif
  #else
    #ifdef __GNUC__
      #define TANGRAM_CORE_EXPORT __attribute__ ((dllimport))
    #else
      #define TANGRAM_CORE_EXPORT __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
    #endif
  #endif
  #define TANGRAM_CORE_LOCAL
#else
  #if __GNUC__ >= 4
    #define TANGRAM_CORE_EXPORT __attribute__ ((visibility ("default")))
    #define TANGRAM_CORE_LOCAL __attribute__ ((visibility ("hidden")))
  #else
    #define TANGRAM_CORE_EXPORT
    #define TANGRAM_CORE_LOCAL
  #endif
#endif

#endif // _TANGRAM_CORE_EXPORT_H_