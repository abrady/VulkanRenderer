# CMakeList.txt : CMake project for vulkan5, include source and define
# WARNING: in certain cases you may need to re-run CMake: Configure (or from the command line do cmake -B build -S vulkan5)
#          e.g. when I set the generatore platform. 
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
include_directories("${VULKAN_UTILS}\\glm")
include_directories("${VULKAN_UTILS}\\stb")
include_directories("${VULKAN_UTILS}\\tinyobjloader")

include_directories("$ENV{VK_SDK_PATH}/include")
add_compile_options(-W3)

set(TARGET_DIR "${CMAKE_BINARY_DIR}/Debug") # TODO: configure so it will work for release builds too

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
        COMMENT "build models"
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
        COMMENT "build textures"
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

# Set the source and destination directories
set(SHADER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/Source/Shaders")
set(SHADER_DEST_DIR "${TARGET_DIR}/Assets/Shaders") 

# Create the destination directory if it doesn't exist
file(MAKE_DIRECTORY ${SHADER_DEST_DIR})

# Compile vertex shaders
file(MAKE_DIRECTORY ${SHADER_DEST_DIR}/Vert)
file(GLOB_RECURSE VERTEX_SHADERS CONFIGURE_DEPENDS "${SHADER_SOURCE_DIR}/vert/*")
foreach(SHADER ${VERTEX_SHADERS})
    get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
    set(SHADER_OUTPUT "${SHADER_DEST_DIR}/vert/${SHADER_NAME}.spv")
    add_custom_command(
        OUTPUT "${SHADER_OUTPUT}"
        COMMAND glslc "${SHADER}" -o "${SHADER_OUTPUT}"
        DEPENDS ${SHADER}
        COMMENT "building vertex shader ${SHADER}"
    )
    list(APPEND COMPILED_SHADERS "${SHADER_OUTPUT}")
endforeach()
# Compile fragment shaders
file(MAKE_DIRECTORY ${SHADER_DEST_DIR}/Frag)
file(GLOB_RECURSE FRAGMENT_SHADERS CONFIGURE_DEPENDS "${SHADER_SOURCE_DIR}/frag/*")
foreach(SHADER ${FRAGMENT_SHADERS})
    get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
    set(SHADER_OUTPUT "${SHADER_DEST_DIR}/frag/${SHADER_NAME}.spv")
    add_custom_command(
        OUTPUT "${SHADER_OUTPUT}"
        COMMAND glslc "${SHADER}" -o "${SHADER_OUTPUT}"
        DEPENDS ${SHADER}
        COMMENT "building fragment shader ${SHADER}"
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
    "Source/Vulk/VulkPipelineBuilder.cpp"
)

add_executable (VulkanRenderer "Source/main.cpp" ${VULKAN_RENDERER_SOURCES})
if(MSVC)
  target_compile_options(VulkanRenderer PRIVATE /W4 /WX)
else()
  target_compile_options(VulkanRenderer PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

add_dependencies(VulkanRenderer compile_shaders)
add_dependencies(VulkanRenderer compile_models)
add_dependencies(VulkanRenderer compile_textures)
target_link_libraries(VulkanRenderer vulkan-1)
target_link_libraries(VulkanRenderer glfw3)


# TODO: Add tests and install targets if needed.
