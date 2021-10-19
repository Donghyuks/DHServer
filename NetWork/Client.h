#pragma once

#ifndef NETWORK_EXPORTS
#define NETWORK_DLL __declspec(dllimport)
#else
#define NETWORK_DLL __declspec(dllexport)
#endif

class Client
{
};

