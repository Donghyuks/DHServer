#pragma once

#ifdef C2NETWORKAPI_EXPORTS
#define C2NETWORKAPI_DLL __declspec(dllexport)
#else
#define C2NETWORKAPI_DLL __declspec(dllimport)
#endif

#include "C2NetworkAPIDefine.h"

