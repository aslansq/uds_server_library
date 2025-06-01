cmake_minimum_required(VERSION 3.10)

# Enable compile commands
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set the project name
project(seednkey)

# Add the shared library
add_library(seednkey SHARED seednkey.c)