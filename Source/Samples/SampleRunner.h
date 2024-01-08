
/**
 * takes a class that takes Vulk/SampleRunner as a constructor argument and 
 * has a 'render' function that takes a VkCommandBuffer, a current frame index, a VkViewport, and a VkRect2D scissor
*/
template <typename T>
class SampleRunner : public Vulk
{
protected:
    std::unique_ptr<T> world;
    void init() override
    {
        world = std::make_unique<T>(*this);
    }

    void drawFrame(VkCommandBuffer commandBuffer, VkFramebuffer frameBuffer) override
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VK_CALL(vkBeginCommandBuffer(commandBuffer, &beginInfo));

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = frameBuffer;
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChainExtent;

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.1f, 0.0f, 0.1f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)swapChainExtent.width;
        viewport.height = (float)swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        {
            world->render(commandBuffer, currentFrame, viewport, scissor);
        }
        vkCmdEndRenderPass(commandBuffer);

        VK_CALL(vkEndCommandBuffer(commandBuffer));
    }

    virtual void cleanup()
    {
        world.reset();
    }

    void keyCallback(int key, int scancode, int action, int mods)
    {
        VulkCamera &camera = world->camera;
        if (action == GLFW_PRESS || action == GLFW_REPEAT)
        {
            glm::vec3 fwd = camera.getForwardVec();
            glm::vec3 right = camera.getRightVec();
            glm::vec3 up = camera.getUpVec();
            float move = .005f;
            bool handled = true;
            if (key == GLFW_KEY_W)
            {
                camera.eye += move * fwd;
            }
            else if (key == GLFW_KEY_A)
            {
                camera.eye -= move * right;
            }
            else if (key == GLFW_KEY_S)
            {
                camera.eye -= move * fwd;
            }
            else if (key == GLFW_KEY_D)
            {
                camera.eye += move * right;
            }
            else if (key == GLFW_KEY_Q)
            {
                camera.eye -= move * up;
            }
            else if (key == GLFW_KEY_E)
            {
                camera.eye += move * up;
            }
            else if (key == GLFW_KEY_LEFT)
            {
                camera.yaw -= 15.0f;
            }
            else if (key == GLFW_KEY_RIGHT)
            {
                camera.yaw += 15.0f;
            }
            else if (key == GLFW_KEY_UP)
            {
                camera.pitch += 15.0f;
            }
            else if (key == GLFW_KEY_DOWN)
            {
                camera.pitch -= 15.0f;
            }
            else
            {
                handled = false;
            }
            if (handled)
            {
                camera.yaw = fmodf(camera.yaw, 360.0f);
                camera.pitch = fmodf(camera.pitch, 360.0f);
                std::cout << "eye: " << camera.eye.x << ", " << camera.eye.y << ", " << camera.eye.z << " yaw: " << camera.yaw << " pitch: " << camera.pitch << std::endl;
                return;
            }
        }
        Vulk::keyCallback(key, scancode, action, mods);
    }
};
