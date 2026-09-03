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

  // Hardcodes the game data root instead of requiring --game_data_root on
  // every launch. Same pattern as ForzaHorizon2_Recomp/src/forzahorizon2_app.h
  // OnConfigurePaths(). Path confirmed real in PROJECT_STATE.md (Fase 0/4).
  void OnConfigurePaths(rex::PathConfig& paths) override {
    auto cwd = std::filesystem::current_path();
    std::error_code ec;

    paths.user_data_root = cwd / "user";
    paths.cache_root = cwd / "cache";

    std::filesystem::path game_dir =
        "D:\\GAME RECOMP\\Burnout Revenge (Europe) (En,Es,Nl,Sv,Fi)";
    paths.game_data_root = std::filesystem::weakly_canonical(game_dir);
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
