#pragma once

#include <string>
#include <string_view>

namespace libretro_installed_playlist
{
inline constexpr std::string_view filename = "Sony - PlayStation 3 (PSN Installed).lpl";
inline constexpr std::string_view database = "Sony - PlayStation 3.lpl";
inline constexpr std::string_view marker_key = "rpcs3_installed_playlist";
inline constexpr std::string_view marker_value = "rpcs3-libretro:psn-installed:1";
inline constexpr std::size_t max_size = 8 * 1024 * 1024;

struct entry
{
    std::string path; // Installed PARAM.SFO, independent of the source PKG.
    std::string title; // Source PKG filename without its final extension.
    std::string core_path;
};

enum class status { unchanged, updated, unowned, invalid };

struct result
{
    status state = status::invalid;
    std::string json;
    std::string error;
};

// nullopt-like distinction: exists=false creates a playlist; an existing empty
// or unsupported file is an error and must never be replaced by a new one.
result update(std::string_view existing, bool exists, const entry& game);

// Frontend names are already extension-free; use them for frontend URI paths
// when supplied. Native paths use their own basename, with either separator.
std::string package_label(std::string_view path, std::string_view frontend_name = {});
}
