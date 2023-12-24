# VulkanRenderer

This is a series of adventures to try to learn how to make a renderer.
1. Geometry
1. Lighting

...

# Geometry
For geometry lets hand-make a box, cylinder, sphere

1. Box


# Lighting

# TODOs
* look into vkDrawIndexedIndirect which can describe what it is drawing.
* try the vertex offset in vkDrawIndexed so we don't have to fixup the indicies
* switch to Shader Storage Buffer Objects (SSBOs) for the actor xforms
 
# Log

## 12/23 Instancing continued

Plan:
* Now that we support multiple instances of a single mesh, I need to add support for multiple meshes and drawing all the instances for each mesh.
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
* let's add the axes so I can confirm the orientation of things.
* hmm, only the first instance model is running properly, what'd I do wrong...ah, auto& not auto
* out of pool memory now that I added a second mesh to render...just allocate more per actor

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