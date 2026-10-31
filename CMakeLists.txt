cmake_minimum_required(VERSION 3.16)
project(SubscriptionManagementSystem CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DNOMINMAX -DWIN32_LEAN_AND_MEAN -D_CRT_SECURE_NO_WARNINGS)

# MySQL Detection
set(MYSQL_DEFAULT_INCLUDE "C:/Program Files/MySQL/MySQL Server 8.0/include")
set(MYSQL_DEFAULT_LIB "C:/Program Files/MySQL/MySQL Server 8.0/lib/libmysql.lib")

if(DEFINED ENV{MYSQL_DIR})
    set(MYSQL_INCLUDE_DIR "$ENV{MYSQL_DIR}/include")
    set(MYSQL_LIB_PATH "$ENV{MYSQL_DIR}/lib/libmysql.lib")
else()
    set(MYSQL_INCLUDE_DIR "${MYSQL_DEFAULT_INCLUDE}")
    set(MYSQL_LIB_PATH "${MYSQL_DEFAULT_LIB}")
endif()

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/backend
    ${CMAKE_CURRENT_SOURCE_DIR}/backend/third_party/crow
    ${CMAKE_CURRENT_SOURCE_DIR}/backend/third_party/nlohmann
    ${CMAKE_CURRENT_SOURCE_DIR}/backend/third_party/asio/include
    ${CMAKE_CURRENT_SOURCE_DIR}/backend/third_party/libbcrypt/include
    "${MYSQL_INCLUDE_DIR}"
)

set(BACKEND_SOURCES
    backend/main.cpp
    backend/database.cpp
    backend/auth.cpp
    backend/admin.cpp
    backend/customer_auth.cpp
    backend/customer.cpp
)

set(BCRYPT_SOURCES
    backend/third_party/libbcrypt/src/bcrypt.c
    backend/third_party/libbcrypt/src/crypt_blowfish.c
    backend/third_party/libbcrypt/src/crypt_gensalt.c
    backend/third_party/libbcrypt/src/wrapper.c
)

add_executable(subscription_backend
    ${BACKEND_SOURCES}
    ${BCRYPT_SOURCES}
)

target_link_libraries(subscription_backend PRIVATE
    "${MYSQL_LIB_PATH}"
    ws2_32
    crypt32
    Advapi32
)

# Optional helper test executable for offline/unit verification
add_executable(unit_tests
    backend/test_runner.cpp
    backend/database.cpp
    backend/auth.cpp
    backend/admin.cpp
    backend/customer_auth.cpp
    backend/customer.cpp
    ${BCRYPT_SOURCES}
)

target_link_libraries(unit_tests PRIVATE
    "${MYSQL_LIB_PATH}"
    ws2_32
    crypt32
    Advapi32
)
