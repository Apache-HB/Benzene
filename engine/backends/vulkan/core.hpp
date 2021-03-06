#pragma once

#include "base.hpp"
#include "extra_api.hpp"
#include "swap_chain.hpp"
#include "renderer.hpp"
#include "imgui.hpp"

namespace benzene::vulkan
{
    struct spec_version {
        spec_version(uint32_t v): major{VK_VERSION_MAJOR(v)}, minor{VK_VERSION_MINOR(v)}, patch{VK_VERSION_PATCH(v)} {}
        uint16_t major, minor, patch;
    };
}

template<>
struct format::formatter<benzene::vulkan::spec_version> {
	template<typename OutputIt>
	static void format(format::format_output_it<OutputIt>& it, [[maybe_unused]] format::format_args args, benzene::vulkan::spec_version item){        
		formatter<uintptr_t>::format(it, {}, item.major);
        it.write('.');
        formatter<uintptr_t>::format(it, {}, item.minor);
        it.write('.');
        formatter<uintptr_t>::format(it, {}, item.patch);
    }
};

namespace benzene::vulkan {
    constexpr std::array<const char*, 1> required_device_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    constexpr std::array<const char*, 1> validation_layers = {
        "VK_LAYER_KHRONOS_validation"
    };

    struct BackendModel {
        void clean(){
            indices.clean();
            vertices.clean();
        }
        glm::vec3 pos;
        
        Model* model;
        VertexBuffer vertices;
        IndexBuffer indices;

        RenderPipeline* pipeline;
        #ifdef ENABLE_WIREFRAME_OUTLINE
        RenderPipeline* wireframe_pipeline;
        #endif
    };

    class Backend : public IBackend {
        public:
        Backend(const char* application_name, GLFWwindow* window);
        ~Backend();

        void frame_update(std::unordered_map<ModelId, Model*>& models);
        void end_run();

        private:
        static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT _severity, VkDebugUtilsMessageTypeFlagsEXT _type, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data);
        vk::DebugUtilsMessengerCreateInfoEXT make_debug_messenger_create_info();
        void init_debug_messenger();
        void init_physical_device();
        void init_logical_device();

        std::vector<const char*> get_extensions();

        bool check_validation_layer_support();

        void framebuffer_resize_callback([[maybe_unused]] int width, [[maybe_unused]] int height){
            this->framebuffer_resized = true;
        }

        void mouse_button_callback(int button, bool state){
            ImGui::GetIO().MouseDown[button] = state;
        }

        void mouse_pos_callback(double x, double y){
            ImGui::GetIO().MousePos = ImVec2{(float)x, (float)y};
        }

        void mouse_scroll_callback(double xoffset, double yoffset){
            ImGui::GetIO().MouseWheelH += (float)xoffset;
            ImGui::GetIO().MouseWheel += (float)yoffset;
        }

        template<typename F>
        std::optional<uint32_t> get_queue_index(vk::PhysicalDevice dev, F functor){
            auto queue_families = dev.getQueueFamilyProperties();
            for(uint32_t i = 0; i < queue_families.size(); i++)
                if(functor(queue_families[i], i))
                    return i;

            return {};
        }

        void cleanup_renderer();
        void create_renderer();
        void recreate_renderer();

        void build_command_buffer(size_t i);
        void draw_debug_window();

        Instance instance; 
        bool framebuffer_resized, is_wireframe;
        size_t current_frame;
        std::vector<Buffer> ubos;
        vk::DescriptorPool descriptor_pool;
        std::vector<vk::DescriptorSet> descriptor_sets;
        std::vector<vk::Semaphore> image_available, render_finished;
        std::vector<vk::Fence> in_flight_fences, images_in_flight;
        std::vector<vk::CommandBuffer> command_buffers;
        std::vector<vk::Framebuffer> framebuffers;
        vk::DebugUtilsMessengerEXT debug_messenger;
        Imgui imgui_renderer;

        SwapChain swapchain;
        RenderPipeline pipeline;

        #ifdef ENABLE_WIREFRAME_OUTLINE
        RenderPipeline wireframe_pipeline;
        #endif
        
        uint32_t graphics_queue_id, presentation_queue_id;

        float frame_time, min_frame_time, max_frame_time;
        std::array<float, 100> last_frame_times;
        std::chrono::time_point<std::chrono::high_resolution_clock> last_frame_timestamp;
        uint64_t frame_counter, fps;

        std::unordered_map<ModelId, BackendModel> internal_models;
    };
} // namespace benzene::vulkan
