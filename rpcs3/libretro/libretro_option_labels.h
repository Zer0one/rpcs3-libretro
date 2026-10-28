#pragma once

#include "libretro.h"

#include <set>
#include <string>
#include <string_view>
#include <utility>

// Presentation only: never decorate the values exchanged with the frontend or
// saved in .opt files. Derive the marker at publication time so platform and
// per-title defaults use the same source as the frontend's Reset action.
namespace libretro_option_labels
{
    inline constexpr std::string_view suffix = " (Default)";

    inline const char* keep(std::string text)
    {
        // Interned strings remain valid across registration calls, vector
        // rebuilds and title changes; repeated labels share the same storage.
        static std::set<std::string> labels;
        return labels.insert(std::move(text)).first->c_str();
    }

    inline std::string_view undecorated(std::string_view label)
    {
        // Older definitions already used either capitalization in a few labels.
        return label.ends_with(suffix) || label.ends_with(" (default)")
            ? label.substr(0, label.size() - suffix.size()) : label;
    }

    inline const retro_core_option_v2_definition* find(
        const retro_core_option_v2_definition* definitions, std::string_view key)
    {
        for (auto def = definitions; def && def->key; ++def)
            if (key == def->key) return def;
        return nullptr;
    }

    inline void mark_defaults(retro_core_option_v2_definition* definitions,
        const retro_core_option_v2_definition* english = nullptr)
    {
        for (auto def = definitions; def && def->key; ++def)
        {
            // Local defaults are ignored by Libretro; use the published English
            // definition even when a translated definition has an older default.
            const auto* source = english ? find(english, def->key) : def;
            if (!source || !def->values[0].value) continue;
            const char* default_value = source->values[0].value;
            for (const auto& value : source->values)
            {
                if (!value.value) break;
                if (source->default_value && std::string_view(value.value) == source->default_value)
                    default_value = value.value;
            }
            for (auto& value : def->values)
            {
                if (!value.value) break;
                // Remove our previous marker before re-evaluating a changed
                // default; single-value informational rows have no marker.
                if (value.label && undecorated(value.label) != value.label)
                    value.label = keep(std::string(undecorated(value.label)));
                if (!def->values[1].value || !default_value || std::string_view(value.value) != default_value)
                    continue;
                std::string_view label = value.label ? value.label : "";
                if (label.empty())
                    for (const auto& original : source->values)
                    {
                        if (!original.value) break;
                        if (std::string_view(value.value) == original.value && original.label)
                            label = undecorated(original.label);
                    }
                if (label.empty())
                    label = std::string_view(value.value) == "enabled" ? "Enabled" :
                        std::string_view(value.value) == "disabled" ? "Disabled" : value.value;
                value.label = keep(std::string(label) + std::string(suffix));
            }
        }
    }

    inline const char* legacy_description(const retro_core_option_v2_definition& def, size_t default_index)
    {
        // API 0 cannot separate value labels from saved values. Identify the
        // default in the description instead, keeping the value list intact.
        if (!def.desc || !def.values[1].value) return def.desc;
        const auto& value = def.values[default_index];
        return keep(std::string(def.desc) + " [Default: " +
            std::string(undecorated(value.label ? value.label : value.value)) + "]");
    }
}
