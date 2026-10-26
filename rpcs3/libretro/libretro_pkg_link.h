#pragma once

#include <string_view>

namespace libretro_pkg
{
enum class route { install, launch, conflict };

struct package
{
    std::string_view title_id;
    std::string_view content_id;
    std::string_view category;
    bool ps3 = false;
    bool game_exec = false;
    bool patch = false;
};

struct installation
{
    std::string_view title_id;
    std::string_view content_id;
    bool has_eboot = false;
};

inline bool valid_title_id(std::string_view id)
{
    if (id.size() != 9)
        return false;
    for (const char ch : id)
        if (!((ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9')))
            return false;
    return true;
}

inline bool valid_install_directory(std::string_view name)
{
    return !name.empty() && name != "." && name != ".." &&
        name.find_first_of("/\\") == std::string_view::npos &&
        name.find('\0') == std::string_view::npos;
}

// Title identity, rather than the host filename, survives renaming a PKG and
// relocating dev_hdd0 through vfs.yml. Only base PS3 HDD games may be launchers.
inline bool is_base_game(const package& pkg)
{
    return pkg.ps3 && pkg.game_exec && !pkg.patch && pkg.category == "HG" &&
        valid_title_id(pkg.title_id) && pkg.content_id.size() >= 16 &&
        pkg.content_id.substr(7, 9) == pkg.title_id;
}

inline route select(const package& pkg, const installation& installed, bool launcher_enabled = true)
{
    if (!launcher_enabled || !is_base_game(pkg))
        return route::install;

    if (!valid_title_id(installed.title_id))
        return route::install;

    // An update can replace PARAM.SFO without retaining CONTENT_ID. Its exact
    // TITLE_ID still identifies the installed title; a present CONTENT_ID must
    // match, so another edition is neither launched nor overwritten here.
    if (installed.title_id != pkg.title_id ||
        (!installed.content_id.empty() && installed.content_id != pkg.content_id))
        return route::conflict;

    return installed.has_eboot ? route::launch : route::install;
}
}
