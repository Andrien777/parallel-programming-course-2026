cmake_minimum_required(VERSION 4.3)
project(parallel_programming_course_2026)

set(CMAKE_CXX_STANDARD 20)

include_directories(lab1/src)

add_executable(parallel_programming_course_2026
        lab1/src/MetricsCollector.h
        lab1/src/SingleThreadedCollector.cpp
        lab1/src/SingleThreadedCollector.h
        lab1/src/MetricsCollector.cpp
        lab1/src/main.cpp
        lab1/src/SingleMutexCollector.cpp
        lab1/src/SingleMutexCollector.h
        lab1/src/EmptyLockCollector.cpp
        lab1/src/EmptyLockCollector.h
        lab1/src/ShardedCollector.cpp
        lab1/src/ShardedCollector.h
        lab1/src/ThreadLocalCollector.cpp
        lab1/src/ThreadLocalCollector.h
        lab1/src/DoubleBufferedCollector.cpp
        lab1/src/DoubleBufferedCollector.h
        lab1/src/inline_helpers.h
        lab1/src/BrokenDoubleBufferedCollector.cpp
        lab1/src/BrokenDoubleBufferedCollector.h)
