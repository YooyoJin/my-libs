cmake_minimum_required(VERSION 3.15)
project(my-libs-examples C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ============ 模块库登记处(每加一个模块在这里加一段) ============
add_library(fanctrl STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../source/modules/fanctrl/fanctrl.c
)
target_include_directories(fanctrl PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../source/modules/fanctrl
)
target_compile_options(fanctrl PRIVATE -Wall -Wextra)  # 库本体保持零告警

# 将来: add_library(pidcore STATIC .../source/modules/pidcore/pidcore.c)
# 将来: target_link_libraries(fanctrl PUBLIC pidcore)

# ============ 演示目标 ============
add_subdirectory(pcdemo)
