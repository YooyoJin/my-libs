add_executable(fanctrldemo fanctrldemo.c)

target_include_directories(fanctrldemo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../source/modules/fanctrl
)
target_link_libraries(fanctrldemo PRIVATE fanctrl)
