//
// Created by Rick Gessner on 1/23/24.
//

#pragma once

#include <iostream>

#if defined (_DEBUG) || ! (defined (NDEBUG) || defined (_NDEBUG))
    #define DEBUG 1
#endif

#if defined (_MSC_VER)
  #define MSVC 1
#endif

#if DEBUG
    #ifdef MSVC // Microsoft Visual Studio
        #define ASSERT(x) if (!(x)) __debugbreak()
    #else // Everything else
        #include <cassert>
        #define ASSERT(x) assert(x)
    #endif

    #define DBG(x) std::cout << x << "\n"
    #define TODO {}
#else
    #define ASSERT(x)
    #define DBG(x)
    #define TODO static_assert(false);
#endif