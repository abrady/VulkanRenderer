
# CMakeList.txt : CMake project for vulkan5, include source and define
# WARNING: in certain cases you may need to re-run CMake: Configure (or from the command line do cmake -B build -S vulkan5)
#          e.g. when I set the generatore platform. 
#
# cmake -DCMAKE_BUILD_TYPE=Debug - for Debug build

if(NOT CMAKE_BUILD_TYPE)
    message("CMAKE_BUILD_TYPE not set, defaulting to Debug")
    set(CMAKE_BUILD_TYPE Debug)
endif()

cmake_minimum_required (VERSION 3.8)
project ("VulkanRenderer")

set(CMAKE_GENERATOR_PLATFORM x64)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set a variable for the base path
set(VULKAN_UTILS "c:\\Vulkan")
# Use the variable in the include_directories command
include_directories("${VULKAN_UTILS}\\glfw\\include")

include_directories("${CMAKE_SOURCE_DIR}/Source")
include_directories("${CMAKE_SOURCE_DIR}/Source/ThirdParty/include")
include_directories("$ENV{VK_SDK_PATH}/include")
add_compile_options(-W3)

# TODO: configure so it will work for release builds too
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(TARGET_DIR "${CMAKE_BINARY_DIR}/Debug") 
    set(GLSLC_FLAGS "-g" "-O0" "-I${CMAKE_SOURCE_DIR}/Source/Shaders/Common") # build debug symbols and turn off optimizations
endif()

##############################################################################
## Models 
##############################################################################
set(MODEL_SOURCE_DIR "${CMAKE_SOURCE_DIR}/Assets/Models")
set(MODEL_DEST_DIR "${TARGET_DIR}/Assets/Models") 

file(MAKE_DIRECTORY ${MODEL_DEST_DIR})
file(GLOB_RECURSE MODELS CONFIGURE_DEPENDS "${MODEL_SOURCE_DIR}/*")
foreach(MODEL_FILE ${MODELS})
    # Get the path of the file relative to the source directory
    file(RELATIVE_PATH MODEL_REL_PATH "${MODEL_SOURCE_DIR}" "${MODEL_FILE}")
    # Construct the destination path
    set(DEST "${MODEL_DEST_DIR}/${MODEL_REL_PATH}")
    # Get the directory of the destination path
    get_filename_component(DEST_DIR "${DEST}" DIRECTORY)
    # Create the destination directory if it doesn't exist
    file(MAKE_DIRECTORY "${DEST_DIR}")
    # Add a custom command to copy the file
    add_custom_command(
        OUTPUT "${DEST}"
        COMMAND ${CMAKE_COMMAND} -E copy "${MODEL_FILE}" "${DEST}"
        DEPENDS "${MODEL_FILE}"
        COMMENT "build models ${MODEL_FILE} to ${DEST}"
    )
    # Add the output file to a list
    list(APPEND COMPILED_MODELS "${DEST}")
endforeach()

# Add a custom target to trigger the shader compilation
add_custom_target(
    compile_models
    DEPENDS ${COMPILED_MODELS}
)



##############################################################################
## Textures
##############################################################################

set(TEXTURES_SOURCE_DIR "${CMAKE_SOURCE_DIR}/Assets/Textures")
set(TEXTURES_DEST_DIR "${TARGET_DIR}/Assets/Textures") 

file(MAKE_DIRECTORY ${TEXTURES_DEST_DIR})
file(GLOB_RECURSE TEXTURES CONFIGURE_DEPENDS "${TEXTURES_SOURCE_DIR}/*")
foreach(TEXTURES_FILE ${TEXTURES})
    # Get the path of the file relative to the source directory
    file(RELATIVE_PATH TEXTURES_REL_PATH "${TEXTURES_SOURCE_DIR}" "${TEXTURES_FILE}")
    # Construct the destination path
    set(DEST "${TEXTURES_DEST_DIR}/${TEXTURES_REL_PATH}")
    # Get the directory of the destination path
    get_filename_component(DEST_DIR "${DEST}" DIRECTORY)
    # Create the destination directory if it doesn't exist
    file(MAKE_DIRECTORY "${DEST_DIR}")
    # Add a custom command to copy the file
    add_custom_command(
        OUTPUT "${DEST}"
        COMMAND ${CMAKE_COMMAND} -E copy "${TEXTURES_FILE}" "${DEST}"
        DEPENDS "${TEXTURES_FILE}"
        COMMENT "build texture ${TEXTURES_FILE}"
    )
    # Add the output file to a list
    list(APPEND COMPILED_TEXTURES "${DEST}")
endforeach()

# Add a custom target to trigger the shader compilation
add_custom_target(
    compile_textures
    DEPENDS ${COMPILED_TEXTURES}
)


##############################################################################
## Shaders
##############################################################################

# TODO: glslc has a -M option that will print a makefile dependency, so we could use that instead of globbing
file(GLOB SHADER_COMMON_FILES "${CMAKE_SOURCE_DIR}/Source/Shaders/Common/*")

