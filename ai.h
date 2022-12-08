#ifndef AI_H
#define AI_H

#include <QMainWindow>
#include <QMediaRecorder>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QAudioProbe>
#include <QAudioRecorder>
#include <QAudioInput>
#include <QFileDialog>
#include <QFile>
#include <QUrl>
#include <QDir>

QT_BEGIN_NAMESPACE
namespace Ui { class Ai; }
class QAudioRecorder;
class QAudioProbe;
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

private slots:
    void togglePause();
    void toggleRecord();
    void translate();
    void appendText(QString text);

    void updateStatus(QMediaRecorder::Status);
    void onStateChanged(QMediaRecorder::State);
    void onResponseFinish(QNetworkReply *response);
    void updateProgress(qint64 pos);
    void initializeAudioInput(const QAudioDeviceInfo &deviceInfo);
    void displayErrorMessage();
    void on_exitButton_clicked();
    void on_recordButton_clicked();
    void on_micVolumeSlider_valueChanged(int value);

    void on_clearButton_clicked();

private:
    void clearAudioLevels();

    QAudioEncoderSettings settings;
    QNetworkAccessManager *qnam;
    QNetworkRequest request;
    QUrl url;
    const QDir location = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString fileName = "record";
    QString filePath;
    QFile file;
    const int maxDuration = 3000; // maximum recording duration allowed
    const int minDuration = 1000; // minimium recording duration allowed
    int recordDuration = 0; // recording duration in miliseconds

    QAudioRecorder *m_audioRecorder = nullptr;
    QScopedPointer<QAudioInput> m_audioInput;
    QAudioProbe *m_probe = nullptr;
    QList<AudioLevel*> m_audioLevels;
    bool m_outputLocationSet = false;
    QByteArray byteArr;

private:
    Ui::Ai *ui;
};
#endif // AI_H
