#include "rpcs3/libretro/libretro_content_keys.h"

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>

int main(int argc, char** argv)
{
    assert(argc == 2);
    namespace fs = std::filesystem;
    using namespace libretro_content_keys;
    const fs::path root(argv[1]);
    assert(!fs::exists(root));
    const auto pkg = (root / "pkg").string();
    const auto other = (root / "other-pkg").string();
    const auto data = (root / "system/rpcs3/data").string();
    const std::string id = "EP0177-NPEB00630_00-DAYTONAUSA000000";
    const std::string sidecar = join(pkg, id + ".rap");
    const std::string central = join(data, "rap/" + id + ".rap");
    const auto exists = [](const std::string& p) { return fs::is_regular_file(p); };
    const auto make_file = [](const std::string& p)
    {
        fs::create_directories(fs::path(p).parent_path());
        // Synthetic bytes only, not a game license or disc key.
        std::ofstream(p, std::ios::binary).write("0123456789abcdef", 16);
    };

    make_file(central);
    make_file(sidecar);
    assert(find_rap(pkg, data, id, exists) == sidecar);
    assert(find_rap(other, data, id, exists) == central);
    assert(find_rap({}, data, id, exists) == central); // Direct PARAM.SFO boot.
    assert(find_rap(pkg, data, "EP0177-NPEB00630_00-ANOTHERLICENSE00", exists).empty());
    assert(!fs::exists(root / "system/rpcs3/dev_hdd0")); // Lookup never imports.
    fs::rename(sidecar, join(pkg, id + ".rAp"));
    assert(fs::equivalent(find_rap(pkg, data, id, exists), join(pkg, id + ".rAp")));
    fs::remove(join(pkg, id + ".rAp"));
    assert(find_rap(pkg, data, id, exists) == central);
    fs::remove(central);
    assert(find_rap(pkg, data, id, exists).empty());

    set_pkg_directory(pkg);
    assert(pkg_directory() == pkg);
    // Reset does not alter the context. Loading another package replaces it.
    set_pkg_directory(other);
    assert(pkg_directory() == other);
    set_pkg_directory({});
    assert(pkg_directory().empty());

    const std::string image = join(pkg, "MotorStorm (Europe).iso");
    const std::string legacy = join(data, "redump/");
    const auto paths = iso_paths(image, data, legacy);
    assert(paths.size() == 8);
    assert(paths[0] == join(pkg, "MotorStorm (Europe).dkey"));
    assert(paths[1] == join(pkg, "MotorStorm (Europe).key"));
    assert(paths[2] == join(data, "key/MotorStorm (Europe).dkey"));
    assert(paths[3] == join(data, "key/MotorStorm (Europe).key"));
    assert(paths[4] == join(data, "dkey/MotorStorm (Europe).dkey"));
    assert(paths[5] == join(data, "dkey/MotorStorm (Europe).key"));
    assert(paths[6] == join(data, "redump/MotorStorm (Europe).dkey"));
    assert(paths[7] == join(data, "redump/MotorStorm (Europe).key"));
    for (const auto& path : paths) make_file(path);
    for (std::size_t first = 0; first < paths.size(); ++first)
    {
        auto found = std::find_if(paths.begin(), paths.end(), exists);
        assert(found != paths.end() && *found == paths[first]);
        fs::remove(paths[first]);
    }
    const auto windows = iso_paths("C:\\games.v1\\MotorStorm.ISO", "D:\\rpcs3\\data\\", "D:\\rpcs3\\data\\redump\\");
    assert(windows[0] == "C:\\games.v1\\MotorStorm.dkey");
    assert(windows[2] == "D:\\rpcs3\\data\\key/MotorStorm.dkey");
    const auto relative = iso_paths("Game.iso", data, legacy);
    assert(relative[0] == "Game.dkey");
    assert(iso_directories(data, legacy).size() == 3);
    fs::remove_all(root);
    std::cout << "Content-key lookup passed: RAP sidecar/central priority, exact ID, case handling, no import, context replacement/clear, ISO .dkey/.key order, both central folders, legacy and Windows/relative paths\n";
}
