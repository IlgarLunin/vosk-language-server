#pragma once

#include <QAudioInput>
#include <QtEndian>
#include <functional>


class MicrophoneAudioDevice : public QIODevice
{
    Q_OBJECT

public:
    typedef std::function<void(const char *data, qint64 len, qreal level)> WriteCallbackType;

    MicrophoneAudioDevice(const QAudioFormat &format, WriteCallbackType writeCallback);

    void start();
    void stop();

    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;

private:
    const QAudioFormat m_format;
    WriteCallbackType callback;
    quint32 m_maxAmplitude = 0;
    qreal m_level = 0.0; // 0.0 <= m_level <= 1.0

signals:
    void update();
};
