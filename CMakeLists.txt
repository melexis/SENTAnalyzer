cmake_minimum_required (VERSION 3.11)
project(SENT_analyzer)

# custom CMake Modules are located in the cmake directory.
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(ExternalAnalyzerSDK)

set(SOURCES
src/SENTAnalyzer.cpp
src/SENTAnalyzer.h
src/SENTAnalyzerResults.cpp
src/SENTAnalyzerResults.h
src/SENTAnalyzerSettings.cpp
src/SENTAnalyzerSettings.h
src/SENTSimulationDataGenerator.cpp
src/SENTSimulationDataGenerator.h
)

add_analyzer_plugin(SENT_analyzer SOURCES ${SOURCES})
