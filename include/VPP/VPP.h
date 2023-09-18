#ifndef _VPP_H_
#define _VPP_H_

#if defined(_WIN32) && defined(VPP_BUILD_DLL)
#define VPP_API __declspec(dllexport)
#elif defined(_WIN32) && defined(VPP_DLL)
#define VPP_API __declspec(dllimport)
#elif defined(__GNUC__) && defined(VPP_BUILD_DLL)
#define VPP_API __attribute__((visibility("default")))
#else
#define VPP_API
#endif

#endif