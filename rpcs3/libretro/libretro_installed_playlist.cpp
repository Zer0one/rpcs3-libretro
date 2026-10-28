#include "libretro_installed_playlist.h"
#include "util/yaml.hpp"

#include <algorithm>
#include <filesystem>
#include <regex>
#include <set>
#include <stdexcept>

namespace libretro_installed_playlist
{
namespace
{
// yaml-cpp already ships with the core and reads JSON. Emit JSON explicitly:
// YAML's own double-quoted emitter also permits escapes such as \x and \0,
// which RetroArch's JSON reader does not accept.
void quoted(std::string& out, std::string_view value)
{
    constexpr char hex[] = "0123456789abcdef";
    out += '"';
    for (const unsigned char ch : value)
    {
        if (ch == '"' || ch == '\\')
        {
            out += '\\';
            out += static_cast<char>(ch);
        }
        else if (ch < 0x20)
        {
            out += "\\u00";
            out += hex[ch >> 4];
            out += hex[ch & 15];
        }
        else
            out += static_cast<char>(ch);
    }
    out += '"';
}

void emit(std::string& out, const YAML::Node& node, unsigned depth = 0)
{
    if (depth > 64 || out.size() > max_size)
        throw std::runtime_error("playlist exceeds the size or nesting limit");
    if (node.IsNull())
        out += "null";
    else if (node.IsScalar())
    {
        const std::string value = node.Scalar();
        if (node.Tag() == "?")
        {
            static const std::regex literal(R"(true|false|-?(0|[1-9][0-9]*)(\.[0-9]+)?([eE][+-]?[0-9]+)?)");
            if (!std::regex_match(value, literal))
                throw std::runtime_error("playlist contains a non-JSON scalar");
            out += value;
        }
        else
            quoted(out, value);
    }
    else if (node.IsSequence())
    {
        out += '[';
        bool first = true;
        for (const auto& child : node)
        {
            if (!first) out += ',';
            first = false;
            out += '\n';
            out.append((depth + 1) * 2, ' ');
            emit(out, child, depth + 1);
        }
        if (!first)
        {
            out += '\n';
            out.append(depth * 2, ' ');
        }
        out += ']';
    }
    else if (node.IsMap())
    {
        out += '{';
        bool first = true;
        std::set<std::string> keys;
        for (const auto& pair : node)
        {
            if (!pair.first.IsScalar() || !keys.insert(pair.first.Scalar()).second)
                throw std::runtime_error("playlist has a non-string or duplicate object key");
            if (!first) out += ',';
            first = false;
            out += '\n';
            out.append((depth + 1) * 2, ' ');
            quoted(out, pair.first.Scalar());
            out += ": ";
            emit(out, pair.second, depth + 1);
        }
        if (!first)
        {
            out += '\n';
            out.append(depth * 2, ' ');
        }
        out += '}';
    }
    else
        throw std::runtime_error("playlist contains an undefined value");
}

std::string field(const YAML::Node& node, const char* key)
{
    const auto value = node[key];
    return value && value.IsScalar() ? value.Scalar() : std::string{};
}

void set_string(YAML::Node node, const char* key, const std::string& value)
{
    node[key] = value;
    node[key].SetTag("!");
}

std::string path_key(const std::string& path)
{
    // URI paths are owned by the frontend; never reinterpret their scheme.
    if (path.find("://") != std::string::npos)
        return path;
    auto value = std::filesystem::path(std::u8string(path.begin(), path.end())).lexically_normal().generic_u8string();
    std::string normalized(value.begin(), value.end());
#ifdef _WIN32
    // Windows paths are case-insensitive; leave non-ASCII UTF-8 bytes intact.
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch)
    {
        return static_cast<char>(ch >= 'A' && ch <= 'Z' ? ch + ('a' - 'A') : ch);
    });
#endif
    return normalized;
}
}

