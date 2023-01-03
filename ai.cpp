// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "ai.h"
#include "audiolevel.h"

#include "ui_ai.h"

#include <qmediadevices.h>
#include <qmediaformat.h>
#include <qaudiodevice.h>
#include <qaudiobuffer.h>
#include <qaudioinput.h>
#include <qimagecapture.h>
#include <qmediaformat.h>
#include <QMimeType>
#include "constants.h"

static QList<qreal> getBufferLevels(const QAudioBuffer &buffer);

Ai::Ai()
    : ui(new Ui::Ai)
{
    ui->setupUi(this);

    ui->recordButton->setStyleSheet("font-size: 16pt; font-weight: bold; color: white;background-color:#154360; padding: 12px; spacing: 12px;");
    ui->clearButton->setStyleSheet("font-size: 16pt; font-weight: bold; color: white;background-color:#154360; padding: 12px; spacing: 12px;");
    ui->exitButton->setStyleSheet("font-size: 16pt; font-weight: bold; color: white;background-color:#154360; padding: 12px; spacing: 12px;");
    ui->textTerminal->setStyleSheet("font: 14pt; color: #00cccc; background-color: #001a1a;");
    ui->labelOutputDevice->setStyleSheet("font-size: 14pt; font-weight: bold; color: white;background-color:#154360; padding: 5px; spacing: 5px;");
    ui->audioOutputDeviceBox->setStyleSheet("font-size: 14pt; font-weight: bold; color: white;background-color:orange; padding: 4px; spacing: 4px;");
    ui->labelInputDevice->setStyleSheet("font-size: 14pt; font-weight: bold; color: white;background-color:#154360; padding: 5px; spacing: 5px;");
    ui->audioInputDeviceBox->setStyleSheet("font-size: 14pt; font-weight: bold; color: white;background-color:orange; padding: 4px; spacing: 4px;");
    ui->labelRecordTime->setStyleSheet("font-size: 14pt; font-weight: bold; color: white;background-color:#154360; padding: 5px; spacing: 5px;");
    ui->recordTimeBox->setStyleSheet("font-size: 14pt; font-weight: bold; color: white;background-color:orange; padding: 4px; spacing: 4px;");
    ui->labelMicLevelInfo->setStyleSheet("font-size: 14pt; font-weight: bold; color: white;background-color:#154360; padding: 6px; spacing: 6px;");
    ui->labelSpeechLevelInfo->setStyleSheet("font-size: 14pt; font-weight: bold; color: white;background-color:#154360; padding: 6px; spacing: 6px;");
    appendText(tr("Device supports OpenSSL: %1").arg((QSslSocket::supportsSsl()) ? "Yes" : "No"));

    m_audioRecorder = new QMediaRecorder(this);
    m_captureSession.setRecorder(m_audioRecorder);
    m_captureSession.setAudioInput(new QAudioInput(this));
    m_speech = new QTextToSpeech(this);

    //audio devices    
    for (auto device: QMediaDevices::audioInputs()) {
        auto name = device.description();
        ui->audioInputDeviceBox->addItem(name, QVariant::fromValue(device));
    }
    connect(ui->audioInputDeviceBox, QOverload<int>::of(&QComboBox::activated), this, &Ai::inputDeviceChanged);

    for (auto &device: QMediaDevices::audioOutputs()) {
        auto name = device.description();
        ui->audioOutputDeviceBox->addItem(name, QVariant::fromValue(device));
    }
    connect(ui->audioOutputDeviceBox, QOverload<int>::of(&QComboBox::activated), this, &Ai::outputDeviceChanged);

    //inputDeviceChanged(0);
    //outputDeviceChanged(0);

    //record times
    ui->recordTimeBox->addItem(QStringLiteral("1000"), QVariant(1000));
    ui->recordTimeBox->addItem(QStringLiteral("2000"), QVariant(2000));
    ui->recordTimeBox->addItem(QStringLiteral("3000"), QVariant(3000));
    ui->recordTimeBox->setCurrentIndex(1);

    connect(m_audioRecorder, &QMediaRecorder::durationChanged, this, &Ai::updateProgress);
    connect(m_audioRecorder, &QMediaRecorder::recorderStateChanged, this, &Ai::onStateChanged);
    connect(m_audioRecorder, &QMediaRecorder::errorChanged, this, &Ai::displayErrorMessage);

    //http request
    qnam = new QNetworkAccessManager(this);
    this->url.setUrl(baseApi);
    this->url.setQuery("key=" + apiKey);
    this->request.setUrl(this->url);
    this->request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    //connect(qnam, &QNetworkAccessManager::finished, this, &Ai::onResponseFinish);

    if (!this->location.exists())
        this->location.mkpath(".");
    filePath = location.filePath(fileName);
    m_audioRecorder->setOutputLocation(filePath);    
    file.setFileName(this->m_audioRecorder->outputLocation().toString().remove("file:/") + ".wav");
    appendText(tr("Recording file : %1").arg(file.fileName()));
    m_outputLocationSet = true;

    setSpeechEngine();

    AudioLevel *level = new AudioLevel(ui->centralwidget);
    level->setMinimumSize(QSize(0,50));
    m_audioLevels.append(level);
    ui->levelsLayout->addWidget(level);           
}


