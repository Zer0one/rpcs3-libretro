// Native schema/registration audit. Does not load RPCS3 or run content.
#ifndef RPCS3_OPTIONS_HEADER
#define RPCS3_OPTIONS_HEADER "rpcs3/libretro/libretro_core_options.h"
#endif
#include RPCS3_OPTIONS_HEADER
#ifndef RPCS3_VISIBILITY_HEADER
#define RPCS3_VISIBILITY_HEADER "rpcs3/libretro/libretro_option_visibility.h"
#endif
#include RPCS3_VISIBILITY_HEADER
#include "rpcs3/libretro/libretro_patch_manager.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

static unsigned version;
static unsigned language;
static size_t count;
static std::vector<std::string> keys;
static std::map<std::string, bool> visibility;
static bool display_available = true;
static size_t display_calls;
static std::map<std::string, bool> shown_sections;
static std::set<std::string> rejected_keys;
static std::vector<std::string> displayed_keys;

static bool enabled(const char* key)
{
    const auto it = shown_sections.find(key);
    return it != shown_sections.end() && it->second;
}

static bool selector(const char* key)
{
    if (!std::strcmp(key, libretro_option_visibility::advanced_sections_key)) return true;
    for (const auto& section : libretro_option_visibility::sections)
        if (!std::strcmp(key, section.selector)) return true;
    return false;
}

static bool advanced(const char* key)
{
    return std::any_of(std::begin(libretro_option_visibility::advanced_keys),
        std::end(libretro_option_visibility::advanced_keys),
        [key](const char* candidate) { return !std::strcmp(key, candidate); });
}

static bool environment(unsigned command, void* data);

template <typename Definition>
static void check_default_labels(const Definition* definitions)
{
    for (auto def = definitions; def && def->key; ++def)
    {
        const auto* source = libretro_option_labels::find(options_us.definitions, def->key);
        if (!source) continue;
        size_t marked = 0;
        for (const auto& value : def->values)
        {
            if (!value.value) break;
            const bool is_marked = value.label && std::string_view(value.label).ends_with(libretro_option_labels::suffix);
            const bool expected = def->values[1].value && std::string_view(value.value) == source->default_value;
            assert(is_marked == expected);
            if (value.label)
            {
                const auto base = libretro_option_labels::undecorated(value.label);
                assert(!base.ends_with(libretro_option_labels::suffix) && !base.ends_with(" (default)"));
            }
            marked += is_marked;
        }
        if (def->values[1].value) assert(marked == 1);
    }
}

static bool update(bool force = false)
{
    return libretro_option_visibility::update(environment, option_defs_us, enabled, force);
}


static bool environment(unsigned command, void* data)
{
    switch (command)
    {
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY:
    {
        ++display_calls;
        const auto& display = *static_cast<retro_core_option_display*>(data);
        assert(!selector(display.key));
        displayed_keys.emplace_back(display.key);
        if (!display_available || rejected_keys.contains(display.key)) return false;
        visibility[display.key] = display.visible;
        return true;
    }
    case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION:
        *static_cast<unsigned*>(data) = version; return true;
    case RETRO_ENVIRONMENT_GET_LANGUAGE:
        *static_cast<unsigned*>(data) = language; return true;
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2_INTL:
    {
        const auto* intl = static_cast<retro_core_options_v2_intl*>(data);
        const auto* defs = intl->us->definitions;
        check_default_labels(defs);
        if (intl->local) check_default_labels(intl->local->definitions);
        for (; defs->key; ++defs) keys.emplace_back(defs->key);
        return true;
    }
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2:
        check_default_labels(static_cast<retro_core_options_v2*>(data)->definitions);
        for (auto d = static_cast<retro_core_options_v2*>(data)->definitions; d->key; ++d) keys.emplace_back(d->key);
        return true;
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL:
        check_default_labels(static_cast<retro_core_options_intl*>(data)->us);
        if (const auto* local = static_cast<retro_core_options_intl*>(data)->local) check_default_labels(local);
        for (auto d = static_cast<retro_core_options_intl*>(data)->us; d->key; ++d) keys.emplace_back(d->key);
        return true;
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS:
        check_default_labels(static_cast<retro_core_option_definition*>(data));
        for (auto d = static_cast<retro_core_option_definition*>(data); d->key; ++d) keys.emplace_back(d->key);
        return true;
    case RETRO_ENVIRONMENT_SET_VARIABLES:
        for (auto d = static_cast<retro_variable*>(data); d->key; ++d)
        {
            keys.emplace_back(d->key);
            const auto& src = options_us.definitions[keys.size() - 1];
            std::string value(d->value);
            const auto start = value.find("; ");
            assert(start != std::string::npos);
            value = value.substr(start + 2);
            assert(value.substr(0, value.find('|')) == src.default_value);
            std::set<std::string> actual, expected;
            size_t at = 0;
            while (at < value.size())
            {
                const auto end = value.find('|', at);
                actual.insert(value.substr(at, end == std::string::npos ? end : end - at));
                if (end == std::string::npos) break;
                at = end + 1;
            }
            for (const auto& raw : src.values)
            {
                if (!raw.value) break;
                expected.insert(raw.value);
            }
            assert(actual == expected); // Never save decorated display labels.
            if (src.values[1].value)
                assert(std::string_view(d->value).substr(0, start).find(" [Default: ") != std::string_view::npos);
            else
                assert(std::string_view(d->value).substr(0, start) == src.desc);
        }
        return true;
    default: return false;
    }
}

