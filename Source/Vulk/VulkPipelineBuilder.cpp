#include "VulkUtil.h"
#include "Vulk.h"
#include "VulkPipelineBuilder.h"

VulkPipelineBuilder::VulkPipelineBuilder(Vulk &vk) : vk(vk)
{
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; // or VK_CULL_MODE_NONE; for no culling
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR};
}

VulkPipelineBuilder &VulkPipelineBuilder::addShaderStage(VkShaderStageFlagBits stage, char const *path)
{
    auto shaderCode = readFileIntoMem(path);
    VkShaderModule shaderModule = vk.createShaderModule(shaderCode);
    addShaderStage(stage, shaderModule);
    shaderModules.push_back(shaderModule); // this owns the shader module
    return *this;
}

VulkPipelineBuilder &VulkPipelineBuilder::addShaderStage(VkShaderStageFlagBits stage, VkShaderModule shaderModule)
{
    VkPipelineShaderStageCreateInfo shaderStageInfo{};
    shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.stage = stage;
    shaderStageInfo.module = shaderModule;
    shaderStageInfo.pName = "main"; // entrypoint, by convention

    shaderStages.push_back(shaderStageInfo);
    return *this;
}

VulkPipelineBuilder &VulkPipelineBuilder::setPrimitiveTopology(VkPrimitiveTopology topology)
{
    inputAssembly.topology = topology;
    return *this;
}

VulkPipelineBuilder &VulkPipelineBuilder::setLineWidth(float lineWidth)
{
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(vk.physicalDevice, &deviceProperties);
    assert(deviceProperties.limits.lineWidthRange[0] <= lineWidth && lineWidth <= deviceProperties.limits.lineWidthRange[1]);
    rasterizer.lineWidth = lineWidth;
    return *this;
}

VulkPipelineBuilder &VulkPipelineBuilder::setCullMode(VkCullModeFlags cullMode)
{
    rasterizer.cullMode = cullMode;
    return *this;
}

VulkPipelineBuilder &VulkPipelineBuilder::setDepthTestEnabled(bool enabled)
{
    depthStencil.depthTestEnable = enabled;
    return *this;
}
VulkPipelineBuilder &VulkPipelineBuilder::setDepthWriteEnabled(bool enabled)
{
    depthStencil.depthWriteEnable = enabled;
    return *this;
}

VulkPipelineBuilder &VulkPipelineBuilder::addVertexInputBindingDescription(uint32_t binding, uint32_t stride, VkVertexInputRate inputRate)
{
    bindingDescription.binding = binding;
    bindingDescription.stride = stride;
    bindingDescription.inputRate = inputRate;
    return *this;
}

VulkPipelineBuilder &VulkPipelineBuilder::addVertexInputField(uint32_t binding, uint32_t location, uint32_t offset, VkFormat format)
{
    VkVertexInputAttributeDescription attributeDescription{};
    attributeDescription.binding = binding;
    attributeDescription.location = location;
    attributeDescription.format = format;
    attributeDescription.offset = offset;
    attributeDescriptions.push_back(attributeDescription);
    return *this;
}

VulkPipelineBuilder &VulkPipelineBuilder::addVertexInputFieldVec3(uint32_t binding, uint32_t location, uint32_t offset)
{
    return addVertexInputField(binding, location, offset, VK_FORMAT_R32G32B32_SFLOAT);
}

VulkPipelineBuilder &VulkPipelineBuilder::addVertexInputFieldVec2(uint32_t binding, uint32_t location, uint32_t offset)
{
    return addVertexInputField(binding, location, offset, VK_FORMAT_R32G32_SFLOAT);
}

VulkPipelineBuilder &VulkPipelineBuilder::addVulkVertexInput(uint32_t binding)
{
    return addVertexInputBindingDescription(binding, sizeof(Vertex))
        .addVertexInputFieldVec3(binding, Vertex::PosBinding, offsetof(Vertex, pos))
        .addVertexInputFieldVec3(binding, Vertex::NormalBinding, offsetof(Vertex, normal))
        .addVertexInputFieldVec3(binding, Vertex::TangentBinding, offsetof(Vertex, tangent))
        .addVertexInputFieldVec2(binding, Vertex::TexCoordBinding, offsetof(Vertex, texCoord));
}

VulkPipelineBuilder &VulkPipelineBuilder::setBlendingEnabled(bool enabled, VkColorComponentFlags colorWriteMask)
{
    colorBlendAttachment.blendEnable = enabled;
    if (enabled)
    {
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }
    colorBlendAttachment.colorWriteMask = colorWriteMask;
    return *this;
}

void VulkPipelineBuilder::build(VkDescriptorSetLayout descriptorSetLayout, VkPipelineLayout *pipelineLayout, VkPipeline *graphicsPipeline)
{
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

    VK_CALL(vkCreatePipelineLayout(vk.device, &pipelineLayoutInfo, nullptr, pipelineLayout));

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = *pipelineLayout;
    pipelineInfo.renderPass = vk.renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    VK_CALL(vkCreateGraphicsPipelines(vk.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, graphicsPipeline));

    for (auto shaderModule : shaderModules)
    {
        vkDestroyShaderModule(vk.device, shaderModule, nullptr);
    }
}

VulkPipelineBuilder &VulkPipelineBuilder::setStencilTestEnabled(bool enabled)
{
    depthStencil.stencilTestEnable = enabled;
    return *this;
}

VulkPipelineBuilder &VulkPipelineBuilder::setStencilFrontFailOp(VkStencilOp failOp)
{
    depthStencil.front.failOp = failOp;
    return *this;
}

VulkPipelineBuilder &VulkPipelineBuilder::setFrontStencilPassOp(VkStencilOp passOp)
{
    depthStencil.front.passOp = passOp;
    return *this;
}

VulkPipelineBuilder &VulkPipelineBuilder::setFrontStencilCompareOp(VkCompareOp compareOp)
{
    depthStencil.front.compareOp = compareOp;
    return *this;
}

VulkPipelineBuilder &VulkPipelineBuilder::setFrontStencilCompareMask(uint32_t compareMask)
{
    depthStencil.front.compareMask = compareMask;
    return *this;
}

VulkPipelineBuilder &VulkPipelineBuilder::setFrontStencilWriteMask(uint32_t writeMask)
{
    depthStencil.front.writeMask = writeMask;
    return *this;
}

VulkPipelineBuilder &VulkPipelineBuilder::setFrontStencilReference(uint32_t reference)
{
    depthStencil.front.reference = reference;
    return *this;
}

VulkPipelineBuilder &VulkPipelineBuilder::copyFrontStencilToBack()
{
    depthStencil.back = depthStencil.front;
    return *this;
}

// TODO: figure stencil out
// depthStencilState.front = {}; // Configure front faces
// depthStencilState.back = {}; // Configure back faces
// Configure stencil operations for front faces
// depthStencilState.front.failOp = VK_STENCIL_OP_KEEP;
// depthStencilState.front.passOp = VK_STENCIL_OP_REPLACE;
// depthStencilState.front.depthFailOp = VK_STENCIL_OP_KEEP;
// depthStencilState.front.compareOp = VK_COMPARE_OP_ALWAYS;
// depthStencilState.front.compareMask = 0xFF;
// depthStencilState.front.writeMask = 0xFF;
// depthStencilState.front.reference = 1;
// Configure stencil operations for back faces
// depthStencilState.back = depthStencilState.front;