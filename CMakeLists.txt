cmake_minimum_required(VERSION 3.20)

project(GraphTraversalSimulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Point CMake to the clean, installed SFML folder
set(CMAKE_PREFIX_PATH "C:/Users/Ashraf/Documents/Libraries/SFML_Installed")

# Tell CMake we want the static versions of the libraries
set(SFML_STATIC_LIBRARIES ON)

# Find the SFML modules
find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED)

add_executable(PathVisualizer PathVisualizer.cpp)

target_link_libraries(PathVisualizer PRIVATE 
    SFML::Graphics
    SFML::Window
    SFML::System
)