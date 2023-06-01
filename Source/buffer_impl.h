#include <vulkan/vulkan.hpp>

namespace VPP {

namespace impl {

class UniformBuffer {
public:
	UniformBuffer();
	~UniformBuffer();

	bool Init(vk::BufferUsageFlags usage, vk::DeviceSize size);
	bool SetData(void* data, size_t size);

private:
	vk::BufferUsageFlags usage_ = (vk::BufferUsageFlags)0;
	vk::DeviceSize size_ = 0;
	vk::Buffer buffer_{};
	vk::DeviceMemory memory_{};
};

}

}  // namespace VPP