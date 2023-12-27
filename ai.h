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
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextToSpeech>
#include <QTextDocument>
#include <QDir>
#include <QUrl>

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

public slots:
    void processBuffer(const QAudioBuffer&);
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
    void updateFormats();
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

    QMediaFormat selectedMediaFormat() const;

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
    float m_vox_sensitivity = 0.3;
    bool m_outputLocationSet = false;
    bool m_updatingFormats = false;

    const int maxDuration = 3000; // maximum recording duration allowed
    const int minDuration = 1000; // minimium recording duration allowed
    const unsigned sampleRate = 48000;
    const unsigned channelCount = 2;
    int recordDuration = 0; // recording duration in miliseconds

    QNetworkAccessManager *qnam;
    QUrl urlSpeech;
    QUrl urlSearch;
    QUrl urlLanguageTranslate;
    QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> translate_language_reply;
    QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> translate_reply;
    QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> search_reply;

    const QString fileName = "record";
    QString ext = ".flac";
    QFile file;


    // Map between FileFormat and file extensions
    const QMap<QMediaFormat::FileFormat, QString> formatToExtension = {
                                                                       { QMediaFormat::FileFormat::Mpeg4Audio, ".m4a" },
                                                                       { QMediaFormat::FileFormat::AAC, ".aac" },
                                                                       { QMediaFormat::FileFormat::WMA, ".wma" },
                                                                       { QMediaFormat::FileFormat::MP3, ".mp3" },
                                                                       { QMediaFormat::FileFormat::FLAC, ".flac" },
                                                                       { QMediaFormat::FileFormat::Wave, ".wav" },
                                                                       { QMediaFormat::FileFormat::Ogg, ".ogg" },

                                                                       };

    QString getOutputExtension(const QMediaFormat::FileFormat selectedFormat) {
        return formatToExtension.value(static_cast<QMediaFormat::FileFormat>(selectedFormat), "unknown");
    }

    Ui::Ai *ui = nullptr;

};

#endif // AI_H
