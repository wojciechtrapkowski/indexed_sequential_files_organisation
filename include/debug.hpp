#pragma once
#include <iostream>

#ifndef DEBUG_OUTPUT
#define DEBUG_CERR \
    if (false)     \
    std::cerr
#else
#define DEBUG_CERR std::cerr
#endif