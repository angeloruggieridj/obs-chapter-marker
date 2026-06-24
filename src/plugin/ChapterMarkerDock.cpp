#include "ChapterMarkerDock.hpp"
#include "ChapterName.hpp"
#include "TimeFormat.hpp"

#include <obs-module.h>
#include <obs-frontend-api.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QLabel>
#include <QWidget>
#include <QMainWindow>
#include <QStatusBar>
#include <QDateTime>
#include <QMetaObject>
#include <chrono>

#ifndef OCM_VERSION
#define OCM_VERSION "0.0.0"
#endif

namespace {
int64_t nowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}
} // namespace

ChapterMarkerDock::ChapterMarkerDock(QWidget* parent) : QDockWidget(parent) {
    setObjectName("ObsChapterMarkerDock");
    setWindowTitle(obs_module_text("ChapterMarker"));
    buildUi();
    registerHotkeys();

    // Adopt whatever recording state OBS is already in when the dock loads.
    if (obs_frontend_recording_active()) {
        setRecording(true);
        if (obs_frontend_recording_paused()) setPaused(true);
    } else {
        updateButtonState();
    }
}

ChapterMarkerDock::~ChapterMarkerDock() {
    if (!obsShutdown_) unregisterHotkeys();
}

void ChapterMarkerDock::shutdown() {
    // Release libobs resources while libobs is still alive (called on EXIT).
    unregisterHotkeys();
    obsShutdown_ = true;
}

void ChapterMarkerDock::buildUi() {
    auto* root = new QWidget(this);
    auto* layout = new QVBoxLayout(root);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    // Name row.
    auto* nameRow = new QHBoxLayout();
    auto* nameLabel = new QLabel(obs_module_text("Name.Label"), root);
    nameEdit_ = new QLineEdit(root);
    nameEdit_->setPlaceholderText(nextAutoName());
    nameRow->addWidget(nameLabel);
    nameRow->addWidget(nameEdit_, 1);
    layout->addLayout(nameRow);

    // Add button.
    addButton_ = new QPushButton(obs_module_text("Add.Button"), root);
    addButton_->setMinimumHeight(36);
    connect(addButton_, &QPushButton::clicked, this, &ChapterMarkerDock::onAddChapter);
    connect(nameEdit_, &QLineEdit::returnPressed, this, &ChapterMarkerDock::onAddChapter);
    layout->addWidget(addButton_);

    // Session log.
    layout->addWidget(new QLabel(obs_module_text("Log.Label"), root));
    log_ = new QListWidget(root);
    layout->addWidget(log_, 1);

    // Status line.
    status_ = new QLabel(root);
    status_->setWordWrap(true);
    layout->addWidget(status_);

    versionLabel_ = new QLabel(QString("v%1").arg(OCM_VERSION), root);
    versionLabel_->setStyleSheet("color: gray; font-size: 10px;");
    versionLabel_->setAlignment(Qt::AlignRight);
    layout->addWidget(versionLabel_);

    setWidget(root);
}

QString ChapterMarkerDock::nextAutoName() const {
    return QString::fromStdString(ocm::resolveChapterName("", counter_));
}

void ChapterMarkerDock::setRecording(bool active) {
    if (active && !recording_) {
        clock_.start(nowMs());
        counter_ = 1; // restart auto numbering for the new recording
        nameEdit_->setPlaceholderText(nextAutoName());
    } else if (!active && recording_) {
        clock_.stop();
    }
    recording_ = active;
    if (!active) paused_ = false;
    updateButtonState();
}

void ChapterMarkerDock::setPaused(bool paused) {
    if (!recording_) return;
    if (paused && !paused_) clock_.pause(nowMs());
    else if (!paused && paused_) clock_.resume(nowMs());
    paused_ = paused;
    updateButtonState();
}

void ChapterMarkerDock::updateButtonState() {
    addButton_->setEnabled(recording_);
    if (!recording_) {
        setStatus(obs_module_text("Status.NotRecording"));
    } else if (paused_) {
        setStatus(obs_module_text("Status.Paused"));
    } else if (status_->text().isEmpty()) {
        setStatus(obs_module_text("Status.Ready"));
    }
}

void ChapterMarkerDock::onAddChapter() {
    if (!recording_) {
        setStatus(obs_module_text("Status.NotRecording"), true);
        return;
    }

    const std::string name =
        ocm::resolveChapterName(nameEdit_->text().toStdString(), counter_);

    if (!obs_frontend_recording_add_chapter(name.c_str())) {
        setStatus(obs_module_text("Status.AddFailed"), true);
        showStatusBarMessage(obs_module_text("Status.AddFailed"));
        return;
    }

    const QString elapsed = QString::fromStdString(ocm::formatHms(clock_.elapsedMs(nowMs())));
    const QString wall = QDateTime::currentDateTime().toString("HH:mm:ss");
    const QString qname = QString::fromStdString(name);

    log_->addItem(QString("%1 — %2 (%3)").arg(qname, elapsed, wall));
    log_->scrollToBottom();

    ++counter_;
    nameEdit_->clear();
    nameEdit_->setPlaceholderText(nextAutoName());

    const QString confirm =
        QString(obs_module_text("Status.Added")).arg(qname).arg(elapsed);
    setStatus(confirm);
    showStatusBarMessage(confirm);

    blog(LOG_INFO, "[obs-chapter-marker] chapter added: \"%s\" @ %s",
         name.c_str(), elapsed.toUtf8().constData());
}

void ChapterMarkerDock::setStatus(const QString& msg, bool error) {
    if (!status_) return;
    status_->setText(msg);
    status_->setStyleSheet(error ? "color: #d9534f;" : "color: #5cb85c;");
}

void ChapterMarkerDock::showStatusBarMessage(const QString& msg) {
    // Mirror the replay-buffer UX: a transient message in OBS's bottom-left
    // status bar. Fall back silently to the dock status line if the main window
    // is unavailable or not a QMainWindow.
    auto* mw = static_cast<QMainWindow*>(obs_frontend_get_main_window());
    if (mw && mw->statusBar()) mw->statusBar()->showMessage(msg, 4000);
}

// ---- hotkey --------------------------------------------------------------

void ChapterMarkerDock::registerHotkeys() {
    hkAdd_ = obs_hotkey_register_frontend(
        "obs-chapter-marker.add", obs_module_text("Hotkey.Add"),
        [](void* data, obs_hotkey_id, obs_hotkey_t*, bool pressed) {
            if (!pressed) return;
            auto* self = static_cast<ChapterMarkerDock*>(data);
            // Hotkey fires on a libobs thread; marshal to the GUI thread.
            QMetaObject::invokeMethod(self, "onAddChapter", Qt::QueuedConnection);
        },
        this);
}

void ChapterMarkerDock::unregisterHotkeys() {
    if (hkAdd_ != OBS_INVALID_HOTKEY_ID) {
        obs_hotkey_unregister(hkAdd_);
        hkAdd_ = OBS_INVALID_HOTKEY_ID;
    }
}