int main(int argc, char** argv)
{
    const bool contract = argc == 2 && std::string(argv[1]) == "--contract";
    std::set<std::string> unique, completed_groups;
    std::string previous;
    for (auto d = option_defs_us; d->key; ++d)
    {
        ++count;
        assert(unique.insert(d->key).second);
        bool default_found = false;
        std::string values;
        for (const auto& v : d->values)
        {
            if (!v.value) break;
            default_found |= std::string(v.value) == d->default_value;
            values += std::string(v.value) + '|';
        }
        assert(default_found);
        if (contract)
        {
            std::cout << d->key << '\t' << d->default_value << '\t' << values << '\n';
            continue;
        }
        assert(std::strcmp(d->key, "rpcs3_show_advanced_options"));
        if (!std::strcmp(d->key, libretro_option_visibility::advanced_sections_key))
        {
            assert(!d->category_key && !d->desc_categorized);
            assert(!std::strcmp(d->default_value, "disabled"));
            assert(!std::strcmp(d->values[0].value, "disabled"));
            assert(!std::strcmp(d->values[1].value, "enabled"));
            continue;
        }
        if (selector(d->key))
        {
            assert(!std::strcmp(d->default_value, "disabled"));
            assert(!std::strcmp(d->desc_categorized, "Show Advanced Options"));
            assert(!std::strcmp(d->values[0].value, "disabled"));
            assert(!std::strcmp(d->values[1].value, "enabled"));
            assert(!d->values[2].value);
            assert(!std::strcmp(libretro_option_visibility::section_for(d->category_key)->selector, d->key));
            assert((d + 1)->key && !std::strcmp(d->category_key, (d + 1)->category_key));
            assert(advanced((d + 1)->key));
        }
        const retro_core_option_v2_category* category = option_cats_us;
        while (category->key && std::string(category->key) != d->category_key) ++category;
        assert(category->key && d->desc_categorized);
        assert(std::string(d->desc).starts_with(std::string(category->desc) + ": "));
        if (previous != category->key)
        {
            if (!previous.empty()) completed_groups.insert(previous);
            assert(!completed_groups.contains(category->key));
            previous = category->key;
        }
    }
    if (contract) return 0;
    // Runtime defaults and localized definitions must agree with the English
    // schema, including a default other than the first value and title changes.
    retro_core_option_v2_definition sample[3]{};
    sample[0].key = "dynamic_patch";
    sample[0].desc = "Dynamic Patch";
    sample[0].category_key = "patches";
    sample[0].values[0] = {"0", "Zero"};
    sample[0].values[1] = {"1", "One (default)"}; // Normalize pre-existing lowercase marker.
    sample[0].default_value = "1";
    sample[1].key = "patch_toggle";
    sample[1].desc = "Patch Toggle";
    sample[1].category_key = "patches";
    sample[1].values[0] = {"disabled", nullptr};
    sample[1].values[1] = {"enabled", nullptr};
    sample[1].default_value = "disabled";
    auto translated = sample[0];
    translated.default_value = "0"; // Stale translated default must be ignored.
    translated.values[1].label = "Uno";
    retro_core_option_v2_definition local[] = {translated, {}};
    libretro_option_labels::mark_defaults(sample);
    libretro_option_labels::mark_defaults(local, sample);
    assert(std::string_view(sample[0].values[1].label) == "One (Default)");
    assert(std::string_view(sample[1].values[0].label) == "Disabled (Default)");
    assert(std::string_view(local[0].values[1].label) == "Uno (Default)");
    assert(std::string_view(local[0].values[0].label) == "Zero");
    const char* stable_label = sample[0].values[1].label;
    libretro_option_labels::mark_defaults(sample);
    assert(stable_label == sample[0].values[1].label); // No duplicated suffix/storage.
    sample[0].default_value = "0"; // Platform-selected or per-title default change.
    libretro_option_labels::mark_defaults(sample);
    libretro_option_labels::mark_defaults(local, sample);
    assert(std::string_view(sample[0].values[0].label) == "Zero (Default)");
    assert(std::string_view(sample[0].values[1].label) == "One");
    assert(std::string_view(local[0].values[0].label) == "Zero (Default)");
    assert(std::string_view(local[0].values[1].label) == "Uno");
    assert(std::string_view(stable_label) == "One (Default)"); // Prior publication remains valid.
    assert(std::string_view(libretro_option_labels::legacy_description(sample[0], 0)) == "Dynamic Patch [Default: Zero]");
    assert(std::string_view(sample[0].values[0].value) == "0");
    // Return to a missing translated value label: use the English display label.
    local[0].values[0].label = nullptr;
    libretro_option_labels::mark_defaults(local, sample);
    assert(std::string_view(local[0].values[0].label) == "Zero (Default)");
    // The gate stays above everything it reveals, while ordinary controls
    // remain above the gate, in both mixed sections.
    for (const auto& section : libretro_option_visibility::sections)
    {
        bool gate_seen = false;
        size_t targets = 0;
        for (auto d = option_defs_us; d->key; ++d)
        {
            if (!d->category_key || std::strcmp(d->category_key, section.category)) continue;
            if (!std::strcmp(d->key, section.selector))
            {
                assert(!gate_seen);
                gate_seen = true;
            }
            else if (advanced(d->key))
            {
                assert(gate_seen);
                ++targets;
            }
            else
                assert(!gate_seen);
        }
        assert(gate_seen && targets > 0);
    }
    // Only-advanced sections are last and visibly marked. The same production
    // ordering helper must keep a game-dependent Patch Manager before them.
    std::vector<retro_core_option_v2_definition> mixed;
    std::vector<retro_core_option_v2_category> categories;
    for (auto d = option_defs_us; d->key; ++d) mixed.push_back(*d);
    bool reached_advanced = false;
    for (auto category = option_cats_us; category->key; ++category)
    {
        const bool only = libretro_option_visibility::advanced_only(category->key, option_defs_us);
        assert(only == (std::string_view(category->key) == "scheduling" ||
            std::string_view(category->key) == "timing" || std::string_view(category->key) == "diagnostics"));
        if (only)
            assert(!libretro_option_visibility::section_for(category->key));
        assert(std::string_view(category->desc).ends_with(" (Advanced)") == only);
        assert(!reached_advanced || only);
        reached_advanced |= only;
        categories.push_back(*category);
    }
    retro_core_option_v2_definition patch{};
    patch.key = "test_patch";
    patch.category_key = "patches";
    mixed.push_back(patch);
    categories.push_back({"patches", "Patch Manager", "Synthetic game-dependent category"});
    libretro_option_visibility::move_advanced_sections_last(mixed, categories, option_defs_us);
    const std::vector<std::string> expected = {
        "system", "cpu", "gpu", "audio", "network", "interface", "patches", "scheduling", "timing", "diagnostics"};
    assert(categories.size() == expected.size());
    for (size_t i = 0; i < expected.size(); ++i) assert(categories[i].key == expected[i]);
    std::vector<std::string> mixed_groups;
    for (const auto& def : mixed)
        if (def.category_key && (mixed_groups.empty() || mixed_groups.back() != def.category_key))
            mixed_groups.emplace_back(def.category_key);
    assert(mixed_groups == expected);
    const auto root_switch = std::find_if(mixed.begin(), mixed.end(), [](const auto& def)
    {
        return !std::strcmp(def.key, libretro_option_visibility::advanced_sections_key);
    });
    const auto patch_switch = std::find_if(mixed.begin(), mixed.end(), [](const auto& def)
    {
        return def.category_key && !std::strcmp(def.category_key, "patches");
    });
    const auto first_advanced = std::find_if(mixed.begin(), mixed.end(), [](const auto& def)
    {
        return libretro_option_visibility::advanced_only(def.category_key, option_defs_us);
    });
    assert(patch_switch < root_switch && root_switch < first_advanced);
    for (unsigned v : {2u, 1u, 0u})
        for (unsigned l : {unsigned(RETRO_LANGUAGE_ENGLISH), unsigned(RETRO_LANGUAGE_ITALIAN)})
        {
            version = v; language = l; keys.clear(); bool categories = false;
            libretro_set_core_options(environment, &categories);
            assert(keys.size() == count && categories == (v == 2));
            for (size_t i = 0; i < count; ++i) assert(keys[i] == option_defs_us[i].key);
        }
#ifndef HAVE_NO_LANGEXTRA
    for (auto d = options_it.definitions; d->key; ++d)
        if (d->desc) assert(d->desc_categorized && !std::strcmp(d->desc, d->desc_categorized));
#endif
    // The always-present Patch Manager uses a truthful informational item when
    // empty, never a fake patch switch. Exercise both lifecycle states through
    // the same API 2/1/0 registration path used for actual patch definitions.
    const auto fixed_options = options_us;
    for (const bool title_loaded : {false, true})
    {
        const auto status = libretro_patches::unavailable_status(title_loaded);
        assert(std::string_view(status.key) == "rpcs3_patch_status");
        assert(std::string_view(status.category_key) == "patches");
        assert(!selector(status.key));
        assert(status.values[0].value && !status.values[1].value);
        assert(status.default_value == status.values[0].value);
        assert(std::string_view(status.values[0].label) == (title_loaded
            ? "No patches available for this title" : "Load a title to check for patches"));
        mixed.clear();
        categories.clear();
        for (auto d = option_defs_us; d->key; ++d) mixed.push_back(*d);
        for (auto c = option_cats_us; c->key; ++c) categories.push_back(*c);
        mixed.push_back(status);
        mixed.push_back(sample[0]);
        mixed.push_back(sample[1]);
        categories.push_back({"patches", "Patch Manager", "Patch availability"});
        libretro_option_visibility::move_advanced_sections_last(mixed, categories, option_defs_us);
        mixed.push_back({});
        categories.push_back({});
        options_us.definitions = mixed.data();
        options_us.categories = categories.data();
        for (unsigned v : {2u, 1u, 0u})
            for (unsigned l : {unsigned(RETRO_LANGUAGE_ENGLISH), unsigned(RETRO_LANGUAGE_ITALIAN)})
            {
                version = v;
                language = l;
                keys.clear();
                bool supported = false;
                libretro_set_core_options(environment, &supported);
                assert(keys.size() == count + 3);
                assert(std::count(keys.begin(), keys.end(), status.key) == 1);
                for (size_t i = 0; i < keys.size(); ++i) assert(keys[i] == mixed[i].key);
            }
    }
    options_us = fixed_options;
    const auto* xfloat = libretro_option_labels::find(option_defs_us, "rpcs3_spu_xfloat_accuracy");
    assert(xfloat && std::string_view(xfloat->values[1].label) == "Approximate (Default)");
    const auto* aspect = libretro_option_labels::find(option_defs_us, "rpcs3_aspect_ratio");
    assert(aspect && std::string_view(aspect->values[0].label) == "16:9 (Default)");
    // Selectors precede their targets, remain reachable, and only control keys
    // belonging to their own section. No emulation option is written here.
    for (const auto& section : libretro_option_visibility::sections)
        assert(unique.contains(section.selector));
    display_available = false;
    assert(!update(true));
    assert(visibility.empty());
    display_available = true;
    assert(update());
    std::set<std::string> advanced;
    std::map<std::string, size_t> section_sizes;
    for (const char* key : libretro_option_visibility::advanced_keys)
    {
        assert(unique.contains(key) && advanced.insert(key).second);
        assert(visibility.contains(key));
        const auto* def = libretro_option_visibility::definition_for(option_defs_us, key);
        const auto* section = libretro_option_visibility::section_for(def->category_key);
        assert(!selector(key));
        if (section)
        {
            assert(!visibility.at(key));
            ++section_sizes[section->selector];
        }
        else
        {
            assert(libretro_option_visibility::advanced_only(def->category_key, option_defs_us));
            assert(!visibility.at(key));
        }
    }
    assert(visibility.size() == advanced.size());
    const auto unchanged = display_calls;
    assert(!update());
    assert(display_calls == unchanged);
    for (const auto& section : libretro_option_visibility::sections)
    {
        assert(section_sizes[section.selector] > 0);
        shown_sections[section.selector] = true;
        displayed_keys.clear();
        assert(update());
        assert(displayed_keys.size() == section_sizes[section.selector]);
        for (const auto& key : displayed_keys)
            assert(!std::strcmp(libretro_option_visibility::definition_for(option_defs_us, key.c_str())->category_key, section.category));
        for (const auto& [key, visible] : visibility)
        {
            const auto* owner = libretro_option_visibility::section_for(
                libretro_option_visibility::definition_for(option_defs_us, key.c_str())->category_key);
            assert(visible == enabled(owner ? owner->selector : libretro_option_visibility::advanced_sections_key));
        }
    }
    shown_sections[libretro_option_visibility::sections[0].selector] = false;
    assert(update());
    for (const auto& [key, visible] : visibility)
    {
        const auto* owner = libretro_option_visibility::section_for(
            libretro_option_visibility::definition_for(option_defs_us, key.c_str())->category_key);
        assert(visible == enabled(owner ? owner->selector : libretro_option_visibility::advanced_sections_key));
    }
    // The root gate acts only on dedicated advanced sections, independently
    // of the two mixed-section switches, and is never hidden itself.
    shown_sections[libretro_option_visibility::advanced_sections_key] = true;
    displayed_keys.clear();
    assert(update());
    size_t dedicated = 0;
    for (const auto& [key, visible] : visibility)
    {
        const auto* def = libretro_option_visibility::definition_for(option_defs_us, key.c_str());
        const bool only = libretro_option_visibility::advanced_only(def->category_key, option_defs_us);
        if (only) ++dedicated;
        const auto* owner = libretro_option_visibility::section_for(def->category_key);
        assert(visible == enabled(owner ? owner->selector : libretro_option_visibility::advanced_sections_key));
    }
    assert(displayed_keys.size() == dedicated && dedicated > 0);
    shown_sections[libretro_option_visibility::advanced_sections_key] = false;
    displayed_keys.clear();
    assert(update());
    assert(displayed_keys.size() == dedicated);
    for (const auto& [key, visible] : visibility)
    {
        const auto* owner = libretro_option_visibility::section_for(
            libretro_option_visibility::definition_for(option_defs_us, key.c_str())->category_key);
        assert(visible == (owner && enabled(owner->selector)));
    }
    shown_sections[libretro_option_visibility::advanced_sections_key] = true;
    assert(update());
    displayed_keys.clear();
    assert(update(true));
    assert(displayed_keys.size() == advanced.size());

    // A failed group retries independently; successful groups are not replayed.
    const auto retry_section = libretro_option_visibility::sections[1];
    for (const auto& key : advanced)
        if (!std::strcmp(libretro_option_visibility::definition_for(option_defs_us, key.c_str())->category_key, retry_section.category))
        {
            rejected_keys.insert(key);
            break;
        }
    assert(!rejected_keys.empty());
    assert(update(true));
    rejected_keys.clear();
    displayed_keys.clear();
    assert(update());
    assert(displayed_keys.size() == section_sizes[retry_section.selector]);
    for (const auto& key : displayed_keys)
        assert(!std::strcmp(libretro_option_visibility::definition_for(option_defs_us, key.c_str())->category_key, retry_section.category));
    assert(!update());
    assert(!libretro_option_visibility::update(nullptr, option_defs_us, enabled));
    std::cout << advanced.size() << " advanced controls: independent section visibility, stable repeat, forced republication and group-local retry passed\n";
    std::cout << count << " unique options: valid defaults, contiguous groups, selectors above targets, advanced-only sections last with suffix (including dynamic patches), API 2/1/0 and Italian fallback passed\n";
    std::cout << "Patch Manager status: always-present category, no-title/no-patch single-value information and API 2/1/0 registration passed\n";
    std::cout << "Default labels: all adjustable controls and dynamic patches, translated/changed defaults, stable republication, unchanged saved contract and API 0 description fallback passed\n";
}
