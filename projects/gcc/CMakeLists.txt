cmake_minimum_required(VERSION 3.15)
project(my-libs-projects C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(SRC_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../source)

# ============ 模块库登记处(每加一个模块在这里加一段) ============
add_library(fanctrl STATIC ${SRC_ROOT}/modules/fanctrl/fanctrl.c)
target_include_directories(fanctrl PUBLIC ${SRC_ROOT}/modules/fanctrl)
target_compile_options(fanctrl PRIVATE -Wall -Wextra)  # 库本体保持零告警

# 将来: add_library(pidcore STATIC ${SRC_ROOT}/modules/pidcore/pidcore.c)

# ============ 应用目标(源码在 source/app/) ============
add_executable(fanctrldemo ${SRC_ROOT}/app/pcdemo/fanctrldemo.c)
target_include_directories(fanctrldemo PRIVATE ${SRC_ROOT}/modules/fanctrl)
target_link_libraries(fanctrldemo PRIVATE fanctrl)