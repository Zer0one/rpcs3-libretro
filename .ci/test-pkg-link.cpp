#include "rpcs3/libretro/libretro_pkg_link.h"
#include "rpcs3/libretro/libretro_core_options.h"
#include <cassert>
#include <iostream>
#include <cstring>

static unsigned options_version;
static unsigned registered;

static void check_option(const retro_core_option_v2_definition* definitions)
{
    for (auto def = definitions; def->key; ++def)
    {
        if (std::strcmp(def->key, "rpcs3_pkg_launcher")) continue;
        ++registered;
        assert(!std::strcmp(def->category_key, "system"));
        assert(!std::strcmp(def->default_value, "enabled"));
        assert(!std::strcmp(def->values[0].value, "enabled"));
        assert(!std::strcmp(def->values[1].value, "disabled"));
        assert(!def->values[2].value);
    }
}

static bool environment(unsigned cmd, void* data)
{
    if (cmd == RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION)
    {
        *static_cast<unsigned*>(data) = options_version;
        return true;
    }
    if (cmd == RETRO_ENVIRONMENT_GET_LANGUAGE)
    {
        *static_cast<unsigned*>(data) = RETRO_LANGUAGE_ITALIAN;
        return true;
    }
    if (cmd == RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2_INTL)
    {
        check_option(static_cast<retro_core_options_v2_intl*>(data)->us->definitions);
        return true;
    }
    if (cmd == RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL)
    {
        for (auto def = static_cast<retro_core_options_intl*>(data)->us; def->key; ++def)
        {
            if (std::strcmp(def->key, "rpcs3_pkg_launcher")) continue;
            ++registered;
            assert(!std::strcmp(def->default_value, "enabled"));
            assert(!std::strcmp(def->values[0].value, "enabled"));
            assert(!std::strcmp(def->values[1].value, "disabled"));
            assert(!def->values[2].value);
        }
        return true;
    }
    if (cmd == RETRO_ENVIRONMENT_SET_VARIABLES)
    {
        for (auto def = static_cast<retro_variable*>(data); def->key; ++def)
        {
            if (std::strcmp(def->key, "rpcs3_pkg_launcher")) continue;
            ++registered;
            assert(std::string_view(def->value).ends_with("; enabled|disabled"));
        }
        return true;
    }
    return false;
}

int main()
{
    using namespace libretro_pkg;
    const package daytona{"NPEB00630", "EP0177-NPEB00630_00-DAYTONAUSA000000", "HG", true, true, false};
    const installation installed{daytona.title_id, daytona.content_id, true};
    assert(select(daytona, {}) == route::install);
    assert(select(daytona, installed) == route::launch);
    assert(select(daytona, installed, false) == route::install);
    assert(select(daytona, {daytona.title_id, daytona.content_id, false}) == route::install);
    assert(select(daytona, {"NPUB30493", "UP0177-NPUB30493_00-DAYTONAUSA000000", true}) == route::conflict);
    assert(select(daytona, {"NPUB30493", {}, true}, false) == route::install);
    assert(select(daytona, {daytona.title_id, "EP0177-NPEB00630_00-ANOTHEREDITION00", true}) == route::conflict);
    // Installed updates may omit CONTENT_ID; the original base PKG must still launch.
    assert(select(daytona, {daytona.title_id, {}, true}) == route::launch);
    auto update = daytona;
    update.patch = true;
    assert(select(update, installed) == route::install);
    update.patch = false;
    update.category = "GD";
    assert(select(update, installed) == route::install);
    auto dlc = daytona;
    dlc.game_exec = false;
    assert(select(dlc, installed) == route::install);
    auto other_platform = daytona;
    other_platform.ps3 = false;
    assert(select(other_platform, installed) == route::install);
    auto wrong_header = daytona;
    wrong_header.content_id = "UP0177-NPUB30493_00-DAYTONAUSA000000";
    assert(select(wrong_header, installed) == route::install);
    auto malformed = daytona;
    malformed.title_id = "../escape";
    assert(select(malformed, installed) == route::install);
    for (const auto name : {"", ".", "..", "../NPEB00630", "foo/bar", "foo\\bar"})
        assert(!valid_install_directory(name));
    assert(!valid_install_directory(std::string_view("ok\0bad", 6)));
    assert(valid_install_directory("NPEB00630"));
    for (unsigned version : {2u, 1u, 0u})
    {
        options_version = version;
        registered = 0;
        bool categories = false;
        libretro_set_core_options(environment, &categories);
        assert(registered == 1);
        assert(categories == (version == 2));
    }
    std::cout << "PKG launcher routing: base game, existing update, missing executable, edition/region conflicts, DLC, update, platform and path checks passed\n";
    std::cout << "PKG Launcher option: APIs 2/1/0 register one enabled/disabled choice with enabled default (Italian frontend)\n";
}
