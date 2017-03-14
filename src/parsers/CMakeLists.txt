
cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
project(k::parser)

enable_testing()

include_directories("include")
add_library(kscene_parser "src/scene_parser.cpp")
# target_link_libraries(kscene_parser die-xml)

add_executable(kscene_parser_test "test/scene_parser_test.cpp")
add_test(kscene_parser kscene_parser_test)