std::string package_label(std::string_view path, std::string_view frontend_name)
{
    if (path.find("://") != std::string_view::npos && !frontend_name.empty())
        return std::string(frontend_name);
    const auto separator = path.find_last_of("/\\");
    const auto leaf = separator == std::string_view::npos ? path : path.substr(separator + 1);
    std::string label(leaf);
    if (label.size() > 4)
    {
        const auto suffix = std::string_view(label).substr(label.size() - 4);
        if (suffix[0] == '.' && (suffix[1] == 'p' || suffix[1] == 'P') &&
            (suffix[2] == 'k' || suffix[2] == 'K') && (suffix[3] == 'g' || suffix[3] == 'G'))
            label.resize(label.size() - 4);
    }
    return label;
}

result update(std::string_view existing, bool exists, const entry& game)
{
    try
    {
        if (game.path.empty() || game.title.empty() || game.core_path.empty())
            throw std::runtime_error("missing installed-game or core identity");
        if (existing.size() > max_size)
            throw std::runtime_error("playlist exceeds the size limit");
        YAML::Node root;
        if (exists)
        {
            auto text = existing;
            if (text.starts_with("\xef\xbb\xbf")) text.remove_prefix(3);
            const auto first = text.find_first_not_of(" \t\r\n");
            if (first == std::string_view::npos || text[first] != '{')
                throw std::runtime_error("only uncompressed JSON playlists can be updated");
            const auto documents = YAML::LoadAll(std::string(text));
            if (documents.size() != 1)
                throw std::runtime_error("playlist must contain one JSON document");
            root = documents.front();
            if (!root.IsMap() || !root["items"].IsSequence() ||
                field(root, "version") != "1.5")
                throw std::runtime_error("unsupported playlist structure or version");
            if (field(root, marker_key.data()) != marker_value)
                return {status::unowned, {}, "existing playlist has no matching RPCS3 ownership marker"};
            // Validate before editing, including otherwise unknown fields.
            std::string checked;
            emit(checked, root);
        }
        else
            root = YAML::Load(R"({"version":"1.5","rpcs3_installed_playlist":"rpcs3-libretro:psn-installed:1","default_core_path":"DETECT","default_core_name":"DETECT","label_display_mode":0,"right_thumbnail_mode":0,"left_thumbnail_mode":0,"thumbnail_match_mode":0,"sort_mode":0,"items":[]})");

        auto items = root["items"];
        const auto key = path_key(game.path);
        for (auto item : items)
        {
            if (!item.IsMap())
                throw std::runtime_error("playlist contains a non-object item");
            if (path_key(field(item, "path")) != key)
                continue;
            // Normal upsert: the source PKG labels its matching installed game
            // on every opening, while unrelated metadata/entries stay intact.
            bool changed = false;
            if (field(item, "label") != game.title)
            {
                set_string(item, "label", game.title);
                changed = true;
            }
            if (field(item, "core_name") == "RPCS3" && field(item, "core_path") != game.core_path)
            {
                set_string(item, "core_path", game.core_path);
                changed = true;
            }
            if (!changed)
                return {status::unchanged, {}, {}};
            std::string json;
            emit(json, root);
            if (json.size() >= max_size)
                throw std::runtime_error("updated playlist exceeds the size limit");
            return {status::updated, json + '\n', {}};
        }

        YAML::Node item = YAML::Load(R"({"path":"","label":"","core_path":"","core_name":"RPCS3","crc32":"00000000|crc","db_name":"Sony - PlayStation 3.lpl"})");
        set_string(item, "path", game.path);
        set_string(item, "label", game.title);
        set_string(item, "core_path", game.core_path);
        items.push_back(item);
        if (!exists)
        {
            set_string(root, "default_core_path", game.core_path);
            set_string(root, "default_core_name", "RPCS3");
        }
        std::string json;
        emit(json, root);
        if (json.size() >= max_size)
            throw std::runtime_error("updated playlist exceeds the size limit");
        return {status::updated, json + '\n', {}};
    }
    catch (const std::exception& error)
    {
        return {status::invalid, {}, error.what()};
    }
}
}