void Ai::setSpeechEngine()
{
    m_speech->setPitch(0);
    connect(m_speech, &QTextToSpeech::localeChanged, this, &Ai::localeChanged);
    connect(m_speech, &QTextToSpeech::stateChanged, this, &Ai::onSpeechStateChanged);
    disconnect(ui->language, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &Ai::languageSelected);
    ui->language->clear();
    // Populate the languages combobox before connecting its signal.
    const QVector<QLocale> locales = m_speech->availableLocales();
    QLocale current = m_speech->locale();
    int counter=0;
    for (const QLocale &locale : locales) {
        QString name(QString("%1 (%2)")
                     .arg(QLocale::languageToString(locale.language()))
                     .arg(QLocale::countryToString(locale.country())));
        QVariant localeVariant(locale);
        ui->language->addItem(name, localeVariant);
        if (name.contains("Turkish"))
        {
            current = locale;
            m_current_language_index = counter;
        }
        counter++;
    }
    connect(ui->language, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &Ai::languageSelected);
    ui->language->setCurrentIndex(m_current_language_index);
}

void Ai::languageSelected(int language)
{
    QLocale locale = ui->language->itemData(language).toLocale();
    m_speech->setLocale(locale);
}

void Ai::localeChanged(const QLocale &locale)
{
    QVariant localeVariant(locale);
    ui->language->setCurrentIndex(ui->language->findData(localeVariant));

    disconnect(ui->voice, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &Ai::voiceSelected);
    ui->voice->clear();

    m_voices = m_speech->availableVoices();
    QVoice currentVoice = m_speech->voice();
    for (const QVoice &voice : qAsConst(m_voices)) {
        ui->voice->addItem(QString("%1 - %2 - %3").arg(voice.name())
                          .arg(QVoice::genderName(voice.gender()))
                          .arg(QVoice::ageName(voice.age())));
        if (voice.name() == currentVoice.name())
            ui->voice->setCurrentIndex(ui->voice->count() - 1);
    }
    connect(ui->voice, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &Ai::voiceSelected);
}

void Ai::voiceSelected(int index)
{
    m_speech->setVoice(m_voices.at(index));
}

void Ai::sslErrors(const QList<QSslError> &errors)
{
    QString errorString;
    for (const QSslError &error : errors) {
        if (!errorString.isEmpty())
            errorString += '\n';
        errorString += error.errorString();
    }

    if (QMessageBox::warning(this, tr("SSL Errors"),
                             tr("One or more SSL errors has occurred:\n%1").arg(errorString),
                             QMessageBox::Ignore | QMessageBox::Abort) == QMessageBox::Ignore) {
        reply->ignoreSslErrors();
    }
}

void Ai::httpFinished()
{
    if(reply->error() != QNetworkReply::NoError)
    {
        const QString &errorString = reply->errorString();
        appendText(errorString);
    }
    reply.reset();
}

void Ai::httpReadyRead()
{
    auto data = QJsonDocument::fromJson(reply->readAll());

    QString strFromJson = QJsonDocument(data).toJson(QJsonDocument::Compact).toStdString().c_str();
    appendText(strFromJson);

    auto error = data["error"]["message"];

    if (error.isUndefined()) {
        auto command = data["results"][0]["alternatives"][0]["transcript"].toString();
        if (command.size() > 0){

            appendText(command);
            m_speech->say(command);
        }
        else
        {
            appendText("No response.");
            m_speech->say("Şu an cevap veremiyorum.");
        }
    }
}

