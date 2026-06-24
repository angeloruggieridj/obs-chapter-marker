#include <obs-module.h>
#include <obs-frontend-api.h>
#include <QMainWindow>
#include "ChapterMarkerDock.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-chapter-marker", "en-US")

MODULE_EXPORT const char* obs_module_description(void) {
    return "Chapter Marker - add OBS recording chapter markers from a dock, "
           "with a button, session log, and on-screen confirmation";
}
MODULE_EXPORT const char* obs_module_name(void) { return "Chapter Marker"; }

static ChapterMarkerDock* g_dock = nullptr;
static constexpr const char* DOCK_ID = "obs-chapter-marker-dock";

// Register the dock with OBS. Must happen during obs_module_load (which OBS
// calls from OBSBasic::OBSInit *before* it restores the saved dock layout via
// restoreState) so OBS can restore this dock's position and visibility across
// restarts. Adding it later (e.g. on FINISHED_LOADING) runs after the restore,
// so the dock would reset to hidden every launch.
static void ensure_dock() {
    if (g_dock) return;
    auto* mw = static_cast<QMainWindow*>(obs_frontend_get_main_window());
    if (!mw) return; // no UI (e.g. headless); nothing to dock
    g_dock = new ChapterMarkerDock(mw);
    // add_dock_by_id wraps our plain QWidget in OBS's own dock.
    obs_frontend_add_dock_by_id(DOCK_ID, obs_module_text("ChapterMarker"), g_dock);
    blog(LOG_INFO, "[obs-chapter-marker] dock registered");
}

static void on_frontend_event(enum obs_frontend_event event, void*) {
    switch (event) {
    case OBS_FRONTEND_EVENT_FINISHED_LOADING:
        // Fallback: if the main window was not ready at module load, register now.
        ensure_dock();
        break;
    case OBS_FRONTEND_EVENT_RECORDING_STARTED:
        if (g_dock) g_dock->setRecording(true);
        break;
    case OBS_FRONTEND_EVENT_RECORDING_STOPPED:
        if (g_dock) g_dock->setRecording(false);
        break;
    case OBS_FRONTEND_EVENT_RECORDING_PAUSED:
        if (g_dock) g_dock->setPaused(true);
        break;
    case OBS_FRONTEND_EVENT_RECORDING_UNPAUSED:
        if (g_dock) g_dock->setPaused(false);
        break;
    case OBS_FRONTEND_EVENT_EXIT:
        // Release libobs resources while libobs is still alive, before Qt tears
        // the dock down during shutdown.
        if (g_dock) g_dock->shutdown();
        break;
    default:
        break;
    }
}

bool obs_module_load(void) {
    blog(LOG_INFO, "[obs-chapter-marker] loaded");
    obs_frontend_add_event_callback(on_frontend_event, nullptr);
    ensure_dock(); // before OBS restores the dock layout -> state persists
    return true;
}

void obs_module_unload(void) {
    obs_frontend_remove_event_callback(on_frontend_event, nullptr);
    g_dock = nullptr;
}
