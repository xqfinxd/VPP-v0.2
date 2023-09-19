#pragma once

#if defined(VPP_USE_CONFIG_H)
#include "Config.h"
#endif

#ifdef _VPP_H_
#error "You must not include VPP.h before Config.h"
#endif

#include "VPP/VPP.h"