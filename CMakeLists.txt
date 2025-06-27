cmake_minimum_required(VERSION 3.10)
project(lurkuitae_terminal)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Ajouter les fichiers sources
set(SOURCES
    main.cpp
    core/ollama_interface.cpp
    core/memory.cpp
    core/system_handler.cpp
)

add_executable(lurkuitae_terminal ${SOURCES})
