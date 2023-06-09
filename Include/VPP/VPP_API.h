#pragma once

#ifdef VPP_EXPORTS
#define VPP_API __declspec(dllexport)
#else
#define VPP_API __declspec(dllimport)
#endif