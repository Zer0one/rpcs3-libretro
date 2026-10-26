#pragma once

#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace libretro_content_keys
{
inline std::string join(std::string_view directory, std::string_view filename)
{
    std::string path(directory);
    if (!path.empty() && path.back() != '/' && path.back() != '\\')
        path += '/';
    path += filename;
    return path;
}

struct rap_context
{
    std::mutex mutex;
    std::string pkg_directory;
};

inline rap_context& context()
{
    static rap_context value;
    return value;
}

// Set before a package is installed/booted; keep across reset and clear only
// after shutdown, or when loading new content. Crypto workers read a snapshot.
inline void set_pkg_directory(std::string directory)
{
    auto& state = context();
    std::lock_guard lock(state.mutex);
    state.pkg_directory = std::move(directory);
}

inline std::string pkg_directory()
{
    auto& state = context();
    std::lock_guard lock(state.mutex);
    return state.pkg_directory;
}

inline std::vector<std::string> rap_paths(std::string_view pkg_dir, std::string_view data_dir, std::string_view content_id)
{
    std::vector<std::string> paths;
    for (const std::string& directory : {std::string(pkg_dir), join(data_dir, "rap")})
    {
        if (directory.empty())
            continue;
        // The former importer accepted mixed-case RAP extensions. Preserve
        // that support without copying/renaming files, lower case first.
        for (unsigned mask = 0; mask < 8; ++mask)
        {
            std::string name(content_id);
            name += '.';
            name += (mask & 1) ? 'R' : 'r';
            name += (mask & 2) ? 'A' : 'a';
            name += (mask & 4) ? 'P' : 'p';
            paths.push_back(join(directory, name));
        }
    }
    return paths;
}

template <typename Exists>
std::string find_rap(std::string_view pkg_dir, std::string_view data_dir, std::string_view content_id, Exists exists)
{
    for (const auto& path : rap_paths(pkg_dir, data_dir, content_id))
        if (exists(path))
            return path;
    return {};
}

inline std::vector<std::string> iso_directories(std::string_view data_dir, std::string_view legacy_dir)
{
    return {join(data_dir, "key"), join(data_dir, "dkey"), std::string(legacy_dir)};
}

inline std::vector<std::string> iso_paths(std::string_view image, std::string_view data_dir, std::string_view legacy_dir)
{
    // Match the image's basename, including on Windows and SAF-style paths.
    const auto slash = image.find_last_of("/\\");
    const auto dot = image.find_last_of('.');
    const std::string stem(image.substr(0, dot != std::string_view::npos &&
        (slash == std::string_view::npos || dot > slash) ? dot : image.size()));
    const std::string name = stem.substr(slash == std::string_view::npos ? 0 : slash + 1);
    std::vector<std::string> paths{stem + ".dkey", stem + ".key"};
    for (const auto& directory : iso_directories(data_dir, legacy_dir))
        for (const auto extension : {".dkey", ".key"})
            paths.push_back(join(directory, name + extension));
    return paths;
}
}
