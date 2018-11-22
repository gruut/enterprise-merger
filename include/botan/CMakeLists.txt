cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 11)

file(GLOB SOURCE_FILES
        botan_all_*.cpp
        )
file(GLOB HEADER_FILES
        botan_all_*.h
        )
add_library(botan_all "")

target_sources(botan_all
        PRIVATE
        ${SOURCE_FILES}
        PUBLIC
        ${HEADER_FILES}
        )
