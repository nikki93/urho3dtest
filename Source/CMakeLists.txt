# Set project name
project (urho3dtest)
# Set minimum version
cmake_minimum_required (VERSION 2.8.6)
if (COMMAND cmake_policy)
cmake_policy (SET CMP0003 NEW)
endif ()
# Set CMake modules search path
set (CMAKE_MODULE_PATH $ENV{URHO3D_HOME}/Source/CMake/Modules CACHE PATH "Path to Urho3D-specific CMake modules")
# Include Urho3D cmake module
include (Urho3D-CMake-magic)
# Find Urho3D library
find_package (Urho3D REQUIRED)
include_directories (${URHO3D_INCLUDE_DIR})
# Define target name
set (TARGET_NAME Main)
# Define source files
define_source_files ()
# Setup target with resource copying
setup_main_executable ()

