#include <QDebug>
#include <QDir>
#include <QThread>
#include <QSettings>
#include <QSignalBlocker>
#include <QDesktopServices>
#include <QAudioDeviceInfo>
#include <QAudioFormat>
#include <QJsonDocument>
#include <QJsonObject>

#include "application.h"
#include "vls_common.h"
#include "./ui_application.h"


Application::Application(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Application)
{
    ui->setupUi(this);
    ui->leLog->setContextMenuPolicy(Qt::ActionsContextMenu);

    socket = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);

    m_actionClearLog = new QAction(this);
    m_actionClearLog->setText("Clear");
    connect(m_actionClearLog, &QAction::triggered, this, &Application::onClearLog);
    ui->leLog->addAction(m_actionClearLog);

    m_hearbeatTimer.setInterval(1000);
    connect(&m_hearbeatTimer, &QTimer::timeout, this, &Application::onHearBeat);
    m_hearbeatTimer.start(1000);

    syncUIWithProcessState();
    ui->leLog->setReadOnly(true);

    m_QuitAction = new QAction(this);
    m_QuitAction->setText("Quit");
    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(m_QuitAction);
    connect(m_QuitAction, &QAction::triggered, this, &Application::onQuit);
    connect(ui->actionExit, &QAction::triggered, this, &Application::onQuit);
    connect(ui->actionDownload_models, &QAction::triggered, this, &Application::onDownloadModels);

    trayIcon = new QSystemTrayIcon(this);
    connect(trayIcon, &QSystemTrayIcon::activated, this, &Application::onTrayIconActivated);
    trayIcon->setIcon(QIcon(":/icon.ico"));
    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->show();

    setWindowTitle(QString("Vosk Language Server %1.%2").arg(VLS::VLS_VERSION_MAJOR).arg(VLS::VLS_VERSION_MINOR));

    readSettings();
    connect(trayIcon, &QSystemTrayIcon::messageClicked, this, &Application::onTrayMessageClicked);
    trayIcon->showMessage("", "Vls is running in background", QSystemTrayIcon::NoIcon, 300);

    if(ui->autoStartCheckBox->isChecked())
    {
        onStartClicked();
    }

    connect(ui->pbFetchMicrophones, &QPushButton::clicked, this, &Application::onFetchMicrophones);
    onFetchMicrophones();

    connect(ui->pbRecord, &QPushButton::clicked, this, &Application::onToggleRecording);
    connect(ui->pbStart, &QPushButton::clicked, this, &Application::onStartClicked);
    connect(ui->pbStop, &QPushButton::clicked, this, &Application::onStopClicked);

    connect(socket, &QWebSocket::stateChanged, this, &Application::onSocketStateChanged, Qt::QueuedConnection);
    connect(socket, &QWebSocket::textMessageReceived, this, &Application::onTextMessageReceived, Qt::QueuedConnection);
    connect(socket, &QWebSocket::binaryMessageReceived, this, &Application::onBinaryMessageReceived, Qt::QueuedConnection);

    silenceDetectionTimer.setParent(this);
    silenceDetectionTimer.setSingleShot(true);
    silenceDetectionTimer.setInterval(650);
    connect(&silenceDetectionTimer, &QTimer::timeout, this, &Application::onStoppedTalking);

}

Application::~Application()
{
    delete ui;
}

bool Application::isRunning()
{
    return !m_currentProcess.isNull();
}

void Application::writeSettings()
{
    QSettings settings(QSettings::IniFormat,
                       QSettings::UserScope,
                       QApplication::organizationName(),
                       QApplication::applicationName());

    settings.setValue("address", ui->addressLineEdit->text());
    settings.setValue("port", ui->portSpinBox->value());
    settings.setValue("sample_rate", ui->sampleRateSpinBox->value());
    settings.setValue("threads", ui->numThreadsSpinBox->value());
    settings.setValue("max_alternatives", ui->maxAlternativesSpinBox->value());
    settings.setValue("show_words", ui->showWordsCheckBox->isChecked());
    settings.setValue("model_path", ui->modelPathLineEdit->text());

    settings.beginGroup("gui");
    settings.setValue("auto_start", ui->autoStartCheckBox->isChecked());
    settings.endGroup();

    settings.sync();
}

