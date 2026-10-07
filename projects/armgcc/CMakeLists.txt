cmake_minimum_required(VERSION 3.15)
project(my-libs-armgcc C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(SRC_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../source)

# ============ 模块库登记处(与 projects/gcc 保持同步) ============
add_library(fanctrl STATIC ${SRC_ROOT}/modules/fanctrl/fanctrl.c)
target_include_directories(fanctrl PUBLIC ${SRC_ROOT}/modules/fanctrl)
target_compile_options(fanctrl PRIVATE -Wall -Wextra)  # 库本体保持零告警

# 将来(配好 HC32 SDK 后): 增加 hc32f460demo 固件目标
# add_executable(hc32f460demo ${SRC_ROOT}/app/hc32f460demo/main.c ...)
# target_link_libraries(hc32f460demo PRIVATE fanctrl)
