# VulkanRenderer

This is a series of adventures to try to learn how to make a renderer. 

I'm basically following Introduction to 3D Game Programming with DirectX 12 by Frank Luna which was recommended to me by a friend as a great guide (and so far, it is!)

Some quick thoughts on my approach: one thing you might notice is that I avoid wrapping a lot of the vulkan boilerplate in functions and structs for many of the samples. this makes for verbose code that can seem harder to understand at first glance, however, in my experience, premature abstractions both end up making code more confusing by hiding what is actually going on (i.e. what assumptions the abstraction made) and undermine the goal of code like this which is to understand how Vulkan works. I'm writing this note after six days of effort and my biggest regrets so far have all been premature encapsulation and the subsequent ex-capsulation. 

...

# Samples
1. [Viking Room](/Source/Samples/VikingRoom.h) - loads a model and renders it
2. [Shape Geometry](/Source/Samples/ShapeGeometry.h)
3. [Scene](/Source/Samples/Scene.h) - a scene with a floor and some cylinders and spheres, uses instancing
4. [Land and Waves](/Source/Samples/LandAndWaves.h) - a procedurally generated terrain example
5. [Lights](/Source/Samples/Lighting.h) - basic lighting
6. [LitLandAndWaves](/Source/Samples/LitLandAndWaves.h) - lighting applied to the land and waves scene
7. [TexturedScene](/Source/Samples/TexturedScene.h) - blended textures applied to the land and waves scene, with lighting.
8. [Blending](/Source/Samples/Blending.h)
9. [Outline With Stencil](/Source/Samples/OutlineWorld.h) - using the stencil buffer to make an outline

