// Native schema/registration audit. Does not load RPCS3 or run content.
#ifndef RPCS3_OPTIONS_HEADER
#define RPCS3_OPTIONS_HEADER "rpcs3/libretro/libretro_core_options.h"
#endif
#include RPCS3_OPTIONS_HEADER
#ifndef RPCS3_VISIBILITY_HEADER
#define RPCS3_VISIBILITY_HEADER "rpcs3/libretro/libretro_option_visibility.h"
#endif
#include RPCS3_VISIBILITY_HEADER
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


static bool environment(unsigned command, void* data)
{
    switch (command)
    {
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY:
    {
        ++display_calls;
        if (!display_available) return false;
        const auto& display = *static_cast<retro_core_option_display*>(data);
        assert(std::strcmp(display.key, "rpcs3_show_advanced_options"));
        visibility[display.key] = display.visible;
        return true;
    }
    case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION:
        *static_cast<unsigned*>(data) = version; return true;
    case RETRO_ENVIRONMENT_GET_LANGUAGE:
        *static_cast<unsigned*>(data) = language; return true;
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2_INTL:
    {
        const auto* defs = static_cast<retro_core_options_v2_intl*>(data)->us->definitions;
        for (; defs->key; ++defs) keys.emplace_back(defs->key);
        return true;
    }
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2:
        for (auto d = static_cast<retro_core_options_v2*>(data)->definitions; d->key; ++d) keys.emplace_back(d->key);
        return true;
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL:
        for (auto d = static_cast<retro_core_options_intl*>(data)->us; d->key; ++d) keys.emplace_back(d->key);
        return true;
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS:
        for (auto d = static_cast<retro_core_option_definition*>(data); d->key; ++d) keys.emplace_back(d->key);
        return true;
    case RETRO_ENVIRONMENT_SET_VARIABLES:
        for (auto d = static_cast<retro_variable*>(data); d->key; ++d)
        {
            keys.emplace_back(d->key);
            const auto& src = option_defs_us[keys.size() - 1];
            std::string value(d->value);
            const auto start = value.find("; ");
            assert(start != std::string::npos);
            value = value.substr(start + 2);
            assert(value.substr(0, value.find('|')) == src.default_value);
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
        if (!std::strcmp(d->key, "rpcs3_show_advanced_options"))
        {
            assert(!d->category_key && !d->desc_categorized);
            assert(!std::strcmp(d->default_value, "disabled"));
            continue;
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
    // Failed environment calls must be retried; all advanced controls return
    // after enabling, without filtering definitions or touching saved values.
    display_available = false;
    assert(!libretro_option_visibility::update(environment, false, true));
    assert(visibility.empty());
    display_available = true;
    assert(libretro_option_visibility::update(environment, false));
    std::set<std::string> advanced;
    for (const char* key : libretro_option_visibility::advanced_keys)
    {
        assert(unique.contains(key) && advanced.insert(key).second);
        assert(visibility.contains(key) && !visibility.at(key));
    }
    assert(visibility.size() == advanced.size());
    const auto unchanged = display_calls;
    assert(!libretro_option_visibility::update(environment, false));
    assert(display_calls == unchanged);
    assert(libretro_option_visibility::update(environment, true));
    for (const auto& [key, visible] : visibility) assert(visible);
    assert(libretro_option_visibility::update(environment, true, true));
    assert(libretro_option_visibility::update(environment, false));
    for (const auto& [key, visible] : visibility) assert(!visible);
    std::cout << advanced.size() << " advanced controls: hide/show, stable repeat, forced republication and retry after unsupported/early environment calls passed\n";
    std::cout << count << " unique options: valid defaults, contiguous functional groups, complete API 2/1/0 registration and Italian label fallback passed\n";
}
