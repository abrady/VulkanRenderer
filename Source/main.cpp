#include "Vulk/VulkDescriptorSetLayoutBuilder.h"

#include <iostream>
#include <string>
#include <vector>

#include "Samples/PlanarShadowWorld.h"
// #include "Samples/VikingRoom.h"
// #include "Samples/Scene.h"
// #include "Samples/LandAndWaves.h"
// #include "Samples/Lighting.h"
// #include "Samples/LitLandAndWaves.h"
// #include "Samples/TexturedScene.h"
// #include "Samples/Blending.h"
#include "Samples/SampleRunner.h"
// #include "Samples/MirrorWorld.h"
// #include "Samples/OutlineWorld.h"

int main()
{
    // VikingRoom sample;
    // ShapeGeometry sample;
    // Scene sample;
    // LandAndWaves sample;
    // Lighting sample;
    // LitLandAndWaves sample;
    // TexturedScene sample;
    // Blending sample;
    // SampleRunner<MirrorWorld> sample;
    // SampleRunner<OutlineWorld> sample;
    // SampleRunner<MirrorWorld> sample;
    SampleRunner<PlanarShadowWorld> sample;
    sample.run();
    return 0;
}