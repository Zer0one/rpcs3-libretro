#pragma once

// RPCS3's game patches as core options: the Patch Manager category. See
// libretro_patch_manager.cpp.

#include "libretro.h"

#include <functional>
#include <string>
#include <vector>

namespace libretro_patches
{
	using option_getter = std::function<std::string(const char* key)>;

	// The patches for one game: what patch.yml, imported_patch.yml and the
	// game's own <serial>_patch.yml have for its serial. Empty clears them.
	void collect(const std::string& serial);

	// Clear the collected title and return the informational status to no title.
	void clear();

	// Informational single-value field, not an adjustable patch setting. RetroArch
	// hides categories with no visible items, so this keeps an empty manager visible.
	inline retro_core_option_v2_definition unavailable_status(bool title_loaded)
	{
		retro_core_option_v2_definition status{};
		status.key = "rpcs3_patch_status";
		status.desc = "Patch Manager: Patch Status";
		status.desc_categorized = "Patch Status";
		status.category_key = "patches";
		status.info = "Informational status from the collected patch catalog, not a patch setting. No listed patches does not mean the game is unsupported.";
		// The value itself is readable too: API 0 has no separate value labels.
		const char* message = title_loaded ? "No patches available for this title" : "Load a title to check for patches";
		status.values[0] = {message, message};
		status.default_value = status.values[0].value;
		return status;
	}

	// Whether the collected game has actionable patches (the category is always present).
	bool any();

	// The options for the collected patches, to be declared after the fixed
	// ones, or one informational status when none are available. Pointers stay
	// valid until the next collect()/clear().
	const std::vector<retro_core_option_v2_definition>& definitions();

	// Shows the value choices of a patch only while the patch is on
	void update_display(retro_environment_t cb, const option_getter& get);

	// Writes patch_config.yml for what the options turn on. RPCS3 reads it when
	// the game boots, so a change applies at the next start.
	void write_config(const option_getter& get);
}
