#pragma once

#include <QMainWindow>
#include <QPointer>
#include <QProcess>
#include <QTimer>
#include <QSystemTrayIcon>

#include "recorder.h"


QT_BEGIN_NAMESPACE
namespace Ui { class Application; }
QT_END_NAMESPACE

class Application : public QMainWindow
{
    Q_OBJECT

public:
    Application(QWidget *parent = nullptr);
    ~Application();

    bool isRunning();
    void closeEvent(QCloseEvent *event) override;

private slots:

    void writeSettings();
    void readSettings();

    void onStartClicked();
    void onStopClicked();
    void onDownloadModels();

    void onProcessErrorOccurred(QProcess::ProcessError err);
    void onProcessStateChanged(QProcess::ProcessState st);
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();
    void onClearLog();

    void onHearBeat();
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void onTrayMessageClicked();
    void onFetchMicrophones();

    void onToggleRecording();

    void onQuit();

private:
    void syncUIWithProcessState();
    void syncUIWithRecordingState();
    bool getCurrentMicrophone();
    void onVoiceAvailable(const sf::Int16 *samples, size_t sampleCount);

private:
    Ui::Application *ui;
    QPointer<QProcess> m_currentProcess;
    QAction* m_actionClearLog;
    QMenu* trayIconMenu;
    QAction* m_QuitAction;
    QTimer m_hearbeatTimer;
    QSystemTrayIcon* trayIcon;

    MicrophoneRecorder::Pointer recorder = nullptr;
    bool recordingInProgress = false;

};
