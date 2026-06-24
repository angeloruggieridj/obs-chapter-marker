#include "ChapterMarkerDock.hpp"
#include "ChapterName.hpp"
#include "TimeFormat.hpp"
#include "Export.hpp"

#include <obs-module.h>
#include <obs-frontend-api.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QLabel>
#include <QWidget>
#include <QVariant>
#include <QMainWindow>
#include <QStatusBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QStringConverter>
#include <QPoint>
#include <QDateTime>
#include <QMetaObject>
#include <chrono>
#include <vector>

#ifndef OCM_VERSION
#define OCM_VERSION "0.0.0"
#endif

namespace {
int64_t nowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}
} // namespace

ChapterMarkerDock::ChapterMarkerDock(QWidget* parent) : QWidget(parent) {
    setObjectName("ObsChapterMarkerDock");
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
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    // Name row.
    auto* nameRow = new QHBoxLayout();
    auto* nameLabel = new QLabel(obs_module_text("Name.Label"), this);
    nameEdit_ = new QLineEdit(this);
    nameEdit_->setPlaceholderText(nextAutoName());
    nameRow->addWidget(nameLabel);
    nameRow->addWidget(nameEdit_, 1);
    layout->addLayout(nameRow);

    // Add button.
    addButton_ = new QPushButton(obs_module_text("Add.Button"), this);
    addButton_->setMinimumHeight(36);
    connect(addButton_, &QPushButton::clicked, this, &ChapterMarkerDock::onAddChapter);
    connect(nameEdit_, &QLineEdit::returnPressed, this, &ChapterMarkerDock::onAddChapter);
    layout->addWidget(addButton_);

    // Session log.
    layout->addWidget(new QLabel(obs_module_text("Log.Label"), this));
    log_ = new QListWidget(this);
    layout->addWidget(log_, 1);

    // Export.
    exportButton_ = new QPushButton(obs_module_text("Export.Button"), this);
    connect(exportButton_, &QPushButton::clicked, this, &ChapterMarkerDock::onExport);
    layout->addWidget(exportButton_);

    // Status line.
    status_ = new QLabel(this);
    status_->setWordWrap(true);
    layout->addWidget(status_);

    versionLabel_ = new QLabel(QString("v%1").arg(OCM_VERSION), this);
    versionLabel_->setStyleSheet("color: gray; font-size: 10px;");
    versionLabel_->setAlignment(Qt::AlignRight);
    layout->addWidget(versionLabel_);
}

QString ChapterMarkerDock::nextAutoName() const {
    return QString::fromStdString(ocm::resolveChapterName("", counter_));
}

double ChapterMarkerDock::currentFps() const {
    obs_video_info ovi;
    if (obs_get_video_info(&ovi) && ovi.fps_den > 0)
        return static_cast<double>(ovi.fps_num) / static_cast<double>(ovi.fps_den);
    return 30.0;
}

