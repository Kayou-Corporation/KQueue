# --------------------------------------- #
#               CMAKE CONFIG              #
# --------------------------------------- #

# Define minimum CMake version required
cmake_minimum_required(VERSION 3.30.0)



project(Queue VERSION 1.0.0 LANGUAGES CXX)



#---------------------------------------#
#            BUILD MODE                 #
#---------------------------------------#

add_executable(KQUEUE_TEST "Main.cpp")



#---------------------------------------#
#                COMPILE MODE           #
#---------------------------------------#
target_compile_features(KQUEUE_TEST PUBLIC cxx_std_20)

if(MSVC)
    set(CMAKE_DEBUG_POSTFIX "d")
    target_compile_options(KQUEUE_TEST PUBLIC /W3 /WX)
else()
    target_compile_options(KQUEUE_TEST PUBLIC -Wall -Wextra -Wpedantic -Werror)
endif()



# Build position independent code.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Disable C and C++ compiler extensions.
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_EXTENSIONS OFF)