void Application::readSettings()
{
    QSettings settings(QSettings::IniFormat,
                       QSettings::UserScope,
                       QApplication::organizationName(),
                       QApplication::applicationName());

    ui->addressLineEdit->setText(settings.value("address", "0.0.0.0").toString());
    ui->portSpinBox->setValue(settings.value("port", 8080).toInt());
    ui->sampleRateSpinBox->setValue(settings.value("sample_rate", 16000).toInt());
    ui->numThreadsSpinBox->setValue(settings.value("threads", 1).toInt());
    ui->maxAlternativesSpinBox->setValue(settings.value("max_alternatives", 0).toInt());
    ui->showWordsCheckBox->setChecked(settings.value("show_words").toBool());
    ui->modelPathLineEdit->setText(settings.value("model_path").toString());

    settings.beginGroup("gui");
    ui->autoStartCheckBox->setChecked(settings.value("auto_start").toBool());
    settings.endGroup();
}


void Application::onStartClicked()
{
    if(!isRunning())
    {
        m_currentProcess = new QProcess(this);

        m_currentProcess->setWorkingDirectory(QApplication::applicationDirPath());
        connect(m_currentProcess, &QProcess::stateChanged, this, &Application::onProcessStateChanged);
        connect(m_currentProcess, &QProcess::errorOccurred, this, &Application::onProcessErrorOccurred);
        connect(m_currentProcess, &QProcess::readyReadStandardOutput, this, &Application::onReadyReadStandardOutput);
        connect(m_currentProcess, &QProcess::readyReadStandardError, this, &Application::onReadyReadStandardError);

        QStringList args;
        args << "-a" << ui->addressLineEdit->text();
        args << "-p" << QString("%1").arg(ui->portSpinBox->value());
        args << "-t" << QString("%1").arg(ui->numThreadsSpinBox->value());
        args << "-s" << QString("%1").arg(ui->sampleRateSpinBox->value());
        args << "-l" << QString("%1").arg(ui->maxAlternativesSpinBox->value());
        args << "-w" << QString("%1").arg(ui->showWordsCheckBox->isChecked());
        args << "-m" << QDir::toNativeSeparators(ui->modelPathLineEdit->text());

        m_currentProcess->setProgram(QStringLiteral("asr_server.exe"));
        m_currentProcess->setArguments(args);
        m_currentProcess->start();

    } else {
        qDebug() << "asr_server is not running";
    }
    syncUIWithProcessState();
}

void Application::onStopClicked()
{
    if (recordingInProgress)
    {
        onToggleRecording();
    }

    if(isRunning())
    {
        m_currentProcess->disconnect();
        m_currentProcess->kill();
        m_currentProcess->waitForFinished();
        delete m_currentProcess;
        m_currentProcess.clear();
    }
}

void Application::onDownloadModels()
{
    QDesktopServices::openUrl(QUrl("https://alphacephei.com/vosk/models"));
}

void Application::onProcessErrorOccurred(QProcess::ProcessError err)
{

}

void Application::onProcessStateChanged(QProcess::ProcessState st)
{

}

void Application::onReadyReadStandardOutput()
{
    QString out = m_currentProcess->readAllStandardOutput();
    ui->leLog->appendPlainText(out);
}

void Application::onReadyReadStandardError()
{
    QString out = m_currentProcess->readAllStandardError();
    ui->leLog->appendPlainText(out);
}

void Application::onClearLog()
{
    ui->leLog->clear();
}


void Application::onHearBeat()
{
    syncUIWithProcessState();
    syncUIWithRecordingState();
}

void Application::closeEvent(QCloseEvent *event)
{
#ifdef Q_OS_MACOS
    if (!event->spontaneous() || !isVisible()) {
        return;
    }
#endif
    if (trayIcon->isVisible()) {
        hide();
        event->ignore();
    } else {
        writeSettings();
    }
}

void Application::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason)
        {
            case QSystemTrayIcon::Trigger:
            case QSystemTrayIcon::DoubleClick:
                showNormal();
                activateWindow();
                break;
            case QSystemTrayIcon::MiddleClick:
            default:
            ;
    }
}

void Application::onTrayMessageClicked()
{
    showNormal();
    activateWindow();
}

void Application::onFetchMicrophones()
{
    QSignalBlocker blocker(ui->cbMicrophone);

    ui->cbMicrophone->clear();

    // fetch microphones
    foreach (const QAudioDeviceInfo& info, QAudioDeviceInfo::availableDevices(QAudio::Mode::AudioInput))
    {
        ui->cbMicrophone->addItem(info.deviceName());
    }

}