# Resources
* [Vulkan Tutorial](https://vulkan-tutorial.com/Introduction)

# Building
* Clone the repo

Install the following. Note that CmakeLists.txt assumes these are in C:\Vulkan:
* Install [Vulkan SDK](https://vulkan.lunarg.com/sdk/home)
* Install [GLFW](https://www.glfw.org/)
* Install [GLM](https://glm.g-truc.net/0.9.9/index.html)
* Install [TinyObjLoader](https://github.com/tinyobjloader/tinyobjloader)
* Install [STB](https://github.com/nothings/stb)

* make sure your CMakeLists.txt points to the proper directory
* run `cmake -S . -B build` from the root of the project

# TODOs
* document some of the annoying things I'm finally getting around to coding around.
* VulkPipelineBuilder should use a set for stages and error if they already exist...
* resources next steps: generate a manifest during the build and load that.

* need per-actor materials:
    * x update descriptors set layout
    * x make sure the pool has enough to allocate
    * x update the descriptor set

# Log

## 1/27/24 reflection rendered too
![](Assets/Screenshots/reflected_skull.png)

Okay, what did I learn?
* 

## 1/27/24 mirrored skull rendered!
![](Assets/Screenshots/mirrored_skull0.png)

fixed the nan and now the mirrored skull is showing up, yay! need to fix the lighting and do the rest of it now

## 1/27/24 natvis pretty variable debugging
getting nans in my reflected mesh. glm vecs are a pain in the ass to debug because they have union'd x,y,z with two other things, can I visualize this better?

Hey! MSVC support .natvis files, and someone put one up for glm already! how do I add them?

  set(MY_PROJECT_NATVIS_FILES "${CMAKE_SOURCE_DIR}/Source/Tools/MSVC/glm.natvis")
  target_sources(VulkanRenderer PRIVATE ${MY_PROJECT_NATVIS_FILES})

we're rolling, nice!
![](Assets/Screenshots/glm_natvis.png)


## 1/24/24 reflection
oof, it's been a while. 

To reflect:
1. render skull and everything but the mirror as normal
2. clear the stencil buffer
3. render only the mirror to the stencil buffer
4. render the reflected skull only where stencil is set
5. render the mirror with transparency blending

Let's see what sort of mess I left for myself with that last refactor:
1. the renderer has three main parts:
    1. resource loading: shaders, lights, cameras, materials, textures, meshes
    2. render init: 
        1. create global buffers (mesh, texture views/UBOs/SBOs), 
        2. create the descriptor set layout etc.
        3. bind the UBOs/SBOs and texture views to the descriptor set
        3. create the pipeline and vert/index buffers
    3. actual rendering: bind pipeline, descriptor sets, index/vert 


### Reflection pondering:
Reflecting across the x axis at origin is just -x..., e.g. if I have P = (x,y), then P', the reflected point = (-x,y)

If I think of a plane as having a normal N, then reflection seems to be moving a point in the direction of the plane twice the distance it is from the plane?
* N = (1,0)
* P = (5,2)
* P' = (-5,2), or = P - 10*N  // notice we have 10 which is twice 5: the first moves it to origin, the second to the reflection

More generally, let d be the distance from the plane, then if we have a function 'dist(N,P)' it will return d, the scalar distance from the plane in the direction opposite the normal.
* dist(N,P) = N.P
    * assuming the plane passes through origin.
    * note: if this is negative the point is behind the plane.

We can generalize to any plane with normal N and a point on the plane D:
* dist(N,D,P) = N.(P-D)
    * assuming N is normalized, which it should be

So reflecting a point across an arbitrary plain is really straightforward:
* Given: 
    * a plane (a point D on the plain and a normal N) and a point P
* you can reflect the point across that plane by moving it twice the distance in the direction of the plane:
    * P' = P - 2*N.(P-D)


## 1/14/23 simplified initialization thought experiment
![](Assets/Screenshots/init_render_deps.png)

I've had a todo for a while to get rid of some of the boilerplate for initializing the render. I made this diagram to help. what it shows me is one approach for simplifying:
1. mesh load amd buffers, texture loads, and 'global' buffer creation can happen first.
2. updating the descriptor set needs some of these globals, so it can init second
3. then the pipeline takes shaders and a descriptor set layout, so it happens after 

conceptually this fits what I believe a lot of engines do which is:
* load the resources needed for rendering. this includes shaders even though not needed until step 3 I suppose.
* init the rendering part

I'd like to thank the academy for the award for pointing out the obvious. 

Now let's wrap this up:

### 1. mesh, texture loads, and 'global' buffer creation
For now I'll stick with the VulkTextureLoader and VulkUniformBuffer, those seem okay, maybe eventually a 'resource manager' class will be good.

I'm not super happy with mesh loading and vert buffer handling. I feel like I keep going in circles on them. What is the problem I'm trying to solve:
* init is basically fine. pipelines are relatively complicated, as are descriptor sets etc. defining this for a thing, like an 'actor' is something but also hides things
* rendering is slightly tedious, for a given set of vertices you have a pipeline, descriptorset, verts, indices, and offset/size for rendering
    * maybe the problem is multiple models that have the same vert/index buffers?

### 2. updating the descriptor sets
This part has gotten really tedious, especially the pool size tracking. I'm going to make a builder that tracks all the information around descriptors so we can simplify this.

VulkPipelineBuilder is pretty solid, so that should be fine. 

## 1/11/24 stenciled outline of skull behind wall

![](Assets/Screenshots/skull_behind_wall_outline.png)

here's a quick example of the stenciling showing the skull behind a wall a'la any game that shows other players or enemy outlines. pretty straightforward:
1. render the skull with stencil write on passing pixels
2. render the wall
3. render the skull 1% larger with stencil set to accept only failing tests and with depth buffer turned off

## 1/11/24
fixed the bad looking skull! tl;dr: load/store ops matter! 

    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;

So having the depth/stencil attachment set to "don't care" seems to mean that, even if you call clear, the driver is free to optimize this away because you're clearing a buffer you said you don't care about. 

Instead you have to set it to clear, just like depth:

    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

## 1/11/24 first stencil: outline
The OutlineWorld sample works by rendering the model twice:
1. the first time it writes the stencil buffer for each pixel affected by the model
2. the second time it renders the model slightly larger (10%) and only writes stencil pixels that have failed comparison

![](Assets/Screenshots/stencil_outline_working.png)

here's the working example

![](Assets/Screenshots/stencile_outline_artifacts.png)

when I move the skull around, however, it doesn't work. I need to figure out what's going on here (probably not clearing properly), a problem for another day. goodnight!


## 1/8/24 skull load with normal texture
I downloaded a model from [Turbo Squid](https://www.turbosquid.com/3d-models/nextgen-skull-3d-model/552621)

![](Assets/Screenshots/skull_normals.png)

The interesting thing I did here was to use the normal maps texture for the lighting:

    vec4 tex = texture(texSampler, fragTexCoord);
    vec3 norm = vec3(texture(normSampler, fragTexCoord));

![](Assets/Screenshots/skull_no_normals.png)

here's what it looked like before, for comparison

## 1/7/24 Stenciling: let's make a mirror

sez chatgpt: 

    Stencil buffers are a powerful tool in computer graphics used in various rendering techniques to control the drawing of pixels on the screen. They work alongside the color and depth buffers and are particularly useful when you need to restrict rendering to specific parts of the screen. Here's how they work and what they're typically used for...

Let's make a mirror:
* load a skull model
* make a mirror


### 1. What is a Stencil Buffer?
A stencil buffer is an array of per-pixel information, typically 8 bits per pixel, that stores an integer (stencil value) for every pixel on the screen. It's used in conjunction with the depth and color buffers to add another level of control over which pixels should be updated.

### 2. How Stencil Buffers Work:
Initialization: First, the stencil buffer is cleared to a known value, often 0, at the start of the frame.
Stencil Test: When rendering, the stencil value for each pixel is tested against a specified reference value using a specified stencil function (e.g., less than, equal to). The outcome of this test determines whether the pixel will be drawn.
Stencil Operations: Depending on whether the stencil test (and the depth test) passes or fails, different operations can be performed on the stencil buffer's value, such as keeping the current value, replacing it with a new one, incrementing or decrementing it, etc.

### 3. Uses of Stencil Buffers:
Clipping Regions: By rendering shapes into the stencil buffer, you can define regions of the screen where rendering is allowed or disallowed, effectively clipping any subsequent draws to these regions.
Shadows: In shadow volume techniques, stencil buffers can help determine which areas of the screen are in shadow by incrementing and decrementing stencil values as the shadow volumes are rendered.
Outline Effects: Stencil buffers can be used to create outline effects around objects. By rendering an object into the stencil buffer and then rendering a slightly larger version behind it with only the pixels not matching the stencil value, you create an outline.
Mirror Reflections: For rendering reflections on a mirror-like surface, you can use the stencil buffer to mask out the area of the reflection.
Multi-Pass Rendering: In complex rendering techniques where multiple passes are needed, the stencil buffer can help keep track of different stages or areas that need specific rendering effects.



## 1/6/24 Clipping a chain link fence and tiles
Let's try to do a clipping shader that culls pixels if they are below a certain alpha, this is useful because it lets you not have to worry about render order for blending alpha components

Here's the chain-link fence rendered on top of the mesh.

![](Assets/Screenshots/clipped_fence.png)

and this shows the clipped version. it uses two textures:
* one for the opacity
* one for the color

I'm not super happy with how we're checking for opacity but this is fine for now. ideally I'd make a buffer with a single alpha value or something.



## 1/6/24 Blending
Blending is a process that combines the color of a new pixel (source) with the color of the pixel already in the framebuffer (destination) to produce a final color. This technique is commonly used for effects like transparency, antialiasing, and light accumulation.

In Vulkan, blending is configured per framebuffer attachment via the VkPipelineColorBlendAttachmentState structure. Here's a brief overview of the key components:
* Enable/Disable Blending: You can enable or disable blending for each attachment. If disabled, the new pixel (source) color overwrites the existing pixel (destination) color.
* Blend Factors: These factors determine how the source and destination colors are weighted. Vulkan provides various options like VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, and more. You can set different factors for color and alpha components.
* Blend Operations: This specifies how the weighted source and destination colors are combined. Common operations include add (VK_BLEND_OP_ADD), subtract, and reverse subtract.
* Color Write Mask: This controls which color components (R, G, B, A) are actually passed to the framebuffer. For example, you might write only the alpha component to create a mask.
* Pipeline Configuration: Blending is set up when you create the graphics pipeline. You’ll specify the blend state for each of the framebuffers you're rendering to.

tl;dr - final color is typically calculated using an equation like: FinalColor = (SrcColor * SrcFactor) op (DstColor * DstFactor) where op is the blend operation, and the factors are what you’ve set in the blend state.

I already did this for the water, but I can't say the transparency looked that great, let me see if playing around with these settings make it look better.

![](Assets/Screenshots/water_blend0.png)


Here's the starting blending. it has the following settings:

        srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendOp = VK_BLEND_OP_ADD;
        srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        alphaBlendOp = VK_BLEND_OP_ADD;

I just copied this from chatpgpt, but here's my interpretation of this:
1. the final color (excluding alpha) is (SrcColor * SrcAlpha) + (DstColor * (1 - SrcAlpha)) : this is basically a linear blend and conserves the energy in the system.
2. the final alpha is (SrcAlpha * 1) + (DstAlpha * 0) = SrcAlpha : so the source material's alpha becomes the final alpha

The final alpha is important because it could affect future transparent blends depending on the blend operation.




## 1/6/24 uv transforms: make the water move


* passing in a mat3 ssbo
* hmm, its getting corrupted somehow, probably alignment. I should figure this out.
* looks like 'layout qualifiers' might be the answer (or just using a mat4, but let's see if we can make it work)

### [4.4. Layout Qualifiers](https://registry.khronos.org/OpenGL/specs/gl/GLSLangSpec.4.60.html#layout-qualifiers)

    Layout qualifiers can appear in several forms of declaration. They can appear as part of an interface block definition or block member, as shown in the grammar in the previous section. They can also appear with just an interface-qualifier to establish layouts of other declarations made with that qualifier:

The qualifiers are:
- **`std140` & `std430`:**
  - **`std140`:** Used with uniform blocks with stricter alignment rules.
    - Vectors are aligned to 16 bytes.
    - Each element in an array of `N` elements is aligned to `16 * N` bytes.
    - Scalars and Vectors: Each element (scalar or vector) is aligned to the size of a vec4 (16 bytes). This means even a float (which is 4 bytes) will start at a 16-byte boundary.
    - Matrices: Each column of a matrix is treated like a vec4 for alignment purposes. For example, in a mat4, each column is aligned to 16 bytes.
    - Arrays: Each element is aligned to the base alignment of the type. For example, an array of vec3s will have each vec3 aligned like a vec4.
    - Due to its alignment rules, std140 can introduce a significant amount of padding to ensure each element meets the alignment requirements.
  - **`std430`:** Used with shader storage blocks for more efficient memory usage.
    - Vectors have natural alignment.
    - Arrays are tightly packed but respect the base alignment of their elements.
    - Scalars and Vectors: They are aligned to their component size. For example, a vec3 of floats is aligned to 12 bytes (3x4 bytes) rather than 16 bytes. A single float is aligned to 4 bytes.
    - Matrices: By default, matrices in std430 are treated as arrays of their column vectors, and each column vector is aligned according to the rules for a vector above. However, you can use row_major or column_major qualifiers to change the layout.
    - Arrays: The alignment of an array is the same as the alignment of its element type, but the stride (the space from one element to the next) is rounded up to the size of a vec4 if the element size is less than the size of a vec4.
- **`shared` & `packed`:**
  - **`shared`:** Default layout, compiler determines the best layout. **DEFAULT**
  - **`packed`:** Tightly packs variables within a block without unnecessary padding. 

- **`row_major` & `column_major`:**
  - **`row_major`:** Indicates matrices are stored row by row.
  - **`column_major`:** Indicates matrices are stored column by column (default). **DEFAULT**

- **`binding`:**
  - Specifies the binding point for a uniform block or shader storage block.

- **`location`:**
  - Specifies the location index of an input or output variable in the shader.

- **`offset`:**
  - Specifies the byte offset of a variable within a uniform or shader storage block.

- **`early_fragment_tests`:**
  - Used in fragment shaders to indicate depth and stencil testing should be performed before execution.

Nutshell:
* shared: allows glslc to determine the layout, can have cross-platform consequences
* std140: scalars and vectors aligned to vec4. stable and portable packing, but can waste space in arrays
* std430: scalars and vectors are aligned to their size (up to the size of vec4), and arrays have elements aligned to the size of the element type but with a stride rounded up to the size of a vec4 

Example: In std140, an array of 3 floats would be aligned like this: [float (16 bytes), float (16 bytes), float (16 bytes)], totaling 48 bytes. In std430, it would be more like [float (4 bytes), float (4 bytes), float (4 bytes)], totaling 12 bytes (or 16 bytes if you consider the stride adjustment for arrays).


![](Assets/Screenshots/animated_water_texture.png)

okay, animated texture. just passing in a mat4 for simplicity. I should figure out how to make GIFs out of these.

## 1/6/24 More textures:
Despite looking like crappy plastic this is apparently the best I can do with my current lighting model. 
let's add a snow and beach texture and see if we can't make it look a bit better. plus I can practice binding multiple shaders
* load and bind a beach and snow texture (and replace the grass I found with the terrain texture I downloaded)
* apply it by height
* take a few passes at making the rendering looka little better

![](Assets/Screenshots/blended_textured_lit_terrain.png)

So this still looks like plastic, but it blends three textures: a beach/mountain terrain/snow texture and looks marginally better. this could be used in an early 90s 3D game.

Let's get the water in, done.

Hmm, the water isn't transparent like I'd like. why is that?
* how is the pipeline blending things?
* VkPipelineColorBlendAttachmentState has this set to false by default in the pipeline builder - fixed

![](Assets/Screenshots/lit_textured_scene_transparent_water.png)

hey! not the most terrible looking thing in the world. 


## 1/5/24 Lit Textured Land
![](Assets/Screenshots/lit_textured_grass.png)
The very basic diffuse part of the grass texture is being rendered. I changed the terrain coloration to blend between it and the "snow" so it looks a tad smoother.

This still looks like shiny plastic. given the basic lighting model I think that's about all I can do for now. 

## 1/1/24 Textures
let's keep working on our land and waves scene:
* set up 
* grass textures
    * get it: https://cc0-textures.com/t/cc0t-grass-001
    * load it
    * texture the terrain
* add a moving water texture


### Grass 'texture' from https://cc0-textures.com/t/cc0t-grass-001

This had a zip file with the following, here are the descriptions according to ChatGPT:
* Grass001_4K_Color.jpg : This is the color map (also known as a diffuse map). It's the basic texture that you apply to a model.
* Grass001_4K-JPG.usda / Grass001_4K-JPG.usdc: These are Universal Scene Description (USD) files. USD is a framework for interchange of 3D graphics data, and it's used in the animation and VFX industry. The .usda format is ASCII, and .usdc is binary.
* Grass001_4K_AmbientOcclusion.jpg : This is an Ambient Occlusion (AO) map
* Grass001_4K_Displacement.jpg : This is a displacement map, which is a type of texture used to add detail at render time. It actually modifies the geometry of the model when it's rendered.
* Grass001_4K_NormalDX.jpg / Grass001_4K_NormalGL.jpg: These are normal maps. They are used to add details without using more polygons. A normal map is a texture that stores a direction at each pixel. These directions are called normals. DX and GL refer to DirectX and OpenGL, which are two different ways of interpreting the data in the normal map.
* Grass001_4K_Roughness.jpg : This is a roughness map, which is used in physically based rendering (PBR) engines to determine how rough or smooth a surface is. This affects how it reflects light.
* Grass001_PREVIEW.jpg : This is a preview image of the texture. 

Let me start with the diffuse (Grass001_4K_Color.jpg) map, but I should incorporate these other parts as well
![](Assets/Screenshots/grass_on_sphere.png)

Now let's stick it on the terrain. I need to check te uvs to see what's going on


### Filtering (scaling textures up and down)

There are four basic kinds of filtering for textures:
* Nearest-Neighbor Filtering: The simplest form of texture filtering. It selects the nearest pixel (texel) from the texture to the pixel being rendered. While fast and easy to implement, it can result in a blocky and pixelated image, especially when the texture is viewed at a steep angle or from a great distance.
* Bilinear Filtering: A more advanced method that determines the color of a pixel by taking an average of the four closest texels. This results in smoother images compared to nearest-neighbor filtering, but it can still produce noticeable blurring, especially in minification (when textures appear smaller than their actual size).
* Trilinear Filtering: An enhancement over bilinear filtering that performs linear interpolation between two mipmap levels. Mipmaps are pre-calculated, optimized sequences of images that represent the texture at different resolutions. Trilinear filtering reduces the aliasing and moiré patterns seen in bilinear filtering by smoothly transitioning between mipmap levels.
* Anisotropic Filtering: The most sophisticated and effective method for texture filtering. It accounts for the angle of the viewer's perspective and applies varying degrees of filtering accordingly. This results in textures that look good from all angles and distances, significantly reducing blurring and preserving detail, especially in textures viewed at glancing angles.

### Texture Addressing Modes:

Texture address modes determine how texture coordinates outside the [0, 1] range are handled:
* Clamp: VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE and VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER. CLAMP_TO_EDGE will clamp coordinates to the very edge of the texture, while CLAMP_TO_BORDER allows you to define a border color for any coordinates outside the texture range.
* Repeat (or Wrap): VK_SAMPLER_ADDRESS_MODE_REPEAT. When texture coordinates outside the [0, 1] range are encountered, the texture will be repeated.
* Mirror (or Mirror Repeat): VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT. It mirrors the texture at every integer junction, so the texture flips at 0, 1, 2, etc.
* Border: VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER provides this functionality, allowing you to set a specific border color for coordinates outside the [0, 1] range.

### Texture Format

it looks like Vulkan likes KTX or KTX2 textures. https://www.khronos.org/ktx/ has this to say: 

    KTX (Khronos Texture) is an efficient, lightweight container format for reliably distributing GPU textures to diverse platforms and applications. The contents of a KTX file can range from a simple base-level 2D texture to a cubemap array texture with mipmaps. KTX files hold all the parameters needed for efficient texture loading into 3D APIs such as OpenGL® and Vulkan®, including access to individual mipmap levels

    KTX 2.0 adds support for Basis Universal supercompressed GPU textures. Developed by Binomial, this compression technology produces compact textures that can be efficiently transcoded to a variety of GPU compressed texture formats at run-time. Additionally, Khronos has released the KHR_texture_basisu extension that enables glTF to contain KTX 2.0 textures, resulting in universally distributable glTF assets that reduce download size and use natively supported texture formats to reduce GPU memory size and boost rendering speed on diverse devices and platforms.

I have no idea who Binomial is, or what most of this means. let's not pay too much attention to it for now.

## 1/1/24 Lit land and waves
![](Assets/Screenshots/lit_land_and_waves.png)

Behold the most beautiful land and waves render you could imagine. To be fair when I check back to 12/28 this does appear significantly better (unless you like that flat shaded look)


Same scene with some different roughness values:
![](Assets/Screenshots/high_and_low_roughness_terrain.png)


## 1/1/24 Lit land - Fix Broken Normals

![](Assets/Screenshots/terrain_normals.png)

These normals look more like bad hair rendering than proper normals, which is what I suspected, did I do the partial derivatives wrong? eh, screw it, I'm just going to ask chatGPT for the formula

![](Assets/Screenshots/terrain_normals_chatgpt.png)

Nope, thanks for nothing chatgpt. am I thinking about this wrong? If I have y = cos(x) then then tangent dy/dx = -sin(x), right? let's 

Okay, figured it out, given the grad of our y function (dy/dx,dy/dz) we can find two tangent vectors as follows:
* Tz = (0,dy/dz,1)
* Tx = (1,dy/dx,0)

The cross product of this is just:
* (dy/dx,-1,dz/dx), but we know we want y to be positive so do Tz x Ty and you get
* (-dy/dx,1,dy/dz), yaaay.

Plugging this into our terrain equation is fairly straightforward, and we get:

![](Assets/Screenshots/terrain_normals_correct.png)


Looks good! moving on

## 1/1/24 Normals rendering
Happy new year!

My sidetrack experiment of using a geometry shader to render normals has taken more time than expected, but that's okay, I've learned a lot about pipelines.

Here's what I'm doing:
* the descriptor set layout is just:
    * the usual global transforms
    * the instance transforms for each actor
    * but: these are bound to the geometry stage instead of the vert stage: VK_SHADER_STAGE_GEOMETRY_BIT
* make a pipeline for rendering normals that:
    * disables z tests
    * uses VK_PRIMITIVE_TOPOLOGY_POINT_LIST so we're rendering points and not triangles
    * disables backface culling
    * takes a geometry shader on top of the usual vert/frag shaders
* for the shaders
    * the vert shader just passes things through untransformed - we need world space 
    * has a geometry shader that takes points/normals as input and outputs two verts at the start and end of the normal (see [normals.geom](/Source/Shaders/Geom/normals.geom))


Here's what I got:

![Alt text](/Assets/Screenshots/normals_render_not_working.png)

Hmm, none of the normals are showing up. Let's fire up renderdoc.

Renderdoc lets you highlight what was just drawn in the texture viewer:

![Alt text](/Assets/Screenshots/renderdoc_texture_viewer_highlight_drawcall.png)

Turning this on I see:

![](Assets/Screenshots/renderdoc_highlight_normals.png)

So the normals are being rendered, they're just not showing up. let's see if we can debug this in the fragment shader.

* zoom in on a texture like this: ![](Assets/Screenshots/renderdoc_zoomed_in_texture.png)
* click on history ![](Assets/Screenshots/renderdoc_texture_history.png)

![](Assets/Screenshots/renderdoc_no_pixel_shader_bound.png)

it is saying that there is no pixel shader bound... what does that mean?
* OMFG, the number of shaders was hard coded to 2, thank you renderdoc.

![](Assets/Screenshots/normals_rendering_properly.png)

yaay! although I am surprised I'm not seeing the normals in the back, maybe the symmetry is causing them to be hidden? oh yeah, when I animate it you can see the backface ones too



## 12/30 Lighting the land and waves
let's put a light in the land and waves demo

light the terrain:
* need to calculate proper normals for the terrain
* use the material we calculate in the frag shader from height

terrain normals:
* we calculate the height as follows: .3(v.pos.z * sinf(0.1f * v.pos.x) + v.pos.x * cosf(0.1f * v.pos.z)
* so the tangents for this are the derivates with respect to x and z:
    * t_x = [1, ∂y/∂x, 0] = [1, 0.1f * v.pos.z * cosf(0.1f * v.pos.x) - v.pos.x * 0.1f * sinf(0.1f * v.pos.z), 0]
    * t_z = [0, ∂y/∂z, 1] = [0, v.pos.x * -0.1f * sinf(0.1f * v.pos.z) + sinf(0.1f * v.pos.x), 1]
* and the normal is the cross product of these tangents

![Alt text](/Assets/Screenshots/lit_terrain_0.png)

Okay, this doesn't look right. what's the debugger saying:
* fragNormal _73.xyz float3 -0.75879753, -0.51331687, 0.40091389
* the transformed normal looks just fine in terms of looking like normals, the direction is definitely wrong though, but it is hard to just eyeball this. maybe I can just render the normals

### Rendering normals
* should be easy, looks like I can just make a geometry shader
* hmm, how do we deal with the fact that gl_Position is in homogenous coordinates and already projected... let's check the spec
    * [7.1.4. Geometry Shader Special Variables](https://registry.khronos.org/OpenGL/specs/gl/GLSLangSpec.4.60.html#built-in-variables) sez
        In the geometry shader, built-in variables are intrinsically declared as:
        ```
            in gl_PerVertex {
                vec4 gl_Position;
                float gl_PointSize;
                float gl_ClipDistance[];
                float gl_CullDistance[];
            } gl_in[];

            in int gl_PrimitiveIDIn;
            in int gl_InvocationID;

            out gl_PerVertex {
                vec4 gl_Position;
                float gl_PointSize;
                float gl_ClipDistance[];
                float gl_CullDistance[];
            };

            out int gl_PrimitiveID;
            out int gl_Layer;
            out int gl_ViewportIndex;
        ```

        Geometry Shader Input Variables
        gl_Position, gl_PointSize, gl_ClipDistance, and gl_CullDistance contain the values written in the previous shader stage to the corresponding outputs.

        gl_PrimitiveIDIn contains the number of primitives processed by the shader since the current set of rendering primitives was started.

        gl_InvocationID contains the invocation number assigned to the geometry shader invocation. It is assigned integer values in the range [0, N-1], where N is the number of geometry shader invocations per primitive.

I see gl_in[] is an array, does that mean I'm dealing with more than one input depending on how the vert shader works? To the [Geometry Shader Specification!](https://docs.vulkan.org/spec/latest/chapters/geometry.html)
* The geometry shader operates on a group of vertices and their associated data assembled from a single input primitive, and emits zero or more output primitives and the group of vertices and their associated data required for each output primitive. 
* Each geometry shader invocation has access to all vertices in the primitive (and their associated data), which are presented to the shader as an array of inputs.
* The input primitive type expected by the geometry shader is specified with an OpExecutionMode instruction in the geometry shader, and must match the incoming primitive type specified by either the pipeline’s primitive topology if tessellation is inactive, or the tessellation mode if tessellation is active, as follows: (redacted)
* our VulkPipelineBuilder assumes VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, so that's the topology right now.

if the input is points, I need the output to be lines or something, how do I specify that?

Okay, next steps:
* DONE change our topology to VK_PRIMITIVE_TOPOLOGY_POINT_LIST in he normal pipeline
* write the shaders
    * x vert
    * geom
    * frag

light the waves:
* make the water material
* calc pos and norm in world space and pass it through to frag shader from vert shader
* add light and material to fragment shader and calculate ambience/diffuse/specular

## 12/30 More types of lights
I'm not feeling more lights stuff right now, looks like more of what I've already done.

* what should I do next? round out the types of lights
    * directional
    * light attenuation
    * spotlight
    * multiple lights?

### Directional Lights
Given:
* N: normal at p
* L: negative direction of the light
* C: color of the light

we can calculate the light energy that strikes a surface as follows:
* lambert = max(dot(N,L),0)
* L_energy = lambert * C

Then just plug this into the rest of your lighting calculations as usual.

## 12/30 debugging the sphere
![Alt text](/Assets/Screenshots/black_back_of_sphere.png)
When I was moving the light around to validate the shader I noticed the sphere looked like the back half was getting shaved off. I made the clear color magenta to highlight this better. If I just do ambient it looks fine, so best guess is that this is the specular somehow becoming negative. I could just mess around until I figured it out, but this seems like the perfect chance to do some shader debugging.

How can you debug shaders? let's ask ChatGPT
* RenderDoc:
    * Description: RenderDoc is a frame-capture based graphics debugger, often used for analyzing and debugging Vulkan (as well as other API) applications. It captures all the data you need to recreate a rendering event, allowing you to inspect each draw call and the state of the GPU.
    * Usefulness: It's invaluable for understanding what's happening in your draw calls and for inspecting the state of the pipeline and shaders at any point in time.
* Intel® Graphics Performance Analyzers (Intel® GPA):
    * Description: Intel® GPA provides a suite of tools for graphics analysis and optimizations that can help you make the most of Intel's integrated graphics and other compatible GPUs. It includes a range of features for analyzing CPU & GPU workloads, understanding bottlenecks, and visualizing performance data. While it's optimized for Intel hardware, it can provide insights for any platform compatible with its analysis features.
    * Usefulness: Intel® GPA is particularly useful for performance tuning and optimization. It allows you to analyze the rendering pipeline, understand how your shaders affect performance, and experiment with changes to see their impact. For Vulkan development, it can help in optimizing both the compute and graphics workloads that your shaders are part of.
* NVIDIA Nsight Graphics:
    * Description: Nsight Graphics is a powerful debugging and performance analysis tool that supports Vulkan among other APIs. It allows you to inspect your application at a high level and dive down into the details to understand how your graphics API usage affects the GPU.
    * Usefulness: It's particularly useful for performance analysis and for developers using NVIDIA GPUs. It offers shader debugging with the ability to inspect variables and step through shader code.

I've used RenderDoc and Intel GPA professionally before, let's start with RenderDoc. My goal is to see if I can debug any one of the black pixels we see at the left side of the sphere:
* looking at the Texture Viewer
    * right click on one of the black pixels
    * click 'History'
    * in the 'Pixel History on Swapchain etc.' I see two operations
        1. clearing the pixel to magenta
        2. the pixel being turned black. for this operation I can expand the operations and I see Shader Out:
            * R: nan
            * G: nan
            * B: nan
            * A: nan
* well okay, some bad float operations going on, makes sense now. Let's see if I can actually debug the operations. https://renderdoc.org/docs/how/how_debug_shader.html - renderdoc does support debugging, apparently
    * glslc needs -g and -O0 (dash oh zero)
* To debug just right click on the event in the 'Pixel History on Swapchain etc.' (EID 17 in my case) and select Debug
    * aha! this line is bad: vec4 microfacet = vec4((m + 8) / 8 * pow(dot(fragNormal, h), m));
    * why?
        * ah! pow(dot(fragNormal, h), m) is the problem:
            * dot(fragNormal, h) is negative
            * m is a float
        * so taking a negative number to a non-integer exponent required imaginary numbers, so it just errors. just cast m to an int (should probably do this anyway)
    * hmm, not working. why? let's check the spec: https://registry.khronos.org/OpenGL/specs/gl/GLSLangSpec.4.60.html#exponential-functions
        * "Results are undefined if x < 0"
    * whoops, okay, just put a 'max' on that.
* fixed!
![Alt text](/Assets/Screenshots/fixed_back_of_sphere.png)


## 12/29 Material Textures? or more types of lights?
* what should I do next? round out the types of lights
    * light attenuation
    * spotlight
    * directional

## 12/29 Lighting
![Alt text](Assets/Screenshots/first_lit_sphere.png)
Victory! 

* yay, onto the next level
* now that I've read about lighting and made some notes below. what next?
    1. define a material, we need the ambient and diffuse properties at least
    2. we need a light as well, probably pass these in using an SSBO
    3. let's do a sphere or a plane first and see how it looks. maybe hold the light constant while the eye moves.
    4. let's also just do one material for the whole thing, we'll do texture mapping next

Notes:
* hmm, how do normals get handled when transformed? can I just rotate them?
    * I see that I should use fragNormal = mat3(transpose(inverse(worldMat))) * inNormal
    * why is that? apprently because we need n' . v' to be 0 where v' = worldMat*v and n' = that inverse transpose thing above. 
    * why? ah, I see n' * Mv = 0
    * so Nn * Mv = 0
    * so n_tN*Mv = 0
    * blah blah, matrix math N needs to be the inverse transposed so that we get n_t*I*v = 0
    * but wait, if worldMat is a pure rotation matrix then the transpose is the inverse so transpose(inverse(worldMat)) = worldMat
    * so I ~can~ just rotate normals by the worldMat *sigh* overthinking is my worst enemy

## 12/28 Lighting

### Lighting vectors:
* E is the eye vector
* p is a point visible to the eye
* v = Norm(E - p) is the unit vector from the eye to p
* L is the unit light vector pointing in the opposite direction 
* r = -v, the reflection vector


### Lambert's cosine law. 

Lambert's Cosine Law can be expressed mathematically as:
I = I0 * cos(theta)
where:
* I is the intensity of the light observed,
* I0 is the intensity when the light is perpendicular to the surface (at
theta = 0 degrees)
* Theta is tngle of incidence

In terms of vector math we can just say:
* I = I0(N . L)

we don't want to add light when it hits from behind, so put a max term in this:
* I = I0*max(N . L, 0)

### A simplified lighting model has three components:
* Diffuse : how light is colored by the material as it bounces around inside it and comes out.
* Ambient : indirect light
* Specular: how light is colored by the shininess of the material

#### Diffuse
Diffuse light models how light may enter a material, bounce around and exit.

Two parts:
1. the first part has the light color, and the *diffuse albedo* color, which specifies the diffuse reflectance. you get this by multiplying the RGB components together
    * let's say you had a light B = (.8, .8, .8) and a diffuse albedo at a point m_d = (.5, 1, .75) then the amount of diffuse light reflected would be
        * B⊗m_d = (.4, .8, .6)
2. Now that we have the intensity of the light, multiply this value by our lambertian and you get the full equation: c_d = B⊗m_d*max(N . L, 0)

#### Ambient
Ambient is a hack to approximate indirect light that exists in the real world. we just use the diffuse material information for this and combine it with a global ambient light value:
* c_a = A_l⊗m_d 

#### Specular
The Fresnel effect is where when light reaches the intersection of two media with different indices of refraction, some of that light is reflected, and some is refracted. we call this 'specular reflection'
* let 0<= R_f <= 1 be the amount of reflected light
* (1-R_f) is the amount of refracted light (by conservation of energy)

##### Schlick's Approximation:
Christoph Schlick proposed a simple approximation formula for the Fresnel factor:
* F(θ) = F_0 + (1 - F_0)(1 - cos(θ))^5

Where:
* F(θ) is the resulting reflection
* F_0 is the head-on full reflectence which is calculated from the refractance of the material
* cos(θ) is the angle between the surface normal and the light

We can turn this into:
* F(θ) = F_0 + (1 - F_0)(1 - N.v)^5

Some F_0 values:
* Gold: (1.00, 0.71, 0.29)
* Copper: (0.95, 0.64, 0.54)
* Aluminum: (0.91, 0.92, 0.92)
* Silver: (0.97, 0.96, 0.91)
* Water (non-chlorinated): (0.02, 0.02, 0.02)
* Glass (typical): (0.04, 0.04, 0.04)
* Plastic (average): (0.03, 0.03, 0.03)
* Wood (varies greatly): (0.03, 0.02, 0.01)
* Skin: (0.03, 0.02, 0.02)
* Concrete: (0.04, 0.04, 0.04)
* Asphalt: (0.05, 0.05, 0.05)

##### Roughness, BRDF and the *microfacet* model
Objects in the real world aren't smooth mirrors, to model this we use the *microfacet* model.

For a given light L and a given view v, we want to know what portion of microfacets reflect towards v
* Define H = normalize(L+V) as the halfway vector between the light and the vector to the viewer

The Bidirectional Reflectance Distribution Function (BRDF) describes how light reflects at an opaque surface. In the context of microfacet models:
* The D term (Distribution) is particularly relevant here. It describes the distribution of microfacets. A common distribution function used is the GGX/Trowbridge-Reitz distribution, which accounts for varying roughness.
* The specular BRDF often includes a term that involves the dot product of H and the normal N, reflecting the alignment of microfacets with the halfway vector.
* D = n.h is popular as it is easy to model

We can then model this as follows:
* let m be the model's 'roughness', where lower values are 'rougher' and higher are smoother
* F_h(θ) = (m + 8)/8(n.h)^m
   * the m+8/8 is some voodoo that approximates light conservation, probably due to the exponentiation of the cos which is in [0,1]

##### Putting it all together: The complete specular equation
Given:
* L: is the light vector
* v: the view direction
* h: is the normal at which microfacets reflect light towards the viewer
* a_h: is the angle between h and L

Then:
* R_f(a_h) is the amount of light reflected towards the viewer due to surface roughness
* S(θ_h) tells us how much light reflected towards the viewer for a given facet due to fresnel effect
* and max(L.n,0) is just the quantity of incoming light (again, if striking from behind we want this to be zero)

So:
* c_s = max(L.n,0)S(θ_h)R_f(a_h)
* c_s = max(L.n,0)B⊗[F_0 + (1 - F_0)(1 - N.v)^5][(m + 8)/8(n.h)^m]

#### Our Full Lighting Equation
Combining our c_a + c_d + c_s we get:
* c_a + c_d + c_s = A_l⊗m_d + B⊗m_d*max(N . L, 0) + max(L.n,0)B⊗[F_0 + (1 - F_0)(1 - N.v)^5][(m + 8)/8(n.h)^m]

But we can simplify a little as specular and diffuse both use the max(...)B⊗
* c_a + c_d + c_s = A_l⊗m_d + B⊗max(N . L, 0){m_d* + [F_0 + (1 - F_0)(1 - N.v)^5](m + 8)/8(n.h)^m}

## 12/28
![Alt text](Assets/Screenshots/basic_waves.png)
* yay working
* basically this updates the buffer each frame

## 12/27
* render the waves
* dynamically update the waves

Notes:
* enough screwing around: get the water rendering issues out of the way. what issues do we have?
    1. terrain has its own special shader because height is colored: we need a new shader for water
    2. but shaders are bound to pipelines and immutible so we need to make a pipeline for water too
    3. and we need our own descriptorsetalayout, descriptor set, pool, etc.
    So:
        * x make the water shaders
        * make a descriptor set layout, descriptor set, and pool
        * make a pipeline
            * use the same ubo as the actor pipeline
            * bind the same vertex description as well
        * render
* waves are not showing up, hmm.
    * swapping render order and the hils still show up
    * adding the waves mesh to the actors and it shows up
    * it must be something about the index or vertex buffers...
    * oh good lord, I was passing 0 for the number of indices to draw in vkCmdDrawIndexed and the number of indices was being passed as the number of instances. intellisense might be nice to have working...

## 12/26
* render the waves
* dynamically update the waves

Notes:
* validation errors: output of vertex shader doesn't match input of fragment...whoops, wrong vert shader
* after a long time of copy-pasta-ing all the vulkan boilerplate I finally broke down and packed a few things up. Partly this is because in my experience it is easy to write wrappers thatt don't add value until you really understand how the code is being used. In this case the level of abstraction that seems useful for descriptor sets seems to be at the type of thing (e.g. uniform, texture sampler, shader buffer) so I'm giving that a try qirh the VulkDescriptorSetLayoutBuilder and VulkDescriptorSetUpdater, both of which provide helper one function for each of these types.q

## 12/26
* render the waves
* dynamically update the waves

## 12/25 add some water
* create a grid
* update the water position over time in CPU
* update the buffer for rendering

Notes:
* did basic refactor so we can render the terrain and waves separately

## 12/25 Procedural Terrain
![Alt text](Assets/Screenshots/terrain_land.png)
very basic results of terrain:
* adjust the heights based on x/y noise function
* use the frag shader to color based on passed in height

* Let's make a landscape:
    * higher stuff has snow
    * lower stuff is beach
    * middle stuff is grassy
    * (but just do colors for now)
## 12/24 stop making a scene

## 12/24 would you please start making a scene
![Alt text](Assets/Screenshots/first_scene.png)
~Look at that ugly-ass scene!~
* make a scene: 
    * I'd like the axes to be rendered properly
    * let's make a floor
    * draw some cylinders and spheres.
* we only have one texture at the moment (uv checker) might be nice to draw different ones
* I just realized the vertex color isn't used at all, I should remove that - DONE


Notes:
![Alt text](Assets/Screenshots/renderdoc_basic.png)
* little sidetrip with renderdoc - I was just curious to see what this looks like and was not disappointed. I do wish I could see inside the descriptorsets, it looks like this is included in the api.

## 12/23 Instancing continued
![Alt text](Assets/Screenshots/per_model_descriptor_set_and_ssbo.png)

Plan:
* Now that we support multiple instances of a single mesh, I need to add support for multiple meshes and drawing all the instances for each mesh. - DONE
    * Reminder of how things work currently:
        * we stick all of our meshes and all of our indices into a big buffer
        * we have a list of actors in the Scene 
        * we create a contiguous set of SSBO buffers which we write the instance xforms to
        * in the descriptor set layout we specify that we're using an SSBO at binding 2
        * We create an SSBO for all the actors which we mmap
        * We add these to the per-frame descriptor set
        * we render the set of indices N times where N is the number of instanced actors.
    * what we need:
        * we can still stick all the meshes and indices into big buffers
        * we need the actors per mesh type
        * we need a set of per-mesh-type SSBOs
        * descriptor set layout can stay the same
        * but we need a per-mesh descriptor set
        * with rendering, for each mesh type:
            * update the transformations for this mesh
            * we bind the descriptor set for this mesh
            * and render the instances for this mesh
* I might take a little segue and try the vertex offset in vkDrawIndexed because why not use what the API provides - DONE
* then I need to try to actually make a relatively interesting scene


Notes:
* x let's add the axes so I can confirm the orientation of things.
* x hmm, only the first instance model is running properly, what'd I do wrong...ah, auto& not auto
* x out of pool memory now that I added a second mesh to render...just allocate more per actor...
    * just forgot to include the maxSets when I upped the pool sizes.  

## 12/22 Instancing

![Two Instances](Assets/Screenshots/two_quad_instances.png)

Here's a diagram describing what we did:
![Alt text](Assets/Screenshots/single_mesh_instance_diagram.png)

Learnings:
* SSBOs or Shader Storage Buffer Objects are a way of passing data into a shader distinct from Uniforms (and also vertex data), I'm using it to pass in the transforms because they support dynamic sizing on the shader side. They're different than uniforms in the following ways:
   * can be much larger than UBOs (but this is device specific how much)
   * read/write in the shader (as opposed to read only)
   * support dynamic sized arrays - critical for actor instancing unless I want to compile a special shader per
* when you set up your bindings in vkCreateDescriptorSetLayout it turns out the binding value in VkDescriptorSetLayoutBinding has to be unique across all descriptors in the set, which makes sense but it means that your global space for bindings needs to be disjoint for the two... at some point I'll need to think about either standard bindings ids (e.g. INSTANCE_XFORM=100) or maybe an offset (FRAG_BINDING_START = 100), but let's not worry about it for now

How do I get the xforms of each of the actors to the shader?
* vkCmdDrawIndexed has a parameter instanceIndex that I bet gets passed to the shader
* I need a contiguous block of mat4s so I can index them, one per actor (I suppose)
* Let's go with Uniform Buffer Objects for that. the tutorial did the following for this, for each frame:
    * make a ubo VkBuffer with VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : i.e. you can write to it in cpu memory and it is auto-sync'd
    * allocate some VkDeviceMemory for the buffer that meets the above requirements 
    * create pointers to it in cpu space by calling vkMapMemory
    


Misc Notes:
* "Unable to allocate 2 descriptors of type VK_DESCRIPTOR_TYPE_STORAGE_BUFFER from VkDescriptorPool 0x4b7df1000000002f[]. This pool only has 0 descriptors of this type remaining"
   * for VkDescriptorPoolSize, pass in VK_DESCRIPTOR_TYPE_STORAGE_BUFFER info
* "Write update to VkDescriptorSet binding #2 failed with error message: Attempted write update to buffer descriptor failed due to: Buffer with usage mask 0x10 being used for a descriptor update of type VK_DESCRIPTOR_TYPE_STORAGE_BUFFER does not have VK_BUFFER_USAGE_STORAGE_BUFFER_BIT set.
   * I allocated this the same way I did my uniform buffer...
   * ah! I was still creating this as a uniform buffer: VK_BUFFER_USAGE_STORAGE_BUFFER_BIT instead

https://github.com/pumexx/pumex/blob/master/data/shaders/crowd_simple_animation.vert 
https://github.com/SaschaWillems/Vulkan/blob/master/examples/instancing/instancing.cpp ? helpful ?
## 12/21 Got three objects rendering
![Alt text](Assets/Screenshots/three_objects_rendering.png)

Three geos rendering in one big vertex and index buffer. next I'd like to treat these as models that I load once and render multiple times
with my actors so I just need to get refs to them and somehow use that when I go to render them in my drawcall


Glossary
* SSBOs or Shader Storage Buffer Objects: globals for shaders but bigger than uniform buffers and can have an unspecified size up front

TODOS (Closed)
* why does each actor's mesh need its own descriptorset again? because of how I'm doing instancing: the xform for each actor is offset from 0 from the SSBO xform buf. This could just be one big buffer that we use an instance offset for as well (and that's probably the 'right' way to do it), this is just how I did it
* glslc includes must be a thing, I should look into that for some constants/lighting functionality
* it feels like the descriptor set layout could inform the descriptor pool allocator and descriptor set updater...
    * maybe we can have an 'addVertexBindings' for VulkPipelineBuilder that just does that boilerplate.
