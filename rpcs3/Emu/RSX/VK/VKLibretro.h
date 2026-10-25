#pragma once

#include "VulkanAPI.h"
#include "util/types.hpp"

#include <vector>

// The libretro core's hardware Vulkan context.
//
// Without it, the core draws on a device of its own and copies every finished
// frame back to system memory for the frontend. With it, the frontend creates
// the VkInstance, the core creates the VkDevice on it during context
// negotiation - RPCS3's own device setup, with whatever the frontend needs on
// top - and each finished frame is handed to the frontend as an image on that
// device: no copy through the CPU.
//
// Nothing in here knows about libretro's headers; the core fills it in from
// its negotiation callbacks and reads frames out of it in retro_run.
namespace vk
{
	class instance;
	class render_device;
	class swapchain_base;
}

namespace vk::libretro
{
	// On once the frontend has accepted the hardware Vulkan context. The
	// renderer then takes the shared device and swapchain below instead of
	// making its own, and hands frames over as images.
	bool hw_present();
	void set_hw_present(bool on);

	// The device extensions and features the frontend asked for in
	// create_device, merged into what render_device::create asks for itself.
	void set_frontend_requirements(const char* const* extensions, u32 count, const VkPhysicalDeviceFeatures* features);
	void apply_frontend_requirements(std::vector<const char*>& extensions, VkPhysicalDeviceFeatures& features);

	// The device handed to the frontend is the frontend's: it destroys it after
	// destroy_device, so render_device::destroy must leave it alone.
	void set_frontend_device(VkDevice dev);
	bool is_frontend_device(VkDevice dev);

	// Negotiated frontend queue. A separate queue keeps drawable acquisition
	// in the frontend from holding up renderer submissions. Set before create.
	void set_frontend_queue(u32 family, u32 index, bool separate);
	u32 frontend_queue_family();
	u32 frontend_queue_index();
	bool separate_queue();
	VkQueue frontend_queue(VkDevice device);

	// Drain prior submissions with bounded fences, outside both submit locks.
	// Only used at renderer resize/shutdown and context destruction.
	void drain_queues(const vk::render_device& device);

	// A shared VkQueue must be externally synchronized. Renderer submits take
	// the frontend lock only when both use that queue; a dedicated queue still
	// uses RPCS3's global submit lock.
	void set_queue_lock(void (*lock)(void*), void (*unlock)(void*), void* handle);
	void lock_queue();
	void unlock_queue();

	// The instance the frontend created, adopted rather than owned, and the
	// swapchain - and with it the render_device - built during negotiation.
	vk::instance& shared_instance();
	void set_shared_swapchain(vk::swapchain_base* swapchain);
	vk::swapchain_base* shared_swapchain();

	// Frame handoff. The renderer calls frame_ready once a frame's commands
	// have finished on the GPU; the core takes the oldest waiting image in
	// retro_run, so each frame is shown once and in order. A few wait at
	// most; beyond that the oldest go back to the swapchain unseen.
	void frame_ready(u32 image_index);
	bool take_frame(u32& image_index, u32& generation);
	void drop_pending_frames();

	// The core's retro_run tells the handoff it ran, and that it will not
	// (the content is being stopped). While it keeps coming, a frame
	// finishing when the queue is full takes the place of the oldest. Once
	// it has stopped - RetroArch paused, in its menu or stepping frames -
	// the renderer waits after each flip for the next retro_run, and a
	// retro_run that comes then lets the game draw one frame and takes that
	// one: a pause holds the game within a frame, nothing is lost over it,
	// and Frame Advance shows each frame with the input of its step.
	// frontend_returned ends the retro_run frontend_ran began: the frontend
	// counts as stopped by how long it stays away, not by how long a run took.
	void frontend_ran();
	void frontend_returned();
	void frontend_detached();

	// Bumped under the swapchain image lock when images are replaced. Queued
	// indices carry this generation; retired images remain alive while the
	// frontend may still repeat or sample them.
	u32 image_generation();
	void new_image_generation();
}
