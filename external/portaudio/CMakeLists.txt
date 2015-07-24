cmake_minimum_required(VERSION 3.1.0)
project(portaudio)

add_library(portaudio
  pa_memorybarrier.h
  pa_ringbuffer.cpp
  pa_ringbuffer.h
  )
target_include_directories(portaudio PUBLIC .)
