#include <vkpp/image_view.hh>

#include <utility>

#include <iostream>

namespace vkpp {
    ImageView::ImageView(VkDevice& device, VkImageView& image_view)
                        : device { device }, handle { image_view } { }

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

        rhs.device = VK_NULL_HANDLE;
        rhs.handle = VK_NULL_HANDLE;
    }

    VkImageView& ImageView::get_handle() {
        return handle;
    }
}
