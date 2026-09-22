cmake_minimum_required(VERSION 3.20)
# C is needed only because LLVMConfig.cmake's FindLibEdit module runs check_include_file(),
# which requires the C language to be enabled.
project(llvm2ct CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(LLVM 21.1 REQUIRED CONFIG HINTS /usr/lib/llvm-21/lib/cmake/llvm)
find_package(Boost REQUIRED)
list(APPEND CMAKE_MODULE_PATH ${LLVM_CMAKE_DIR})
include(AddLLVM)

include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(LLVM_LIBS core irreader support)

add_executable(llvm2ct main.cpp codegen.cpp core.cpp debuginfo.cpp mapping.cpp builtin.cpp)
target_compile_options(llvm2ct PRIVATE -Wall -Wextra -Werror -O2)
target_link_libraries(llvm2ct ${LLVM_LIBS} Boost::headers)
