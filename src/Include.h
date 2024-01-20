#pragma once

#pragma warning(push, 0) // Disable warnings from external libraries
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#pragma warning (pop)

#include <iostream>
#include <fstream>

// Macros

#define DEBUG_ON true
#define DEBUG_OFF false

// Debug Logger helper func
#define log(str) \
    do { \
        if (m_Spec.Debug) { \
            std::cout << str << "\n"; \
        } \
    } while(0)