void Ai::onResponseFinish(QNetworkReply *response)
{
    auto data = QJsonDocument::fromJson(response->readAll());
    response->deleteLater();

    QString strFromJson = QJsonDocument(data).toJson(QJsonDocument::Compact).toStdString().c_str();
    appendText(strFromJson);

    auto error = data["error"]["message"];

    if (error.isUndefined()) {
        auto command = data["results"][0]["alternatives"][0]["transcript"].toString();
        if (command.size() > 0){

            appendText(command);
        }
        else
        {
            appendText("No response.");
        }
    }
}

void Ai::translate()
{

    if (!file.open(QIODevice::ReadOnly)) {
       qDebug()  << "cannot open file:" << file.errorString() << file.fileName();
       return;
    }

    QByteArray fileData = file.readAll();
    file.close();

    QJsonDocument data {
        QJsonObject { {
                "audio",
                QJsonObject { {"content", QJsonValue::fromVariant(fileData.toBase64())} }
            },  {
                "config",
                QJsonObject {
                    {"encoding", "LINEAR16"},
                    {"languageCode", "TR"},
                    {"model", "command_and_search"},
                    {"sampleRateHertz", QJsonValue::fromVariant(sampleRate)},
                    {"audioChannelCount", 2}
                }}
        }
    };

//    qnam->post(this->request, data.toJson(QJsonDocument::Compact));

    reply.reset(qnam->post(QNetworkRequest(url), data.toJson(QJsonDocument::Compact)));

    connect(reply.get(), &QNetworkReply::sslErrors, this, &Ai::sslErrors);
    connect(reply.get(), &QNetworkReply::finished, this, &Ai::httpFinished);
    connect(reply.get(), &QIODevice::readyRead, this, &Ai::httpReadyRead);
}

static QVariant boxValue(const QComboBox *box)
{
    int idx = box->currentIndex();
    if (idx == -1)
        return QVariant();

    return box->itemData(idx);
}

void Ai::inputDeviceChanged(int index)
{
    qDebug() << "inputDeviceChanged";
    m_captureSession.audioInput()->setDevice(boxValue(ui->audioInputDeviceBox).value<QAudioDevice>());
    //m_captureSession.audioInput()->setDevice(ui->audioInputDeviceBox->itemData(index).value<QAudioDevice>());
}

void Ai::outputDeviceChanged(int index)
{
    qDebug() << "outputDeviceChanged";
    QAudioDevice info = QMediaDevices::defaultAudioInput();
    QAudioFormat format;
    format.setSampleFormat(QAudioFormat::Int16);
    format.setSampleRate(44100);
    format.setChannelCount(1);


    if (!info.isFormatSupported(format)) {
        qWarning() << "Default format not supported - trying to use nearest";
    }

}

void Ai::updateProgress(qint64 duration)
{
    if (m_audioRecorder->error() != QMediaRecorder::NoError)
        return;

    if (duration >= this->recordDuration &&
            this->m_audioRecorder->recorderState() != QMediaRecorder::RecorderState::StoppedState &&
            this->m_audioRecorder->recorderState() == QMediaRecorder::RecorderState::RecordingState)
    {
        this->m_audioRecorder->stop();
        translate();
    }

    ui->statusbar->showMessage(tr("Recorded %1 sec").arg(duration / 1000));
}

void Ai::onStateChanged(QMediaRecorder::RecorderState state)
{
    QString statusMessage;

    switch (state) {
    case QMediaRecorder::RecordingState:
        ui->recordButton->setText(tr("Stop"));
        statusMessage = tr("Recording to %1").arg(m_audioRecorder->outputLocation().toString());
        break;
    case QMediaRecorder::PausedState:
        ui->recordButton->setText(tr("Stop"));
        clearAudioLevels();
        statusMessage = tr("Paused");
        break;
    case QMediaRecorder::StoppedState:
        clearAudioLevels();
        ui->recordButton->setText(tr("Record"));
        statusMessage = tr("Stopped");
        break;
    }

    if (m_audioRecorder->error() == QMediaRecorder::NoError)
        ui->statusbar->showMessage(statusMessage);
}

void Ai::onSpeechStateChanged(QTextToSpeech::State state)
{
    if (state == QTextToSpeech::Speaking) {
        ui->statusbar->showMessage("Speech started...");
    } else if (state == QTextToSpeech::Ready)
        ui->statusbar->showMessage("Speech stopped...", 2000);
    else if (state == QTextToSpeech::Paused)
        ui->statusbar->showMessage("Speech paused...");
    else
        ui->statusbar->showMessage("Speech error!");
}


