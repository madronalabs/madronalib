cmake_minimum_required(VERSION 2.8.12)
project(portaudio)

add_library(portaudio
  pa_memorybarrier.h
  pa_ringbuffer.cpp
  pa_ringbuffer.h
  )
target_include_directories(portaudio PUBLIC .)
