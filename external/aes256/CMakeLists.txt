
project(aes256)

add_library(aes256
  aes256.cpp
  aes256.h
  )
target_include_directories(aes256 PUBLIC .)
