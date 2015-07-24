cmake_minimum_required(VERSION 3.1.0)
project(cJSON)

add_library(cjson
  cJSON.c
  cJSON.h
  )
target_include_directories(cjson PUBLIC .)
