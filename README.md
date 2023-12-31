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
5. [Lights](/Source/Samples/Lighting.h)

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
* I've been sloppy naming structs: MeshRender, MeshRenderInfo, MeshFrameResources: make this more coherent
* tests for lighting? I'm vaguely nervous about math errors and having something that looks fine to my untrained eye but is actually wrong.
* it feels like the descriptor set layout could inform the descriptor pool allocator and descriptor set updater...

# Log

## 12/30 Lighting the land and waves
* let's put a light in the land and waves demo

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
