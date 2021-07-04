#include <QDebug>
#include <QDir>
#include <QSettings>

#include "application.h"
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
    QSettings settings(QApplication::applicationDirPath(), QSettings::IniFormat);
    settings.setValue("model_path", ui->modelPathLineEdit->text());
}

void Application::readSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::Scope::UserScope, "VLS");
    ui->modelPathLineEdit->setText(settings.value("model_path").toString());
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

