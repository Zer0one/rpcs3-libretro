// Synthetic playlist fixtures only: no PKGs, games, licenses or live playlists.
#include "rpcs3/libretro/libretro_installed_playlist.h"
#include "rpcs3/libretro/libretro_core_options.h"
#include "util/yaml.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace pl = libretro_installed_playlist;

int main(int argc, char** argv)
{
    assert(argc == 2);
    const std::filesystem::path output(argv[1]);
    std::filesystem::create_directories(output);
    assert(pl::package_label("/archive/Game (Europe) [PSN].pkg") == "Game (Europe) [PSN]");
    assert(pl::package_label("C:\\archive\\Game.Name.PKG") == "Game.Name");
    assert(pl::package_label("relative.Name.pKg") == "relative.Name");
    assert(pl::package_label("/archive/Game.pkg", "An unrelated frontend name") == "Game");
    assert(pl::package_label("saf://provider/encoded-id.pkg", "Game — 日本語") == "Game — 日本語");
    assert(pl::package_label("saf://provider/encoded-id.pkg", "Game.pkg") == "Game.pkg");
    assert(pl::package_label("/archive/.pkg") == ".pkg");
    assert(pl::package_label("/archive/").empty());
    const pl::entry game{"/system/rpcs3/dev_hdd0/game/NPEB00001/PARAM.SFO",
        pl::package_label("/archive/Gioco \"PSN\" — 日本語\nTitolo.pkg"), "/cores/rpcs3_libretro.dylib"};
    const auto created = pl::update({}, false, game);
    assert(created.state == pl::status::updated && created.error.empty());
    std::ofstream(output / "created.lpl") << created.json;
    const auto first = YAML::Load(created.json);
    assert(first["version"].as<std::string>() == "1.5");
    assert(first["default_core_path"].as<std::string>() == game.core_path);
    assert(first["default_core_name"].as<std::string>() == "RPCS3");
    assert(first["label_display_mode"].as<int>() == 0);
    assert(first["items"].size() == 1);
    assert(first["items"][0]["path"].as<std::string>() == game.path);
    assert(first["items"][0]["label"].as<std::string>() == game.title);
    assert(first["items"][0]["db_name"].as<std::string>() == pl::database);

    assert(first[pl::marker_key.data()].as<std::string>() == pl::marker_value);
    for (const std::string foreign : {
        R"({"version":"1.5","items":[]})",
        R"({"version":"1.5","rpcs3_installed_playlist":"another-core","items":[]})"})
    {
        const auto protected_list = pl::update(foreign, true, game);
        assert(protected_list.state == pl::status::unowned);
        assert(protected_list.json.empty() && !protected_list.error.empty());
    }

    assert(pl::update(created.json, true, game).state == pl::status::unchanged);
    // A recognized installed game is enough to rebuild a deleted list: this
    // updater needs no extraction result or source PKG payload to do so.
    const auto recovery_path = output / "recover-after-collision.lpl";
    const std::string user_list = R"({"version":"1.5","items":[]})";
    std::ofstream(recovery_path) << user_list;
    assert(pl::update(user_list, true, game).state == pl::status::unowned);
    std::ifstream protected_file(recovery_path);
    const std::string protected_bytes((std::istreambuf_iterator<char>(protected_file)), {});
    assert(protected_bytes == user_list);
    protected_file.close();
    std::filesystem::remove(recovery_path);
    const auto recovered = pl::update({}, std::filesystem::exists(recovery_path), game);
    assert(recovered.state == pl::status::updated);
    std::ofstream(recovery_path) << recovered.json;
    assert(pl::update(recovered.json, true, game).state == pl::status::unchanged);
    auto normalized = game;
    normalized.path = "/system/rpcs3/dev_hdd0/game/../game/NPEB00001/PARAM.SFO";
    assert(pl::update(created.json, true, normalized).state == pl::status::unchanged);

    // RA metadata, user labels, save slots, unknown fields and scalar types
    // must survive when appending another installed title.
    const std::string existing = R"({"version":"1.5","rpcs3_installed_playlist":"rpcs3-libretro:psn-installed:1","default_core_path":"DETECT","default_core_name":"DETECT","sort_mode":2,"thumbnail_match_mode":1,"scan_search_recursively":true,"extension":{"number":-1.25e+3,"numeric_string":"42","boolean_string":"true","optional":null,"list":[false,7,"7"]},"items":[{"path":"/other/PARAM.SFO","label":"My Label","core_path":"/custom/core.dylib","core_name":"Custom","entry_slot":4,"runtime_seconds":23,"custom":"keep"}]})";
    const auto appended = pl::update(existing, true, game);
    assert(appended.state == pl::status::updated);
    std::ofstream(output / "preserved.lpl") << appended.json;
    const auto kept = YAML::Load(appended.json);
    assert(kept["sort_mode"].as<int>() == 2);
    assert(kept["thumbnail_match_mode"].as<int>() == 1);
    assert(kept["scan_search_recursively"].as<bool>());
    assert(kept["extension"]["optional"].IsNull());
    assert(kept["items"].size() == 2);
    assert(kept["items"][0]["label"].as<std::string>() == "My Label");
    assert(kept["items"][0]["entry_slot"].as<int>() == 4);
    assert(kept["items"][0]["custom"].as<std::string>() == "keep");

    // Updating the core path retains the PKG label and does not duplicate a game.
    std::string relocated = created.json;
    const auto label = relocated.find("Gioco");
    assert(label != std::string::npos);
    auto relocated_game = game;
    relocated_game.core_path = "/new cores/rpcs3_libretro.dylib";
    const auto refreshed = pl::update(relocated, true, relocated_game);
    assert(refreshed.state == pl::status::updated);
    const auto refreshed_root = YAML::Load(refreshed.json);
    assert(refreshed_root["items"].size() == 1);
    assert(refreshed_root["items"][0]["label"].as<std::string>() == game.title);
    assert(refreshed_root["items"][0]["core_path"].as<std::string>() == relocated_game.core_path);

    const auto fill = pl::update(R"({"version":"1.5","rpcs3_installed_playlist":"rpcs3-libretro:psn-installed:1","items":[{"path":"/system/rpcs3/dev_hdd0/game/NPEB00001/PARAM.SFO","label":"","core_path":"DETECT","core_name":"DETECT","entry_slot":3}]})", true, game);
    assert(fill.state == pl::status::updated);
    const auto filled = YAML::Load(fill.json)["items"][0];
    assert(filled["label"].as<std::string>() == game.title);
    assert(filled["core_path"].as<std::string>() == "DETECT"); // Explicit frontend association survives.
    assert(filled["entry_slot"].as<int>() == 3);

    // The regular update replaces the matching label when a PKG has another
    // filename. There is no special old-name detection or correction pass.
    auto renamed = game;
    renamed.title = pl::package_label("/archive/Renamed Game [USA].pkg");
    const auto updated = pl::update(fill.json, true, renamed);
    assert(updated.state == pl::status::updated);
    const auto updated_root = YAML::Load(updated.json);
    assert(updated_root["items"].size() == 1);
    assert(updated_root["items"][0]["path"].as<std::string>() == game.path);
    assert(updated_root["items"][0]["label"].as<std::string>() == "Renamed Game [USA]");
    assert(updated_root["items"][0]["entry_slot"].as<int>() == 3);
    assert(updated_root["items"][0]["core_path"].as<std::string>() == "DETECT");
    assert(pl::update(updated.json, true, renamed).state == pl::status::unchanged);
    std::ofstream(output / "pkg-filename-label.lpl") << updated.json;

    auto controls = game;
    controls.title = std::string("Line\tBack\\Slash\r") + char(0) + char(0x1b);
    const auto escaped = pl::update({}, false, controls);
    assert(escaped.state == pl::status::updated);
    std::ofstream(output / "escaped.lpl") << escaped.json;
    assert(YAML::Load(escaped.json)["items"][0]["label"].as<std::string>() == controls.title);

    auto uri = game;
    uri.path = "saf://provider/document/game%2FPARAM.SFO";
    const auto uri_created = pl::update({}, false, uri);
    assert(uri_created.state == pl::status::updated);
    assert(pl::update(uri_created.json, true, uri).state == pl::status::unchanged);
    assert(pl::update("\xef\xbb\xbf" + created.json, true, game).state == pl::status::unchanged);

    for (const std::string input : {
        "", "not JSON", "{", "[]", "{}", "{\"version\":\"2.0\",\"items\":[]}",
        "{\"version\":\"1.5\",\"items\":{}}", "{\"version\":\"1.5\",\"rpcs3_installed_playlist\":\"rpcs3-libretro:psn-installed:1\",\"items\":[null]}",
        "{\"version\":\"1.5\",\"rpcs3_installed_playlist\":\"rpcs3-libretro:psn-installed:1\",\"items\":[],\"items\":[]}",
        "{\"version\":\"1.5\",\"rpcs3_installed_playlist\":\"rpcs3-libretro:psn-installed:1\",\"items\":[],\"bad\":.nan}"})
    {
        const auto rejected = pl::update(input, true, game);
        if (rejected.state != pl::status::invalid) std::cerr << "Unexpected accepted fixture: " << input << "\n";
        assert(rejected.state == pl::status::invalid && rejected.json.empty() && !rejected.error.empty());
    }
    assert(pl::update(std::string(pl::max_size + 1, ' '), true, game).state == pl::status::invalid);
    auto missing = game;
    missing.core_path.clear();
    assert(pl::update({}, false, missing).state == pl::status::invalid);

    bool option_found = false;
    for (auto definition = option_defs_us; definition->key; ++definition)
        if (std::string_view(definition->key) == "rpcs3_pkg_installed_playlist")
        {
            assert(!option_found);
            option_found = true;
            assert(std::string_view(definition->category_key) == "system");
            assert(std::string_view(definition->default_value) == "enabled");
            assert(std::string_view(definition->values[0].value) == "enabled");
            assert(std::string_view(definition->values[1].value) == "disabled");
        }
    assert(option_found);
    std::cout << "Installed playlist: PKG filename labels and regular upsert, ownership/collision protection, recreation, Unicode/JSON types, append/preservation, deduplication, URI/core relocation and option schema passed\n";
}
