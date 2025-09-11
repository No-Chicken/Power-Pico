# gui/CMakeLists.txt
file(GLOB GUI_SOURCES
    "*.cpp"
    "*.c"
    "*.cxx"
    "*.cc"
)

add_library(guilib STATIC ${GUI_SOURCES})

target_include_directories(guilib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../lvgl)
