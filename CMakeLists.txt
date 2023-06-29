project(DataBaseClass)

add_executable(DataBaseClass DataBaseClass.cpp)
add_subdirectory("C:\libpqxx-7.7.5" libpqxx-build)

#set(PostgreSQL_ADDITIONAL_VERSIONS "15")

target_compile_features(DataBaseClass PRIVATE cxx_std_20)
target_link_libraries(DataBaseClass pqxx)
