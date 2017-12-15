project(pautil)

add_library(pautil
  pa_memorybarrier.h
  pa_ringbuffer.cpp
  pa_ringbuffer.h
  )

target_include_directories(pautil PUBLIC .)
