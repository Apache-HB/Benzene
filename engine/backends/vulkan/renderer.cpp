#include "renderer.hpp"

using namespace benzene::vulkan;

#pragma region render_pass

RenderPass::RenderPass(): instance{nullptr},renderpass{nullptr}, swapchain{nullptr} {}
RenderPass::RenderPass(Instance* instance, SwapChain* swapchain): instance{instance}, swapchain{swapchain} {
    vk::AttachmentDescription colour_attachment{};
    colour_attachment.format = swapchain->get_format();
    colour_attachment.samples = vk::SampleCountFlagBits::e1;
    colour_attachment.loadOp = vk::AttachmentLoadOp::eClear;
    colour_attachment.storeOp = vk::AttachmentStoreOp::eStore;
    colour_attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colour_attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colour_attachment.initialLayout = vk::ImageLayout::eUndefined;
    colour_attachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentReference colour_attachment_ref{};
    colour_attachment_ref.layout = vk::ImageLayout::eColorAttachmentOptimal;
    colour_attachment_ref.attachment = 0;

    vk::SubpassDescription subpass{};
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colour_attachment_ref;

    vk::SubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.srcAccessMask = {};
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

            
    vk::RenderPassCreateInfo render_pass_create_info{};
    render_pass_create_info.attachmentCount = 1;
    render_pass_create_info.pAttachments = &colour_attachment;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass;
    render_pass_create_info.dependencyCount = 1;
    render_pass_create_info.pDependencies = &dependency;

    this->renderpass = instance->device.createRenderPass(render_pass_create_info);
}

void RenderPass::clean(){
    this->instance->device.destroyRenderPass(this->renderpass);
}

vk::RenderPass& RenderPass::handle(){
    return renderpass;
}

#pragma endregion

#pragma region pipeline


RenderPipeline::RenderPipeline(): instance{nullptr}, swapchain{nullptr}, renderpass{}, layout{nullptr}, pipeline{nullptr} {}
RenderPipeline::RenderPipeline(Instance* instance, SwapChain* swapchain, RenderPipeline::Options& options): instance{instance}, swapchain{swapchain} {
    auto binding_description = Vertex::get_binding_description();
    auto attribute_descriptions = Vertex::get_attribute_descriptions();

    vk::PipelineVertexInputStateCreateInfo vertex_input_create_info{};
    vertex_input_create_info.vertexBindingDescriptionCount = 1;
    vertex_input_create_info.pVertexBindingDescriptions = &binding_description;
    vertex_input_create_info.vertexAttributeDescriptionCount = attribute_descriptions.size();
    vertex_input_create_info.pVertexAttributeDescriptions = attribute_descriptions.data();

    vk::PipelineInputAssemblyStateCreateInfo input_assembly_create_info{};
    input_assembly_create_info.topology = vk::PrimitiveTopology::eTriangleList;
    input_assembly_create_info.primitiveRestartEnable = false;

    vk::PipelineViewportStateCreateInfo viewport_create_info{};
    viewport_create_info.viewportCount = 1;
    viewport_create_info.pViewports = nullptr; // Dynamic and thus ignored
    viewport_create_info.scissorCount = 1;
    viewport_create_info.pScissors = nullptr; // Dynamic and thus ignored

    vk::PipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.depthClampEnable = false;
    rasterizer.rasterizerDiscardEnable = false;
    rasterizer.polygonMode = options.polygon_mode;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vk::CullModeFlagBits::eBack;
    rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
    rasterizer.depthBiasEnable = false;

    vk::PipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sampleShadingEnable = false;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
    multisampling.minSampleShading = 1.0f;
    multisampling.alphaToCoverageEnable = false;
    multisampling.alphaToOneEnable = false;

    vk::PipelineColorBlendAttachmentState colour_blend_attachment{};
    colour_blend_attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    colour_blend_attachment.blendEnable = false;
            
    vk::PipelineColorBlendStateCreateInfo colour_blending{};
    colour_blending.logicOpEnable = false;
    colour_blending.logicOp = vk::LogicOp::eCopy;
    colour_blending.attachmentCount = 1;
    colour_blending.pAttachments = &colour_blend_attachment;
    colour_blending.blendConstants[0] = 0.0f;
    colour_blending.blendConstants[1] = 0.0f;
    colour_blending.blendConstants[2] = 0.0f;
    colour_blending.blendConstants[3] = 0.0f;

    std::array<vk::DynamicState, 2> dynamic_states = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };

    vk::PipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.dynamicStateCount = dynamic_states.size();
    dynamic_state.pDynamicStates = dynamic_states.data();

    vk::DescriptorSetLayoutBinding ubo_layout_binding{};
    ubo_layout_binding.binding = 0;
    ubo_layout_binding.descriptorType = vk::DescriptorType::eUniformBuffer;
    ubo_layout_binding.descriptorCount = 1;
    ubo_layout_binding.stageFlags = vk::ShaderStageFlagBits::eVertex;

    vk::DescriptorSetLayoutCreateInfo layout_info{};
    layout_info.bindingCount = 1;
    layout_info.pBindings = &ubo_layout_binding;
    
    descriptor_set_layout = instance->device.createDescriptorSetLayout(layout_info);

    vk::PushConstantRange push_constant_range{};
    push_constant_range.stageFlags = vk::ShaderStageFlagBits::eVertex;
    push_constant_range.size = sizeof(PushConstants);
    push_constant_range.offset = 0;

    vk::PipelineLayoutCreateInfo layout_create_info{};
    layout_create_info.setLayoutCount = 1;
    layout_create_info.pSetLayouts = &descriptor_set_layout;
    layout_create_info.pushConstantRangeCount = 1;
    layout_create_info.pPushConstantRanges = &push_constant_range;

    this->layout = instance->device.createPipelineLayout(layout_create_info);
            
    this->renderpass = RenderPass{instance, swapchain};

    std::vector<vk::PipelineShaderStageCreateInfo> shader_stages{};
    for(auto& shader : options.shaders)
        shader_stages.push_back(shader.get_stage_create_info());

    vk::GraphicsPipelineCreateInfo pipeline_create_info{};
    pipeline_create_info.stageCount = shader_stages.size();
    pipeline_create_info.pStages = shader_stages.data();
    pipeline_create_info.pVertexInputState = &vertex_input_create_info;
    pipeline_create_info.pInputAssemblyState = &input_assembly_create_info;
    pipeline_create_info.pViewportState = &viewport_create_info;           
    pipeline_create_info.pRasterizationState = &rasterizer;
    pipeline_create_info.pMultisampleState = &multisampling;
    pipeline_create_info.pColorBlendState = &colour_blending;
    pipeline_create_info.pDynamicState = &dynamic_state;
    pipeline_create_info.layout = layout;
    pipeline_create_info.renderPass = renderpass.handle();
    pipeline_create_info.subpass = 0;
    pipeline_create_info.basePipelineHandle = {nullptr};
    pipeline_create_info.basePipelineIndex = -1;

    this->pipeline = instance->device.createGraphicsPipeline({nullptr}, pipeline_create_info);
}

void RenderPipeline::clean(){
    instance->device.destroyDescriptorSetLayout(this->descriptor_set_layout);
    instance->device.destroyPipeline(this->pipeline);
    instance->device.destroyPipelineLayout(this->layout);
    renderpass.clean();
}

#pragma endregion