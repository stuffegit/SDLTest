cmake_minimum_required(VERSION 3.20)

project(SDLTest)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(SDL_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
set(SDL_DISABLE_UNINSTALL ON CACHE BOOL "" FORCE)

set(CORE_PATH     "src/core")
set(GRAPHICS_PATH "src/graphics")
set(UI_PATH       "src/ui")

option(SDL_SHARED "" OFF)
option(SDL_STATIC "" ON)
option(SDL_TEST_LIBRARY "" OFF)
option(SDL_TESTS "" OFF)
add_subdirectory(lib/SDL EXCLUDE_FROM_ALL)

set(GLM_ENABLE_CXX_20 ON CACHE BOOL "" FORCE)
add_subdirectory(lib/glm EXCLUDE_FROM_ALL)
add_subdirectory(lib/tinyobjloader EXCLUDE_FROM_ALL)

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/lib/imgui)
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdlgpu3.cpp
)
target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)
target_link_libraries(imgui PUBLIC SDL3::SDL3)

add_executable(SDLTest
  src/main.cpp
  ${CORE_PATH}/app.cpp
  ${CORE_PATH}/settings.cpp
  ${CORE_PATH}/test_map.cpp
  ${GRAPHICS_PATH}/window.cpp
  ${GRAPHICS_PATH}/gpu_context.cpp
  ${GRAPHICS_PATH}/renderer.cpp
  ${GRAPHICS_PATH}/mesh.cpp
  ${UI_PATH}/debug_ui.cpp
)

target_include_directories(SDLTest PRIVATE
  ${CORE_PATH}
  ${GRAPHICS_PATH}
  ${UI_PATH}
)
target_include_directories(SDLTest SYSTEM PRIVATE
  ${CMAKE_SOURCE_DIR}/lib/json/single_include
)

target_link_libraries(SDLTest PRIVATE SDL3::SDL3 glm::glm tinyobjloader imgui)

target_compile_definitions(SDLTest PRIVATE
  GLM_FORCE_DEPTH_ZERO_TO_ONE
  GLM_FORCE_RADIANS
)

if(MSVC)
  target_compile_options(SDLTest PRIVATE /W4 /permissive-)
else()
  target_compile_options(SDLTest PRIVATE
    -Wall -Wextra -Wpedantic
    -Wshadow -Wconversion -Wsign-conversion
  )
endif()

# ---------------------------------------------------------------------------
# Shader compilation (requires glslc from the Vulkan SDK)
# ---------------------------------------------------------------------------
find_program(GLSLC glslc)
if(NOT GLSLC)
  message(FATAL_ERROR
    "glslc not found.\n"
    "Install the Vulkan SDK: https://vulkan.lunarg.com/sdk/home\n"
    "Then re-run cmake.")
endif()

set(SHADER_SRC_DIR ${CMAKE_SOURCE_DIR}/assets/shaders)
set(SHADER_OUT_DIR ${CMAKE_BINARY_DIR}/assets/shaders)
file(MAKE_DIRECTORY ${SHADER_OUT_DIR})

set(SHADER_SOURCES
  mesh.vert
  mesh.frag
)

set(SPIRV_OUTPUTS)
foreach(SHADER ${SHADER_SOURCES})
  set(INPUT  ${SHADER_SRC_DIR}/${SHADER})
  set(OUTPUT ${SHADER_OUT_DIR}/${SHADER}.spv)
  add_custom_command(
    OUTPUT  ${OUTPUT}
    COMMAND ${GLSLC} ${INPUT} -o ${OUTPUT}
    DEPENDS ${INPUT}
    COMMENT "Compiling shader: ${SHADER}"
    VERBATIM
  )
  list(APPEND SPIRV_OUTPUTS ${OUTPUT})
endforeach()

add_custom_target(Shaders DEPENDS ${SPIRV_OUTPUTS})

add_custom_target(CopyMeshes
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets/meshes
    ${CMAKE_BINARY_DIR}/assets/meshes
  COMMENT "Copying mesh assets"
)

add_dependencies(SDLTest Shaders CopyMeshes)
