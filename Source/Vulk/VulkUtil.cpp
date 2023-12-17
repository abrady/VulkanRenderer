
// The following line is needed to make the implementation file for the header file
// i.e. the body of this functions will be defined in this file while other uses of the header
// will just declare the functions

#pragma warning(push, 0) // assume these headers know what they're doing

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#pragma warning(pop)