#include <vkpp/image.hh>

#include <vkpp/device.hh>
#include <vkpp/command_buffer.hh>
#include <vkpp/queue.hh>

#include <vkpp/exception.hh>

#include <utility>

namespace vkpp {
    Image::Image(Device& logical_device, std::uint32_t width, std::uint32_t height,
                 VkFormat format, VkImageUsageFlags usage, std::uint32_t mip_levels,
                 VkSampleCountFlagBits samples, VkImageTiling tiling_mode)
                : layout { VK_IMAGE_LAYOUT_UNDEFINED },
                  tiling_mode { VK_IMAGE_TILING_OPTIMAL },
                  sharing_mode { VK_SHARING_MODE_EXCLUSIVE },
                  device { logical_device.get_handle() } {
        VkImageCreateInfo create_info;
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        create_info.pNext = nullptr;
        create_info.flags = 0;

        create_info.imageType = VK_IMAGE_TYPE_2D;

        create_info.format = format;
        this->format       = format;

        create_info.extent.width  = width;
        create_info.extent.height = height;
        create_info.extent.depth  = 1;

        this->extent = { width, height, 1 };

        this->mip_levels      = mip_levels;
        create_info.mipLevels = mip_levels;
        create_info.arrayLayers = 1;

        create_info.samples = samples;
        this->samples       = samples;

        create_info.tiling      = tiling_mode;
        create_info.sharingMode = sharing_mode;

        create_info.usage = usage;
        this->usage       = usage;

        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = nullptr;

        create_info.initialLayout = layout;

        if (VkResult error = vkCreateImage(device, &create_info, nullptr, &handle)) {
            throw Exception { error, "couldn't create image!" };
        }
    }

    Image::~Image() noexcept {
        if (handle != VK_NULL_HANDLE) {
            vkDestroyImage(device, handle, nullptr);
        }
    }

    Image::Image(Image&& image) noexcept {
        swap(*this, image);
    }

    Image& Image::operator=(Image&& image) noexcept {
        swap(*this, image);
        return *this;
    }

    void swap(Image& lhs, Image& rhs) {
        using std::swap;

        swap(lhs.handle, rhs.handle);
        swap(lhs.device, rhs.device);

        swap(lhs.extent, rhs.extent);
        swap(lhs.usage,  rhs.usage);
        swap(lhs.format, rhs.format);

        swap(lhs.mip_levels, rhs.mip_levels);
        swap(lhs.samples,    rhs.samples);

        swap(lhs.layout, rhs.layout);
        swap(lhs.sharing_mode, rhs.sharing_mode);
        swap(lhs.tiling_mode, rhs.tiling_mode);

        swap(lhs.memory, rhs.memory);
    }

    VkImage& Image::get_handle() {
        return handle;
    }

    const VkExtent3D& Image::get_extent() const {
        return extent;
    }

    VkFormat Image::get_format() const {
        return format;
    }

    VkImageUsageFlags Image::get_usage() const {
        return usage;
    }

    VkSharingMode Image::get_sharing_mode() const {
        return sharing_mode;
    }

    VkImageTiling Image::get_tiling_mode() const {
        return tiling_mode;
    }

    VkImageLayout Image::get_layout() const {
        return layout;
    }

    std::uint32_t Image::get_mip_levels() const {
        return mip_levels;
    }

    VkSampleCountFlagBits Image::get_samples() const {
        return samples;
    }

    VkDeviceMemory& Image::get_bound_memory() {
        return memory;
    }

    VkMemoryRequirements Image::get_memory_requirements() const {
        VkMemoryRequirements requirements;
        vkGetImageMemoryRequirements(device, handle, &requirements);
        return requirements;
    }

    void Image::bind(DeviceMemory& device_memory, std::uint32_t offset) {
        memory = device_memory.get_handle();
        vkBindImageMemory(device, handle, memory, offset);
    }

