// burnoutrevenge - ReXGlue Recompiled Project
//
// Customize your app by overriding virtual hooks from rex::ReXApp.

#pragma once

#include <rex/rex_app.h>

#include <filesystem>

class BurnoutrevengeApp : public rex::ReXApp {
 public:
  using rex::ReXApp::ReXApp;

  static std::unique_ptr<rex::ui::WindowedApp> Create(
      rex::ui::WindowedAppContext& ctx) {
    return std::unique_ptr<BurnoutrevengeApp>(new BurnoutrevengeApp(ctx, "burnoutrevenge",
        PPCImageConfig));
  }

  // Resolve the game data root without requiring --game_data_root on every
  // launch. `paths.game_data_root` already reflects a --game_data_root CLI
  // override (or a `game_data_root` cvar set before this point) by the time
  // this runs - the .toml config file itself loads too late in ReXApp's
  // startup sequence to reach here, so a toml-only override won't apply.
  // Only fall back to the relative default when nothing else provided one,
  // so both the CLI override and a plain `<exe dir>/game/` extraction work
  // out of the box on any machine.
  void OnConfigurePaths(rex::PathConfig& paths) override {
    auto cwd = std::filesystem::current_path();
    std::error_code ec;

    paths.user_data_root = cwd / "user";
    paths.cache_root = cwd / "cache";

    if (paths.game_data_root.empty()) {
      paths.game_data_root = std::filesystem::weakly_canonical(cwd / "game", ec);
    }
    paths.update_data_root = paths.game_data_root;

    std::filesystem::create_directories(paths.user_data_root, ec);
    std::filesystem::create_directories(paths.cache_root, ec);
  }

  // Override virtual hooks for customization:
  // void OnPostInitLogging() override {}
  // void OnPreSetup(rex::RuntimeConfig& config) override {}
  // void OnLoadXexImage(std::string& xex_image) override {}
  // void OnPostLoadXexImage() override {}
  // void OnPostSetup() override {}
  // void OnCreateDialogs(rex::ui::ImGuiDrawer* drawer) override {}
  // std::unique_ptr<rex::ui::ImGuiDialog> CreateAchievementsOverlay() override;
  // std::unique_ptr<rex::ui::AchievementNotificationDialog>
  // CreateAchievementNotificationDialog() override;
  // void OnShutdown() override {}
};