# Set the source and destination directories
set(SHADER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/Source/Shaders")
set(SHADER_DEST_DIR "${TARGET_DIR}/Source/Shaders") 

# Create the destination directory if it doesn't exist
file(MAKE_DIRECTORY ${SHADER_DEST_DIR})

# Compile vertex shaders
file(MAKE_DIRECTORY ${SHADER_DEST_DIR}/Vert)
file(GLOB_RECURSE VERTEX_SHADERS CONFIGURE_DEPENDS "${SHADER_SOURCE_DIR}/Vert/*")
foreach(SHADER ${VERTEX_SHADERS})
    get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
    set(SHADER_OUTPUT "${SHADER_DEST_DIR}/Vert/${SHADER_NAME}.spv")
    add_custom_command(
        OUTPUT "${SHADER_OUTPUT}"
        COMMAND glslc ${GLSLC_FLAGS} "${SHADER}" -o "${SHADER_OUTPUT}"
        DEPENDS ${SHADER} ${SHADER_COMMON_FILES}
        COMMENT "building vertex shader ${SHADER}"
    )
    list(APPEND COMPILED_SHADERS "${SHADER_OUTPUT}")
endforeach()
# Compile fragment shaders
file(MAKE_DIRECTORY ${SHADER_DEST_DIR}/Frag)
file(GLOB_RECURSE FRAGMENT_SHADERS CONFIGURE_DEPENDS "${SHADER_SOURCE_DIR}/Frag/*")
foreach(SHADER ${FRAGMENT_SHADERS})
    get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
    set(SHADER_OUTPUT "${SHADER_DEST_DIR}/Frag/${SHADER_NAME}.spv")
    add_custom_command(
        OUTPUT "${SHADER_OUTPUT}"
        COMMAND glslc ${GLSLC_FLAGS} "${SHADER}" -o "${SHADER_OUTPUT}"
        DEPENDS ${SHADER} ${SHADER_COMMON_FILES}
        COMMENT "building fragment shader ${SHADER}"
    )
    list(APPEND COMPILED_SHADERS "${SHADER_OUTPUT}")
endforeach()

# Compile Geometry shaders
file(MAKE_DIRECTORY ${SHADER_DEST_DIR}/Tess)
file(GLOB_RECURSE GEOMETRY_SHADERS CONFIGURE_DEPENDS "${SHADER_SOURCE_DIR}/Geom/*")
foreach(SHADER ${GEOMETRY_SHADERS})
    get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
    set(SHADER_OUTPUT "${SHADER_DEST_DIR}/Geom/${SHADER_NAME}.spv")
    add_custom_command(
        OUTPUT "${SHADER_OUTPUT}"
        COMMAND glslc ${GLSLC_FLAGS} "${SHADER}" -o "${SHADER_OUTPUT}"
        DEPENDS ${SHADER} ${SHADER_COMMON_FILES}
        COMMENT "building geometry shader ${SHADER}"
    )
    list(APPEND COMPILED_SHADERS "${SHADER_OUTPUT}")
endforeach()

# Add a custom target to trigger the shader compilation
add_custom_target(
    compile_shaders 
    DEPENDS ${COMPILED_SHADERS}
)

##############################################################################
## Executable
##############################################################################

# Add source to this project's executable.
link_directories("$ENV{VK_SDK_PATH}\\Lib")
link_directories("C:\\Vulkan\\glfw\\build\\src\\Debug")

set(VULKAN_RENDERER_SOURCES
    "Source/Vulk/Vulk.cpp"
    "Source/Vulk/VulkGeo.cpp"
    "Source/Vulk/VulkUtil.cpp"
    "Source/Vulk/VulkMesh.cpp"
    "Source/Vulk/VulkActor.cpp"
    "Source/Vulk/VulkCamera.cpp"
    "Source/Vulk/VulkDescriptorSetLayoutBuilder.cpp"
    "Source/Vulk/VulkDescriptorSetUpdater.cpp"
    "Source/Vulk/VulkPipelineBuilder.cpp"
    "Source/Vulk/VulkResources.cpp"
    "Source/Vulk/VulkTextureView.cpp"
)


add_executable (VulkanRenderer "Source/main.cpp" ${VULKAN_RENDERER_SOURCES})
if(MSVC)
  set(MY_PROJECT_NATVIS_FILES "${CMAKE_SOURCE_DIR}/Source/Tools/MSVC/glm.natvis")
  target_sources(VulkanRenderer PRIVATE ${MY_PROJECT_NATVIS_FILES})
  # /analyze - /analyze:warninglevel 4
  # /fsanitize=address /fsanitize=fuzzer
  # /sdl - Enables recommended Security Development Lifecycle (SDL) checks
  target_compile_options(VulkanRenderer PRIVATE /W4 /WX /sdl)
  add_compile_options(/fsanitize=address)
else()
  target_compile_options(VulkanRenderer PRIVATE -Wall -Wextra -pedantic -Werror)
  add_compile_options(-sanitize=address)
endif()

add_dependencies(VulkanRenderer compile_shaders)
add_dependencies(VulkanRenderer compile_models)
add_dependencies(VulkanRenderer compile_textures)
target_link_libraries(VulkanRenderer vulkan-1)
target_link_libraries(VulkanRenderer glfw3)


# TODO: Add tests and install targets if needed.