    void Image::transition(CommandPool& pool, VkImageLayout from, VkImageLayout to) {
        VkImageMemoryBarrier image_memory_barrier;
        image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_memory_barrier.pNext = nullptr;

        image_memory_barrier.oldLayout = from;
        image_memory_barrier.newLayout = to;

        image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        image_memory_barrier.image = get_handle();

        image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_memory_barrier.subresourceRange.baseMipLevel = 0;
        image_memory_barrier.subresourceRange.levelCount = 1;
        image_memory_barrier.subresourceRange.baseArrayLayer = 0;
        image_memory_barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags src_stage, dst_stage;

        if (from == VK_IMAGE_LAYOUT_UNDEFINED &&
            to   == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            image_memory_barrier.srcAccessMask = 0;
            image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (from == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
                   to   == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else {
            throw Exception { "couldn't transition image layout!",
                              "unknown image layout transition!" };
        }

        auto command_buffer = pool.allocate_and_begin();
        command_buffer.pipeline_barrier(src_stage, dst_stage,
                                        image_memory_barrier);
        command_buffer.end();
        pool.get_queue().submit(command_buffer)
                        .wait_idle();

        layout = to; // A new layout!
    }

    void swap(DeviceImage& lhs, DeviceImage& rhs) {
        using std::swap;

        swap(static_cast<Image&>(lhs), static_cast<Image&>(rhs));

        swap(lhs.device_memory, rhs.device_memory);
    }

    DeviceImage& DeviceImage::operator=(DeviceImage&& image) noexcept {
        swap(*this, image);
        return *this;
    }

    DeviceImage::DeviceImage(DeviceImage&& image) noexcept {
        swap(*this, image);
    }

    DeviceImage::DeviceImage(Device& device, CommandPool& pool,
                             vkhr::Image& image,
                             std::uint32_t mip_levels)
                            : Image { device,
                                      static_cast<std::uint32_t>(image.get_width()),
                                      static_cast<std::uint32_t>(image.get_height()),
                                      VK_FORMAT_R8G8B8A8_UNORM,
                                      VK_IMAGE_USAGE_SAMPLED_BIT |
                                      VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                      mip_levels,
                                      VK_SAMPLE_COUNT_1_BIT,
                                      VK_IMAGE_TILING_OPTIMAL } {
        Buffer staging_buffer {
            device,
            static_cast<VkDeviceSize>(image.get_size_in_bytes()),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT
        };

        auto size = image.get_size_in_bytes();

        auto staging_memory_requirements = staging_buffer.get_memory_requirements();

        auto buffer = image.get_data();

        DeviceMemory staging_memory {
            device,
            staging_memory_requirements,
            DeviceMemory::Type::HostVisible
        };

        staging_buffer.bind(staging_memory);
        staging_memory.copy(size, buffer);

        auto image_memory_requirements = get_memory_requirements();

        device_memory = DeviceMemory {
            device,
            image_memory_requirements,
            DeviceMemory::Type::DeviceLocal
        };

        bind(device_memory);

        transition(pool, VK_IMAGE_LAYOUT_UNDEFINED,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        copy(staging_buffer, pool);

        transition(pool, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    DeviceMemory& DeviceImage::get_device_memory() {
        return device_memory;
    }

    void DeviceImage::copy(Buffer& staged, CommandPool& pool) {
        auto command_buffer = pool.allocate_and_begin();
        command_buffer.copy_buffer_image(staged, *this);
        command_buffer.end();
        pool.get_queue().submit(command_buffer)
                        .wait_idle();
    }

    ImageView::ImageView(VkDevice& device, VkImageView& image_view)
                        : device { device }, handle { image_view } { }

    ImageView::ImageView(Device& logical_device, Image& real_image)
                        : image { real_image.get_handle() },
                          device { logical_device.get_handle() } {
        VkImageViewCreateInfo create_info;
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.pNext = nullptr;
        create_info.flags = 0;

        create_info.image = image;

        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = real_image.get_format();

        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        if (VkResult error = vkCreateImageView(device, &create_info, nullptr, &handle)) {
            throw Exception { error, "couldn't create image view!" };
        }
    }

    ImageView::~ImageView() noexcept {
        if (handle != VK_NULL_HANDLE) {
            vkDestroyImageView(device, handle, nullptr);
        }
    }

    ImageView::ImageView(ImageView&& image_view) noexcept {
        swap(*this, image_view);
    }

    ImageView& ImageView::operator=(ImageView&& image_view) noexcept {
        swap(*this, image_view);
        return *this;
    }

    void swap(ImageView& lhs, ImageView& rhs) {
        using std::swap;

        swap(lhs.device, rhs.device);
        swap(lhs.handle, rhs.handle);
        swap(lhs.image,  rhs.image);
    }

    VkImage& ImageView::get_image() {
        return image;
    }

    VkImageView& ImageView::get_handle() {
        return handle;
    }
}
