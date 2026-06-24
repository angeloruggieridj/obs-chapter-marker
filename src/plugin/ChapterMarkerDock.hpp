#pragma once
#include <QWidget>
#include <QString>
#include <vector>
#include <obs.h>
#include "RecordingClock.hpp"

class QLineEdit;
class QPushButton;
class QListWidget;
class QLabel;

// Dock content exposing OBS's built-in recording chapter markers: an "Add
// Chapter Marker" button, custom/auto naming, a session log, recording-state
// awareness, visible confirmation (OBS status bar + in-dock status line), and
// export of the session markers to NLE-native formats. Registered with OBS via
// obs_frontend_add_dock_by_id, which wraps this plain widget in its own dock.
class ChapterMarkerDock : public QWidget {
    Q_OBJECT
public:
    explicit ChapterMarkerDock(QWidget* parent = nullptr);
    ~ChapterMarkerDock() override;

    // Driven by frontend recording events from plugin-main.
    void setRecording(bool active);
    void setPaused(bool paused);

    void shutdown();

private slots:
    void onAddChapter();
    void onExport();

private:
    enum class Status { Neutral, Success, Error };

    struct Marker {
        QString name;
        long long elapsedMs;
        QString wallClock;
    };

    void buildUi();
    void updateButtonState();
    void refreshStateStatus();
    void setStatus(const QString& msg, Status level = Status::Neutral);
    void showStatusBarMessage(const QString& msg);
    QString nextAutoName() const;
    double currentFps() const;
    void exportTo(const QString& format); // "youtube" | "edl" | "premiere" | "fcpxml"

    void registerHotkeys();
    void unregisterHotkeys();

    ocm::RecordingClock clock_;
    std::vector<Marker> markers_;
    int counter_ = 1;          // next auto-name index, reset per recording
    bool recording_ = false;
    bool paused_ = false;
    bool obsShutdown_ = false;

    QLineEdit* nameEdit_ = nullptr;
    QPushButton* addButton_ = nullptr;
    QPushButton* exportButton_ = nullptr;
    QListWidget* log_ = nullptr;
    QLabel* status_ = nullptr;
    QLabel* versionLabel_ = nullptr;

    obs_hotkey_id hkAdd_ = OBS_INVALID_HOTKEY_ID;
};