void Application::onToggleRecording()
{
    QString currentMicrophoneName = getCurrentMicrophoneName();
    if (currentMicrophoneName.isEmpty() || currentMicrophoneName.isNull())
        return;

    if (recordingInProgress)
    {
        // shutdown recording
        recordingInProgress = false;
        ui->pbRecord->setText(QStringLiteral("Start recording"));

        socket->close();
        audioIO->stop();

    } else {
        if(QAudioDeviceInfo::availableDevices(QAudio::AudioInput).size() > 0)
        {
            // start recording

            QAudioFormat format;
            format.setSampleRate(8000);
            format.setChannelCount(1);
            format.setSampleSize(16);
            format.setSampleType(QAudioFormat::SignedInt);
            format.setByteOrder(QAudioFormat::LittleEndian);
            format.setCodec("audio/pcm");

            foreach (const QAudioDeviceInfo& info, QAudioDeviceInfo::availableDevices(QAudio::AudioInput))
            {
                if (info.deviceName() == currentMicrophoneName)
                {
                    if (!info.isFormatSupported(format))
                        format = info.nearestFormat(format);
                    break;
                }
            }

            QString url = QString::asprintf("ws://%s:%d",
                              ui->addressLineEdit->text().toStdString().c_str(),
                              ui->portSpinBox->value());
            socket->open(QUrl(url));

            audioInput = new QAudioInput(QAudioDeviceInfo::defaultInputDevice(), format, this);
            audioIO.reset(new MicrophoneAudioDevice(format, [this](const char *data, qint64 len, qreal level) {
                QByteArray buffer(data, len);
                socket->sendBinaryMessage(buffer);
                if (level > 0.003)
                {
                    silenceDetectionTimer.start();
                }
            }));

            recordingInProgress = true;
            audioIO->start();
            audioInput->start(audioIO.data());
            ui->pbRecord->setText(QStringLiteral("Stop recording"));

        }
    }
}

void Application::onTextMessageReceived(const QString &message)
{
    QJsonParseError err;
    QByteArray ba(message.toStdString().c_str());
    QJsonDocument json = QJsonDocument::fromJson(ba, &err);

    if (err.error == QJsonParseError::NoError)
    {
        QJsonObject obj = json.object();
        // find partial
        auto itPartial = obj.find(QStringLiteral("partial"));
        if (itPartial != obj.end())
        {
            QString text = obj.value(QStringLiteral("partial")).toString();
            if (text.isEmpty()) return;

            ui->tePartialResult->setPlainText(text);
            return;
        }

        // find final
        auto it = obj.find(QStringLiteral("text"));
        if (it != obj.end())
        {
            QString text = obj.value(QStringLiteral("text")).toString();
            if (text.isEmpty()) return;

            ui->teFinalResult->setPlainText(text);
            return;
        }

    } else {
        QString r = err.errorString();
        r += "\n";
        r += message;
        ui->tePartialResult->setPlainText(r);
    }
}

void Application::onBinaryMessageReceived(const QByteArray &message)
{

}

void Application::onSocketStateChanged(QAbstractSocket::SocketState state)
{

}

void Application::onStoppedTalking()
{
    // socket->sendTextMessage("{\"eof\" : 1}");
}


void Application::onQuit()
{
    writeSettings();
    qApp->quit();
}

void Application::syncUIWithProcessState()
{
    auto enable_disable_widgets = [this](bool enabled)
    {
        ui->addressLineEdit->setReadOnly(enabled);
        ui->portSpinBox->setReadOnly(enabled);
        ui->sampleRateSpinBox->setReadOnly(enabled);
        ui->numThreadsSpinBox->setReadOnly(enabled);
        ui->maxAlternativesSpinBox->setReadOnly(enabled);
        ui->showWordsCheckBox->setEnabled(!enabled);
        ui->modelPathLineEdit->setReadOnly(enabled);
        ui->pbStart->setEnabled(!enabled);
        ui->pbStop->setEnabled(enabled);
    };

    enable_disable_widgets(isRunning());
}

void Application::syncUIWithRecordingState()
{
    ui->pbRecord->setText(recordingInProgress ?
                              QStringLiteral("Stop recording") :
                              QStringLiteral("Start recording"));

    bool microphoneAvailable = QAudioDeviceInfo::availableDevices(QAudio::AudioInput).size() > 0;

    ui->pbRecord->setEnabled(microphoneAvailable /*&& isRunning()*/);
}

QString Application::getCurrentMicrophoneName()
{
    if (ui->cbMicrophone->count() > 0)
    {
        return ui->cbMicrophone->currentText();
    }

    return QString();
}



