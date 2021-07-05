#include <QDebug>
#include <QDir>
#include <QSettings>

#include "application.h"
#include "vls_common.h"
#include "./ui_application.h"


Application::Application(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Application)
{
    ui->setupUi(this);
    ui->leLog->setContextMenuPolicy(Qt::ActionsContextMenu);

    m_actionClearLog = new QAction(this);
    m_actionClearLog->setText("Clear");
    connect(m_actionClearLog, &QAction::triggered, this, &Application::on_clearLog);
    ui->leLog->addAction(m_actionClearLog);

    m_hearbeatTimer.setInterval(1000);
    connect(&m_hearbeatTimer, &QTimer::timeout, this, &Application::on_hearBeat);
    m_hearbeatTimer.start(1000);

    syncUIWithProcessState();
    ui->leLog->setReadOnly(true);

    m_QuitAction = new QAction(this);
    m_QuitAction->setText("Quit");
    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(m_QuitAction);
    connect(m_QuitAction, &QAction::triggered, this, &Application::onQuit);
    connect(ui->actionExit, &QAction::triggered, this, &Application::onQuit);

    trayIcon = new QSystemTrayIcon(this);
    connect(trayIcon, &QSystemTrayIcon::activated, this, &Application::on_trayIconActivated);
    trayIcon->setIcon(QIcon(":/vosk.ico"));
    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->show();

    setWindowTitle(QString("Vosk Language Server %1.%2").arg(VLS::VLS_VERSION_MAJOR).arg(VLS::VLS_VERSION_MINOR));

    readSettings();
    connect(trayIcon, &QSystemTrayIcon::messageClicked, this, &Application::on_trayMessageClicked);
    trayIcon->showMessage("", "Vls is running in background", QSystemTrayIcon::NoIcon, 300);

    if(ui->autoStartCheckBox->isChecked())
    {
        on_pbStart_clicked();
    }

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


void Application::on_pbStart_clicked()
{
    if(!isRunning())
    {
        m_currentProcess = new QProcess(this);

        m_currentProcess->setWorkingDirectory(QApplication::applicationDirPath());
        connect(m_currentProcess, &QProcess::stateChanged, this, &Application::on_processStateChanged);
        connect(m_currentProcess, &QProcess::errorOccurred, this, &Application::on_processErrorOccurred);
        connect(m_currentProcess, &QProcess::readyReadStandardOutput, this, &Application::on_readyReadStandardOutput);
        connect(m_currentProcess, &QProcess::readyReadStandardError, this, &Application::on_readyReadStandardError);

        QStringList args;
        args << "-a" << ui->addressLineEdit->text();
        args << "-p" << QString("%1").arg(ui->portSpinBox->value());
        args << "-t" << QString("%1").arg(ui->numThreadsSpinBox->value());
        args << "-s" << QString("%1").arg(ui->sampleRateSpinBox->value());
        args << "-l" << QString("%1").arg(ui->maxAlternativesSpinBox->value());
        args << "-w" << QString("%1").arg(ui->showWordsCheckBox->isChecked());
        args << "-m" << QDir::toNativeSeparators(ui->modelPathLineEdit->text());

        m_currentProcess->setProgram("asr_server.exe");
        m_currentProcess->setArguments(args);
        m_currentProcess->start();

    }
    syncUIWithProcessState();
}

void Application::on_pbStop_clicked()
{
    if(isRunning())
    {
        m_currentProcess->disconnect();
        m_currentProcess->kill();
        m_currentProcess->waitForFinished();
        delete m_currentProcess;
        m_currentProcess.clear();
    }
}

void Application::on_processErrorOccurred(QProcess::ProcessError err)
{

}

void Application::on_processStateChanged(QProcess::ProcessState st)
{

}

void Application::on_readyReadStandardOutput()
{
    QString out = m_currentProcess->readAllStandardOutput();
    ui->leLog->appendPlainText(out);
}

void Application::on_readyReadStandardError()
{
    QString out = m_currentProcess->readAllStandardError();
    ui->leLog->appendPlainText(out);
}

void Application::on_clearLog()
{
    ui->leLog->clear();
}


void Application::on_hearBeat()
{
    syncUIWithProcessState();
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

void Application::on_trayIconActivated(QSystemTrayIcon::ActivationReason reason)
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

void Application::on_trayMessageClicked()
{
    showNormal();
    activateWindow();
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