void ChapterMarkerDock::setRecording(bool active) {
    if (active && !recording_) {
        clock_.start(nowMs());
        counter_ = 1;       // restart auto numbering for the new recording
        markers_.clear();   // markers belong to a single recording session
        log_->clear();
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
    exportButton_->setEnabled(!markers_.empty());
    refreshStateStatus();
}

// Always reflect the current recording state on the status line (neutral
// colour). Called on every state transition so the label never goes stale.
void ChapterMarkerDock::refreshStateStatus() {
    if (!recording_)
        setStatus(obs_module_text("Status.NotRecording"), Status::Neutral);
    else if (paused_)
        setStatus(obs_module_text("Status.Paused"), Status::Neutral);
    else
        setStatus(obs_module_text("Status.Ready"), Status::Neutral);
}

void ChapterMarkerDock::onAddChapter() {
    if (!recording_) {
        setStatus(obs_module_text("Status.NotRecording"), Status::Error);
        return;
    }

    const std::string name =
        ocm::resolveChapterName(nameEdit_->text().toStdString(), counter_);

    if (!obs_frontend_recording_add_chapter(name.c_str())) {
        setStatus(obs_module_text("Status.AddFailed"), Status::Error);
        showStatusBarMessage(obs_module_text("Status.AddFailed"));
        return;
    }

    const long long elapsedMs = clock_.elapsedMs(nowMs());
    const QString elapsed = QString::fromStdString(ocm::formatHms(elapsedMs));
    const QString wall = QDateTime::currentDateTime().toString("HH:mm:ss");
    const QString qname = QString::fromStdString(name);

    markers_.push_back({qname, elapsedMs, wall});
    log_->addItem(QString("%1 — %2 (%3)").arg(qname, elapsed, wall));
    log_->scrollToBottom();

    ++counter_;
    nameEdit_->clear();
    nameEdit_->setPlaceholderText(nextAutoName());
    exportButton_->setEnabled(true);

    const QString confirm =
        QString(obs_module_text("Status.Added")).arg(qname).arg(elapsed);
    setStatus(confirm, Status::Success);
    showStatusBarMessage(confirm);

    blog(LOG_INFO, "[obs-chapter-marker] chapter added: \"%s\" @ %s",
         name.c_str(), elapsed.toUtf8().constData());
}

void ChapterMarkerDock::onExport() {
    if (markers_.empty()) return;
    QMenu menu(this);
    connect(menu.addAction(obs_module_text("Export.YouTube")), &QAction::triggered,
            this, [this] { exportTo("youtube"); });
    connect(menu.addAction(obs_module_text("Export.Resolve")), &QAction::triggered,
            this, [this] { exportTo("edl"); });
    connect(menu.addAction(obs_module_text("Export.Premiere")), &QAction::triggered,
            this, [this] { exportTo("premiere"); });
    connect(menu.addAction(obs_module_text("Export.FinalCut")), &QAction::triggered,
            this, [this] { exportTo("fcpxml"); });
    menu.exec(exportButton_->mapToGlobal(QPoint(0, exportButton_->height())));
}

void ChapterMarkerDock::exportTo(const QString& format) {
    if (markers_.empty()) return;

    std::vector<ocm::MarkerEntry> entries;
    entries.reserve(markers_.size());
    for (const auto& m : markers_)
        entries.push_back({m.name.toStdString(), m.elapsedMs});

    const double fps = currentFps();
    std::string content;
    QString defName, filter;
    if (format == "youtube") {
        content = ocm::exportYouTube(entries);
        defName = "chapters.txt"; filter = "Text (*.txt)";
    } else if (format == "edl") {
        content = ocm::exportEdl(entries, fps);
        defName = "markers.edl"; filter = "EDL (*.edl)";
    } else if (format == "premiere") {
        content = ocm::exportPremiereCsv(entries, fps);
        defName = "markers.csv"; filter = "CSV (*.csv)";
    } else { // fcpxml
        content = ocm::exportFcpxml(entries, fps);
        defName = "markers.fcpxml"; filter = "FCPXML (*.fcpxml)";
    }

    const QString path = QFileDialog::getSaveFileName(
        this, obs_module_text("Export.Button"), defName, filter);
    if (path.isEmpty()) return; // user cancelled

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        setStatus(obs_module_text("Export.Failed"), Status::Error);
        showStatusBarMessage(obs_module_text("Export.Failed"));
        return;
    }
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << QString::fromStdString(content);
    file.close();

    const QString msg =
        QString(obs_module_text("Export.Done")).arg(QFileInfo(path).fileName());
    setStatus(msg, Status::Success);
    showStatusBarMessage(msg);
    blog(LOG_INFO, "[obs-chapter-marker] exported %d markers to %s",
         static_cast<int>(markers_.size()), path.toUtf8().constData());
}

void ChapterMarkerDock::setStatus(const QString& msg, Status level) {
    if (!status_) return;
    status_->setText(msg);
    switch (level) {
    case Status::Success: status_->setStyleSheet("color: #5cb85c;"); break;
    case Status::Error:   status_->setStyleSheet("color: #d9534f;"); break;
    case Status::Neutral: default: status_->setStyleSheet(QString()); break;
    }
}

void ChapterMarkerDock::showStatusBarMessage(const QString& msg) {
    // OBS's status bar is OBSBasicStatusBar, which OVERRIDES showMessage (a
    // non-virtual QStatusBar method) to render into its own message label; the
    // base QStatusBar message area is collapsed to zero width by a permanent
    // widget, so a plain QStatusBar::showMessage call is invisible. Invoke the
    // override dynamically through the meta-object so the real OBSBasicStatusBar
    // slot runs. Falls back to the base call for a non-OBS QStatusBar.
    auto* mw = static_cast<QMainWindow*>(obs_frontend_get_main_window());
    if (!mw) return;
    QStatusBar* sb = mw->findChild<QStatusBar*>();
    if (!sb) sb = mw->statusBar();
    if (!sb) return;
    if (!QMetaObject::invokeMethod(sb, "showMessage", Qt::DirectConnection,
                                   Q_ARG(QString, msg), Q_ARG(int, 10000))) {
        sb->showMessage(msg, 10000);
    }
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
