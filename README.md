# VulkanRenderer

This is a series of adventures to try to learn how to make a renderer. 

I'm basically following Introduction to 3D Game Programming with DirectX 12 by Frank Luna which was recommended to me by a friend as a great guide (and so far, it is!)

Some quick thoughts on my approach: one thing you might notice is that I avoid wrapping a lot of the vulkan boilerplate in functions and structs for many of the samples. this makes for verbose code that can seem harder to understand at first glance, however, in my experience, premature abstractions both end up making code more confusing by hiding what is actually going on (i.e. what assumptions the abstraction made) and undermine the goal of code like this which is to understand how Vulkan works. I'm writing this note after six days of effort and my biggest regrets so far have all been premature encapsulation and the subsequent ex-capsulation. 

...

# Samples
1. [Viking Room](Samples/VikingRoom.h) - loads a model and renders it
2. [Shape Geometry](Samples/)
3. [Scene](Samples/Scene.h) - a scene with a floor and some cylinders and spheres, uses instancing
4. [Land and Waves](Samples/LandAndWaves.h) - a procedurally generated terrain example

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

# TODOs
* I've been sloppy naming structs: MeshRender, MeshRenderInfo, MeshFrameResources: make this more coherent
* why does each actor's mesh need its own descriptorset again?

# Log

## 12/28 Lighting
* yay, onto the next level

A simplified lighting model has three components:
* Specular
* Diffuse
* Ambient

Diffuse:
* Lighting vectors:
    * E is the eye vector
    * p is a point visible to the eye
    * v = Norm(E - p) is the unit vector from the eye to p
    * L is the unit light vector pointing in the opposite direction 
    * r = -v, the reflection vector

For diffuse lighting we'll use Lambert's cosine law. Lambert's Cosine Law can be expressed mathematically as:
I = I0 * cos(theta)
where:
* I is the intensity of the light observed,
* I0 is the intensity when the light is perpendicular to the surface (at
theta = 0 degrees)
* Theta is tngle of incidence

In terms of vector math we can just say:
* I = I0(N . L)

we don't want to add light when it hits from behind, so put a max term in this:
* I = max(I0(N . L), 0)


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