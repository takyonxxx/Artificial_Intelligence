#ifndef AI_H
#define AI_H

#include <QMainWindow>
#include <QtWidgets>
#include <QDir>
#include <QMediaRecorder>
#include <QStandardPaths>
#include <QSslSocket>
#include <QAudioDevice>
#include <QAudioSource>
#include <QAudioOutput>
#include <QAudioSink>
#include <QMediaFormat>
#include <QMediaCaptureSession>
#include <QStandardPaths>
#include <QNetworkAccessManager>
#include <QMediaDevices>
#include <QMediaPlayer>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextToSpeech>
#include <QTextDocument>
#include <QDir>
#include <QUrl>
#include <AudioToolbox/AudioToolbox.h>


QT_BEGIN_NAMESPACE
namespace Ui { class Ai; }
class QAudioBuffer;
QT_END_NAMESPACE

class AudioLevel;

class Ai : public QMainWindow
{
    Q_OBJECT

public:
    Ai();
    ~Ai();

    QByteArray generateSineWave(int durationInSeconds, int sampleRate, int channelCount, double frequency) {
        const int numFrames = durationInSeconds * sampleRate;
        const int numChannels = channelCount;

        QByteArray buffer;
        buffer.resize(numFrames * numChannels * sizeof(short));

        short* data = reinterpret_cast<short*>(buffer.data());
        const double amplitude = 0.5 * SHRT_MAX;

        for (int i = 0; i < numFrames; ++i) {
            double t = static_cast<double>(i) / sampleRate;
            for (int channel = 0; channel < numChannels; ++channel) {
                double value = amplitude * std::sin(2.0 * M_PI * frequency * t);
                data[i * numChannels + channel] = static_cast<short>(value);
            }
        }

        return buffer;
    }

public slots:
    void processBuffer(const QAudioBuffer&);
    void recordBuffer(const QByteArray&);
    QList<qreal> getBufferLevels(const QAudioBuffer&);

private slots:
    void toggleRecord();
    void speechVoice();
    void translateText(QString, QString);

    void localeChanged(const QLocale &locale);
    void voiceSelected(int index);
    void languageSelected(int language);

    void onStateChanged(QMediaRecorder::RecorderState);
    void onSpeechStateChanged(QTextToSpeech::State state);
    void updateProgress(qint64 pos);
    void micBufferReady();
    void displayErrorMessage();
    void inputDeviceChanged(int index);
    void outputDeviceChanged(int index);
    void appendText(QString text);
    void setOutputFile();

    void sslErrors(const QList<QSslError> &errors);
    void httpSpeechFinished();
    void httpTranslateFinished();
    void httpSpeechReadyRead();
    void httpTranslateReadyRead();

    void on_exitButton_clicked();
    void on_clearButton_clicked();
    void on_recordButton_clicked();

    void on_micVolumeSlider_valueChanged(int value);
    void on_speechVolumeSlider_valueChanged(int value);
    void on_voxSensivitySlider_valueChanged(int value);
private:
    void clearAudioLevels();
    void setSpeechEngine();
    void startRecording();
    void stopRecording();

    QMediaFormat selectedMediaFormat() const;

    bool isBufferFull() const;
    void writeAudioToFile(const QByteArray&);
    AudioFileID audioFile;
    CFURLRef fileURL;
    QByteArray accumulatedBuffer;

    QTextToSpeech *m_speech = nullptr;
    QVector<QVoice> m_voices;
    int m_current_language_index{0};

    QMediaCaptureSession m_captureSession;    
    QMediaRecorder *m_audioRecorder = nullptr;
    QAudioSource *m_audioInputSource = nullptr;
    QAudioSource *m_audioOutputSource = nullptr;
    QAudioOutput *m_audioOutput = nullptr;
    QAudioInput* m_audioInput = nullptr;
    QIODevice *ioInputDevice = nullptr;
    QIODevice *ioOutputDevice = nullptr;
    QList<AudioLevel*> m_audioLevels;
    QAudioFormat audio_format;

    bool m_recording = false;
    bool m_record_started = false;
    float m_vox_sensitivity = 0.3;
    bool m_outputLocationSet = false;

    const int maxDuration = 3000; // maximum recording duration allowed
    const int minDuration = 1000; // minimium recording duration allowed
    const unsigned sampleRate = 48000;
    const unsigned channelCount = 1;
    int recordDuration = 0; // recording duration in miliseconds

    QNetworkAccessManager *qnam;
    QUrl urlSpeech;
    QUrl urlSearch;
    QUrl urlLanguageTranslate;
    QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> translate_language_reply;
    QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> translate_reply;
    QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> search_reply;

    const QString fileName = "record";
    QString ext = "";
    QFile file;
    Ui::Ai *ui = nullptr;

};

#endif // AI_H
