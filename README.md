# VulkanRenderer

This is a series of adventures to try to learn how to make a renderer.
1. Geometry
1. Lighting

...

# Geometry
For geometry lets hand-make a box, cylinder, sphere

1. Box


# Lighting

 
# Log

## 12/22 Got three objects rendering
![Alt text](Assets/Screenshots/three_objects_rendering.png)

Three geos rendering in one big vertex and index buffer. next I'd like to treat these as models that I load once and render multiple times
with my actors so I just need to get refs to them and somehow use that when I go to render them in my drawcall