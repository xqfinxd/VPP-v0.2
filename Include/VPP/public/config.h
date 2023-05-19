#pragma once

#ifdef _USRDLL
#define VPP_API __declspec(dllexport)
#else
#define VPP_API __declspec(dllimport)
#endif // _USRDLL