QMediaFormat Ai::selectedMediaFormat() const
{
    QMediaFormat format;
    format.setFileFormat(QMediaFormat::Wave);
    format.setAudioCodec(QMediaFormat::AudioCodec::Wave);
    return format;
}


void Ai::appendText(QString text)
{
    ui->textTerminal->append(text);
}


void Ai::toggleRecord()
{
    if (m_audioRecorder->recorderState() == QMediaRecorder::StoppedState) {

        if(file.exists())
            file.remove();

        this->recordDuration = boxValue(ui->recordTimeBox).toInt();

        auto input_device = boxValue(ui->audioInputDeviceBox).value<QAudioDevice>();

        QAudioFormat format;
        format.setSampleFormat(QAudioFormat::Int16);
        format.setSampleRate(44100);
        format.setChannelCount(2);

        if(input_device.isFormatSupported(format))
        {
            m_captureSession.audioInput()->setDevice(input_device);
            m_audioRecorder->setMediaFormat(selectedMediaFormat());
            m_audioRecorder->setAudioSampleRate(sampleRate);
            m_audioRecorder->setAudioBitRate(0);
            m_audioRecorder->setAudioChannelCount(2);
            m_audioRecorder->setQuality(QMediaRecorder::HighQuality);
            m_audioRecorder->setEncodingMode(QMediaRecorder::ConstantQualityEncoding);

            m_audioRecorder->record();
        }
    }
    else {
        m_audioRecorder->stop();
    }
}

void Ai::displayErrorMessage()
{
    ui->statusbar->showMessage(m_audioRecorder->errorString());
}

void Ai::clearAudioLevels()
{
    for (auto m_audioLevel : qAsConst(m_audioLevels))
        m_audioLevel->setLevel(0);
}

// returns the audio level for each channel
QList<qreal> getBufferLevels(const QAudioBuffer &buffer)
{
    QList<qreal> values;

    auto format = buffer.format();
    if (!format.isValid())
        return values;

    int channels = buffer.format().channelCount();
    values.fill(0, channels);

    int bytesPerSample = format.bytesPerSample();
    QList<qreal> max_values;
    max_values.fill(0, channels);

    const char *data = buffer.constData<char>();
    for (int i = 0; i < buffer.frameCount(); ++i) {
        for (int j = 0; j < channels; ++j) {
            qreal value = qAbs(format.normalizedSampleValue(data));
            if (value > max_values.at(j))
                max_values[j] = value;
            data += bytesPerSample;
        }
    }

    return max_values;
}

void Ai::processBuffer(const QAudioBuffer& buffer)
{
    if (m_audioLevels.count() != buffer.format().channelCount()) {
        qDeleteAll(m_audioLevels);
        m_audioLevels.clear();
        for (int i = 0; i < buffer.format().channelCount(); ++i) {
            AudioLevel *level = new AudioLevel(ui->centralwidget);
            m_audioLevels.append(level);
            ui->levelsLayout->addWidget(level);
        }
    }

    QList<qreal> levels = getBufferLevels(buffer);
    for (int i = 0; i < levels.count(); ++i)
        m_audioLevels.at(i)->setLevel(levels.at(i));
}

void Ai::on_exitButton_clicked()
{
    QApplication::quit();
}


void Ai::on_clearButton_clicked()
{
      ui->textTerminal->clear();
}


void Ai::on_recordButton_clicked()
{
    toggleRecord();
}

void Ai::on_micVolumeSlider_valueChanged(int value)
{
    qreal linearVolume = QAudio::convertVolume(value / qreal(100),
                                               QAudio::LogarithmicVolumeScale,
                                               QAudio::LinearVolumeScale);
    m_captureSession.audioInput()->setVolume(linearVolume);
    ui->labelMicLevelInfo->setText(QString::number(m_captureSession.audioInput()->volume() * 100, 'f', 0) + "%");
}


void Ai::on_speechVolumeSlider_valueChanged(int value)
{
    qreal linearVolume = QAudio::convertVolume(value / qreal(100),
                                               QAudio::LogarithmicVolumeScale,
                                               QAudio::LinearVolumeScale);
    m_speech->setVolume(linearVolume);
    ui->labelSpeechLevelInfo->setText(QString::number(m_speech->volume() * 100, 'f', 0) + "%");
}

