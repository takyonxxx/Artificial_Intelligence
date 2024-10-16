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

#include <QCoreApplication>
#include <QJniObject>
#include <QtCore/private/qandroidextras_p.h>
#include <QFuture>
#include <QFutureWatcher>

static QList<qreal> getBufferLevels(const QAudioBuffer &buffer);

static QVariant boxValue(const QComboBox *box)
{
    int idx = box->currentIndex();
    if (idx == -1)
        return QVariant();

    return box->itemData(idx);
}

Ai::Ai()
    : ui(new Ui::Ai)
{
#ifdef Q_OS_ANROID
    requestMicrophonePermission();
#else
    initializeAi();
#endif
}

Ai::~Ai()
{
    delete m_speech;
    delete m_audioOutput;
    delete ioInputDevice;
    delete ioOutputDevice;
    delete m_audioRecorder;
    delete m_audioInputSource;
    delete qnam;
}

void Ai::initializeAi()
{
    ui->setupUi(this);

#ifdef Q_OS_IOS
    QSize screenSize = qApp->primaryScreen()->availableSize();
    int targetWidth = screenSize.width() * 0.8;
    int targetHeight = screenSize.height() * 0.8;
    // Set the fixed size or adjust the size policy as needed
    this->setFixedSize(targetWidth, targetHeight);
#endif

    ui->recordButton->setStyleSheet("font-size: 16pt; font-weight: bold; color: white;background-color:#900C3F; padding: 6px; spacing: 6px;");
    ui->languageButton->setStyleSheet("font-size: 16pt; font-weight: bold; color: white;background-color:#900C3F; padding: 6px; spacing: 6px;");
    ui->clearButton->setStyleSheet("font-size: 16pt; font-weight: bold; color: white;background-color:#045F0A; padding: 6px; spacing: 6px;");
    ui->exitButton->setStyleSheet("font-size: 16pt; font-weight: bold; color: white;background-color:#045F0A; padding: 6px; spacing: 6px;");
    ui->textTerminal->setStyleSheet("font: 13pt; color: #00cccc; background-color: #001a1a;");
    ui->labelOutputDevice->setStyleSheet("font-size: 14pt; font-weight: bold; color: white;background-color:#154360; padding: 6px; spacing: 6px;");
    ui->audioOutputDeviceBox->setStyleSheet("font-size: 14pt; font-weight: bold; color: white;background-color:#6B0785; padding: 6px; spacing: 6px;");
    ui->labelInputDevice->setStyleSheet("font-size: 14pt; font-weight: bold; color: white;background-color:#154360; padding: 6px; spacing: 6px;");
    ui->audioInputDeviceBox->setStyleSheet("font-size: 14pt; font-weight: bold; color: white;background-color:#6B0785; padding: 6px; spacing: 6px;");
    ui->labelRecordTime->setStyleSheet("font-size: 14pt; font-weight: bold; color: white;background-color:#154360; padding: 6px; spacing: v;");
    ui->recordTimeBox->setStyleSheet("font-size: 14pt; font-weight: bold; color: white;background-color:#6B0785; padding: 6px; spacing: 6px;");
    ui->labelMicLevelInfo->setStyleSheet("font-size: 14pt; font-weight: bold; color: white;background-color:#154360; padding: 6px; spacing: 6px;");
    ui->labelSpeechLevelInfo->setStyleSheet("font-size: 14pt; font-weight: bold; color: white;background-color:#154360; padding: 6px; spacing: 6px;");
    appendText(tr("Device supports OpenSSL: %1").arg((QSslSocket::supportsSsl()) ? "Yes" : "No"));

#ifndef __APPLE__
    m_audioRecorder = new QMediaRecorder(this);
    connect(m_audioRecorder, &QMediaRecorder::durationChanged, this, &Ai::updateProgress);
    connect(m_audioRecorder, &QMediaRecorder::recorderStateChanged, this, &Ai::onStateChanged);
    connect(m_audioRecorder, &QMediaRecorder::errorOccurred, this, &Ai::displayErrorMessage);
    connect(m_audioRecorder, &QMediaRecorder::errorChanged, this, &Ai::displayErrorMessage);
#endif

    QAudioDevice outputDevice;

    for (auto &device: QMediaDevices::audioOutputs()) {
        outputDevice = device;
        break;
    }

    m_format.setSampleFormat(QAudioFormat::Int16);
    m_format.setSampleRate(sampleRate);
    m_format.setChannelCount(channelCount);

    if (outputDevice.isFormatSupported(m_format)) {
        m_audioOutput = new QAudioSink(outputDevice, m_format, this);
        connect(m_audioOutput,&QAudioSink::stateChanged, this, &Ai::handleAudioStateChanged);
    }

    m_audioInput = new QAudioInput(this);
    m_speech = new QTextToSpeech(this);
    langpair = "tr|en";
    languageCode = "TR";


    //audio devices
    for (auto device: QMediaDevices::audioInputs()) {
        auto name = device.description();
        ui->audioInputDeviceBox->addItem(name, QVariant::fromValue(device));
    }
    connect(ui->audioInputDeviceBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &Ai::inputDeviceChanged);

    for (auto &device: QMediaDevices::audioOutputs()) {
        auto name = device.description();
        ui->audioOutputDeviceBox->addItem(name, QVariant::fromValue(device));
    }
    connect(ui->audioOutputDeviceBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &Ai::outputDeviceChanged);

    //record times
    ui->recordTimeBox->addItem(QStringLiteral("1000"), QVariant(1000));
    ui->recordTimeBox->addItem(QStringLiteral("3000"), QVariant(3000));
    ui->recordTimeBox->addItem(QStringLiteral("5000"), QVariant(5000));
    ui->recordTimeBox->setCurrentIndex(1);

    //http request
    qnam = new QNetworkAccessManager(this);
    //    translateClient = new TranslateClient(this);

    this->urlSpeech.setUrl(speechBaseApi);
    this->urlLanguageTranslate.setUrl(translateUrl);
    this->urlAi.setUrl(aiUrl);

    setSpeechEngine();
    inputDeviceChanged(0);
    outputDeviceChanged(0);
    //    m_speech->say("Please press the record button.");
}

void Ai::requestMicrophonePermission()
{
    if (QNativeInterface::QAndroidApplication::sdkVersion() >= 23) {
        QFuture<QtAndroidPrivate::PermissionResult> checkFuture = QtAndroidPrivate::checkPermission(QString("android.permission.RECORD_AUDIO"));

        QFutureWatcher<QtAndroidPrivate::PermissionResult>* checkWatcher = new QFutureWatcher<QtAndroidPrivate::PermissionResult>(this);

        connect(checkWatcher, &QFutureWatcher<QtAndroidPrivate::PermissionResult>::finished, this, [this, checkWatcher]() {
            QtAndroidPrivate::PermissionResult checkResult = checkWatcher->result();
            if (checkResult == QtAndroidPrivate::PermissionResult::Denied) {
                QFuture<QtAndroidPrivate::PermissionResult> requestFuture = QtAndroidPrivate::requestPermission(QString("android.permission.RECORD_AUDIO"));

                QFutureWatcher<QtAndroidPrivate::PermissionResult>* requestWatcher = new QFutureWatcher<QtAndroidPrivate::PermissionResult>(this);

                connect(requestWatcher, &QFutureWatcher<QtAndroidPrivate::PermissionResult>::finished, this, [this, requestWatcher]() {
                    QtAndroidPrivate::PermissionResult requestResult = requestWatcher->result();
                    if (requestResult == QtAndroidPrivate::PermissionResult::Authorized) {
                        qDebug() << "Microphone permission granted";
                        initializeAi();
                    } else {
                        qDebug() << "Microphone permission denied";
                        // Handle the case where permission is denied
                        QMetaObject::invokeMethod(this, "close", Qt::QueuedConnection);
                    }
                    requestWatcher->deleteLater();
                });

                requestWatcher->setFuture(requestFuture);
            } else if (checkResult == QtAndroidPrivate::PermissionResult::Authorized) {
                qDebug() << "Microphone permission already granted";
                initializeAi();
            }
            checkWatcher->deleteLater();
        });

        checkWatcher->setFuture(checkFuture);
    } else {
        // For Android versions below 6.0, permission is granted at install time
        initializeAi();
    }
}

void Ai::handleAudioStateChanged(QAudio::State newState)
{
    qDebug() << newState;

    switch (newState) {
    case QAudio::IdleState:
        m_audioOutput->stop();
        break;

    case QAudio::StoppedState:
        // Stopped for other reasons
        if (m_audioOutput->error() != QAudio::NoError) {
            // Error handling
        }
        break;

    default:
        // ... other cases as appropriate
        break;
    }
}

void Ai::setOutputFile()
{
    // Get the writable location for AppLocalData
    QString filePath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QDir folderDir(filePath);
    // List all files in the folder
    QStringList files = folderDir.entryList(QDir::Files);
    // Remove each file
    for (const QString &file : files) {
        QString file_Path = filePath + QDir::separator() + file;
        QFile::remove(file_Path);
    }

    // Set the output location for the audio recorder
    m_audioRecorder->setOutputLocation(QUrl::fromLocalFile(filePath.remove("file://") + "/record"));
    m_outputLocationSet = true;
}

void Ai::setSpeechEngine()
{
    m_speech->setPitch(0);
    connect(m_speech, &QTextToSpeech::localeChanged, this, &Ai::localeChanged);
    connect(m_speech, &QTextToSpeech::stateChanged, this, &Ai::onSpeechStateChanged);
    connect(ui->language, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &Ai::languageSelected);
    ui->language->clear();
    // Populate the languages combobox before connecting its signal.
    const QVector<QLocale> locales = m_speech->availableLocales();
    QLocale current = m_speech->locale();
    int counter=0;
    for (const QLocale &locale : locales) {
        QString name(QString("%1 (%2)")
                         .arg(QLocale::languageToString(locale.language()))
                         .arg(QLocale::territoryToString(locale.territory())));
        QVariant localeVariant(locale);
        ui->language->addItem(name, localeVariant);
        if (langpair == "tr|en" && name.contains("English (United States)"))
        {
            current = locale;
            m_current_language_index = counter;
        }
        else if (langpair == "en|tr" && name.contains("Turkish"))
        {
            current = locale;
            m_current_language_index = counter;
        }
        counter++;
    }
    ui->language->setCurrentIndex(m_current_language_index);
}

void Ai::languageSelected(int language)
{
    QLocale locale = ui->language->itemData(language).toLocale();
    m_speech->setLocale(locale);
    localeChanged(locale);
}

void Ai::localeChanged(const QLocale &locale)
{
    QVariant localeVariant(locale);
    ui->language->setCurrentIndex(ui->language->findData(localeVariant));

    disconnect(ui->voice, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &Ai::voiceSelected);
    ui->voice->clear();
    int counter=0;

    m_voices = m_speech->availableVoices();
    QVoice currentVoice = m_speech->voice();
    for (const QVoice &voice : std::as_const(m_voices)) {

        ui->voice->addItem(QString("%1 - %2 - %3").arg(voice.name())
                               .arg(QVoice::genderName(voice.gender()))
                               .arg(QVoice::ageName(voice.age())));
        if (QVoice::genderName(voice.gender()).contains("Female") || voice.name().contains("female"))
        {
            m_current_voice_index = counter;
        }
        counter++;
    }
    connect(ui->voice, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &Ai::voiceSelected);
    ui->voice->setCurrentIndex(m_current_voice_index);
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
        ai_reply->ignoreSslErrors();  // Use ai_reply here
    }

    qDebug() << "sslErrors";
}


void Ai::httpSpeechFinished()
{
    if(speech_reply->error() != QNetworkReply::NoError)
    {
        const QString &errorString = speech_reply->errorString();
        appendText(errorString);
    }
    speech_reply.reset();
}

void Ai::httpTranslateFinished()
{
    if(translate_reply->error() != QNetworkReply::NoError)
    {
        const QString &errorString = translate_reply->errorString();
        appendText(errorString);
    }
    translate_reply.reset();
}

void Ai::httpAiFinished()
{
    if(ai_reply->error() != QNetworkReply::NoError)
    {
        const QString &errorString = ai_reply->errorString();
        appendText(errorString);
    }
    ai_reply.reset();
}

void Ai::httpSpeechReadyRead()
{
    auto data = QJsonDocument::fromJson(speech_reply->readAll());

    QString strFromJson = QJsonDocument(data).toJson(QJsonDocument::Compact).toStdString().c_str();
    qDebug() << strFromJson;
    m_recording = false;
    auto error = data["error"]["message"];

    if (error.isUndefined()) {
        auto command = data["results"][0]["alternatives"][0]["transcript"].toString();
        if (command.size() > 0){

            //           auto translatedText = translateClient->translateText(command, langpair);
            translateText(command, langpair);
        }
        else
        {
            appendText(strFromJson);
        }
    }
}

void Ai::speechVoice()
{
    QString filePath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QDir folderDir(filePath);
    QStringList files = folderDir.entryList(QDir::Files);
    for (const QString &_file : files) {
        QString file_Path = filePath + QDir::separator() + _file;
        if(file_Path.contains("record."))
        {
            file.setFileName(file_Path);
            appendText(tr("Record file : %1").arg(file.fileName()));
            break;
        }
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug()  << "cannot open file:" << file.errorString() << file.fileName();
        return;
    }

    QByteArray fileData = file.readAll();
    file.close();

    QJsonObject config {
        {"encoding", "LINEAR16"},
        {"languageCode", languageCode},
        {"model", "command_and_search"},
        {"enableAutomaticPunctuation", true},
        {"audioChannelCount", QJsonValue::fromVariant(channelCount)},
        {"enableWordTimeOffsets", false},
        {"sampleRateHertz", QJsonValue::fromVariant(sampleRate)}
    };

    QJsonDocument data {
        QJsonObject {
            {
                "audio", QJsonObject { {"content", QJsonValue::fromVariant(fileData.toBase64())} }
            },
            {
                "config", config
            }
        }
    };

    this->urlSpeech.setQuery("key=" + speechApiKey);
    speech_reply.reset(qnam->post(QNetworkRequest(urlSpeech), data.toJson(QJsonDocument::Compact)));
    
    connect(speech_reply.get(), &QNetworkReply::sslErrors, this, &Ai::sslErrors);
    connect(speech_reply.get(), &QNetworkReply::finished, this, &Ai::httpSpeechFinished);
    connect(speech_reply.get(), &QIODevice::readyRead, this, &Ai::httpSpeechReadyRead);
}

void Ai::httpTranslateReadyRead()
{
    QString clearText{};
    QString strReply = translate_reply->readAll().toStdString().c_str();
    QJsonDocument jsonResponseDoc = QJsonDocument::fromJson(strReply.toUtf8());

    if (!jsonResponseDoc.isNull() && jsonResponseDoc.isObject()) {
        QJsonObject responseObject = jsonResponseDoc.object();
        if (responseObject.contains("responseData") && responseObject["responseData"].isObject()) {
            QJsonObject responseDataObject = responseObject["responseData"].toObject();
            if (responseDataObject.contains("translatedText") && responseDataObject["translatedText"].isString()) {
                QString translatedText = responseDataObject["translatedText"].toString();
                qDebug() << "Translated Text: " << translatedText;
                appendText(translatedText);
                m_speech->say(translatedText);                
            }
        }
    }
}

void Ai::httpAiReadyRead()
{
    qDebug() << "okkkkkkkkkkk";
    auto data = QJsonDocument::fromJson(ai_reply->readAll());

    QString strFromJson = QJsonDocument(data).toJson(QJsonDocument::Compact).toStdString().c_str();
    qDebug() << strFromJson;
}

void Ai::translateText(QString text, QString langpair)
{
    QUrlQuery query;
    query.addQueryItem("langpair", langpair);
    query.addQueryItem("q", text);
    query.addQueryItem("mt", "1");
    query.addQueryItem("onlyprivate", "0");
    query.addQueryItem("de", "a@b.c");
    this->urlLanguageTranslate.setQuery(query);

    QNetworkRequest request(this->urlLanguageTranslate);
    request.setRawHeader("Authorization", "q9Oh7Lg7J0Hd");
    request.setRawHeader("X-RapidAPI-Host", translateHost.toStdString().c_str());
    request.setRawHeader("X-RapidAPI-Key", translateApiKey.toStdString().c_str());
    translate_reply.reset(qnam->get(request));
    
    connect(translate_reply.get(), &QNetworkReply::sslErrors, this, &Ai::sslErrors);
    connect(translate_reply.get(), &QNetworkReply::finished, this, &Ai::httpTranslateFinished);
    connect(translate_reply.get(), &QIODevice::readyRead, this, &Ai::httpTranslateReadyRead);
}

void Ai::getFromAi(QString text)
{
    QUrlQuery query;
    query.addQueryItem("message", text);
    query.addQueryItem("user_id", "420");
    this->urlAi.setQuery(query);

    // Set up the request headers
    QNetworkRequest request(this->urlAi);
    request.setRawHeader("Authorization", "q9Oh7Lg7J0Hd");
    request.setRawHeader("X-Rapidapi-Host", aiHost.toStdString().c_str());
    request.setRawHeader("X-Rapidapi-Key", aiApiKey.toStdString().c_str());

    ai_reply.reset(qnam->get(request));
    qDebug() << this->urlAi;

    connect(ai_reply.get(), &QNetworkReply::sslErrors, this, &Ai::sslErrors);
    connect(ai_reply.get(), &QNetworkReply::finished, this, &Ai::httpAiFinished);
    connect(ai_reply.get(), &QIODevice::readyRead, this, &Ai::httpAiReadyRead);
}

void Ai::inputDeviceChanged(int index)
{
    const QAudioDevice &inputDevice = ui->audioInputDeviceBox->itemData(index).value<QAudioDevice>();

    qDeleteAll(m_audioLevels);
    m_audioLevels.clear();

    audio_format.setSampleFormat(QAudioFormat::Int16);
    audio_format.setSampleRate(sampleRate);
    audio_format.setChannelCount(channelCount);

    m_audioInputSource = new QAudioSource(inputDevice, audio_format);
    m_audioInputSource->setBufferSize(1024);

    m_audioInput->setDevice(inputDevice);
    m_captureSession.setAudioInput(m_audioInput);
    m_captureSession.setRecorder(m_audioRecorder);

    m_audioLevels.clear();
    for (int i = 0; i < audio_format.channelCount(); ++i) {
        AudioLevel *level = new AudioLevel(ui->centralwidget);
        level->setMinimumSize(QSize(0,50));
        m_audioLevels.append(level);
        ui->levelsLayout->addWidget(level);
    }

    ioInputDevice = m_audioInputSource->start();
    connect(ioInputDevice, &QIODevice::readyRead, this, &Ai::micBufferReady);

    appendText("Default microphone (" + inputDevice.description() + ')');
}

void Ai::outputDeviceChanged(int index)
{
    if(m_audioOutput)
    {
        disconnect(m_audioOutput,&QAudioSink::stateChanged, this, &Ai::handleAudioStateChanged);
        const QAudioDevice &outputDevice = ui->audioOutputDeviceBox->itemData(index).value<QAudioDevice>();
        if (outputDevice.isFormatSupported(m_format)) {
            m_audioOutput = new QAudioSink(outputDevice, m_format, this);
            connect(m_audioOutput,&QAudioSink::stateChanged, this, &Ai::handleAudioStateChanged);
        }
        appendText("Default speaker (" + outputDevice.description() + ')');
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
        speechVoice();
    }
    else
    {
        ui->statusbar->showMessage(tr("Recorded %1 sec to %2").arg(duration / 1000).arg(m_audioRecorder->outputLocation().toString() + ".wav"));
    }
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
    QString statusMessage;

    if (state == QTextToSpeech::Speaking) {
        statusMessage = tr("Speech started...");
        m_speaking = true;
    } else if (state == QTextToSpeech::Ready)
    {
        m_speaking = false;
        statusMessage = tr("Speech stopped...");
    }
    else if (state == QTextToSpeech::Paused)
        statusMessage = tr("Speech paused...");
    else
        statusMessage = tr("Speech error!");

    ui->statusbar->showMessage(statusMessage);
}

QMediaFormat Ai::selectedMediaFormat() const
{
    QMediaFormat format;
    format.resolveForEncoding(QMediaFormat::NoFlags);
    format.setFileFormat(QMediaFormat::FileFormat::Wave);
    format.setAudioCodec(QMediaFormat::AudioCodec::Wave);
    return format;
}

void Ai::appendText(QString text)
{
    ui->textTerminal->append(text);
}

void Ai::toggleRecord()
{
    if(!m_recording)
    {
        m_recording = true;
#ifndef __APPLE__
        if (m_audioRecorder->recorderState() == QMediaRecorder::StoppedState)
        {
            setOutputFile();
            auto format = selectedMediaFormat();
            m_audioRecorder->setMediaFormat(format);
            m_audioRecorder->setAudioBitRate(16000);
            m_audioRecorder->setAudioSampleRate(sampleRate);
            m_audioRecorder->setAudioChannelCount(channelCount);
            m_audioRecorder->setQuality(QMediaRecorder::HighQuality);
            m_audioRecorder->setEncodingMode(QMediaRecorder::ConstantQualityEncoding);
            m_audioRecorder->record();
        }
#endif
        this->recordDuration = boxValue(ui->recordTimeBox).toInt();
    }
    else
    {
#ifndef __APPLE__
        m_audioRecorder->stop();
#endif
        m_recording = false;
    }
}

void Ai::toggleLanguage()
{
    if(langpair == "tr|en")
    {
        langpair = "en|tr";
        languageCode = "EN";
        ui->languageButton->setText("en|tr");
    }
    else
    {
        langpair = "tr|en";
        languageCode = "TR";
        ui->languageButton->setText("tr|en");
    }
    setSpeechEngine();
}

void Ai::displayErrorMessage()
{
    ui->statusbar->showMessage(m_audioRecorder->errorString());
    appendText(m_audioRecorder->errorString());
    m_recording = false;
}

void Ai::clearAudioLevels()
{
    for (auto m_audioLevel : qAsConst(m_audioLevels))
        m_audioLevel->setLevel(0);
}

QList<qreal> Ai::getBufferLevels(const QAudioBuffer &buffer)
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
            qreal amplifiedValue = value * 10;
            if (amplifiedValue >= m_vox_sensitivity)
            {
                if (!m_recording && !m_speaking)
                {
                    qDebug() << amplifiedValue << m_recording;
                    toggleRecord();
                }
            }
            if (value > max_values.at(j))
                max_values[j] = value;
            data += bytesPerSample;
        }
    }
    return max_values;
}

void Ai::micBufferReady()
{
    qint64 bytesAvailable = m_audioInputSource->bytesAvailable();
    if (bytesAvailable <= 0) {
        return;
    }
    auto buffer = ioInputDevice->read(bytesAvailable);
    const QAudioBuffer &audioBuffer = QAudioBuffer(buffer, audio_format);
    processBuffer(audioBuffer);
#ifdef __APPLE__
    recordBuffer(buffer);
#endif
}

void Ai::processBuffer(const QAudioBuffer& buffer)
{
    QList<qreal> levels = getBufferLevels(buffer);
    for (int i = 0; i < levels.count(); ++i)
        m_audioLevels.at(i)->setLevel(levels.at(i));
}

#ifdef __APPLE__
void Ai::startRecording()
{
    QString filePath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/record.wav";
    ui->statusbar->showMessage(tr("Recording to %1").arg(filePath));
    QByteArray audioFilePathBytes = filePath.toLocal8Bit();
    const char *audioFilePath = audioFilePathBytes.constData();

    fileURL = CFURLCreateFromFileSystemRepresentation(NULL,
                                                      (UInt8*)audioFilePath,
                                                      strlen(audioFilePath),
                                                      false);

    if (!fileURL) {
        CFStringRef description = CFCopyDescription(CFURLCopyFileSystemPath(fileURL, kCFURLPOSIXPathStyle));
        fprintf(stderr, "Couldn't create URL for input file! File path: %s\n", audioFilePath);
        if (description) {
            qDebug() << "Error description:" << QString::fromCFString(description);
            CFRelease(description);
        }
        return;
    }

    // Prepare the audio file for writing
    AudioStreamBasicDescription audioFormat;
    audioFormat.mSampleRate = sampleRate;
    audioFormat.mFormatID = kAudioFormatLinearPCM;
    audioFormat.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
    audioFormat.mBytesPerPacket = sizeof(short) * channelCount;
    audioFormat.mFramesPerPacket = 1;
    audioFormat.mBytesPerFrame = sizeof(short) * channelCount;
    audioFormat.mChannelsPerFrame = channelCount;
    audioFormat.mBitsPerChannel = 8 * sizeof(short);

    OSStatus status = AudioFileCreateWithURL((CFURLRef)fileURL,
                                             kAudioFileWAVEType,
                                             &audioFormat,
                                             kAudioFileFlags_EraseFile,
                                             &audioFile);

    if (status != noErr)
    {
        qDebug() << "Error creating audio file. Error code: " << status;
        return;
    }

    qDebug() << "File path:" << filePath;

    // Release the CFURLRef
    CFRelease(fileURL);
}
#endif
#ifdef __APPLE__
void Ai::stopRecording()
{
    if (audioFile != nullptr) {
        if (fileURL != nullptr) {
            // Log file size after closing
            QFileInfo fileInfo(QString::fromCFString(CFURLCopyFileSystemPath(fileURL, kCFURLPOSIXPathStyle)));
            qDebug() << "Audio file closed. File size:" << fileInfo.size() << " bytes";
        } else {
            qDebug() << "File URL is null in stopRecording function.";
        }
        AudioFileClose(audioFile);
        audioFile = nullptr;
        speechVoice();
    }
}
#endif
#ifdef __APPLE__
void Ai::writeAudioToFile(const QByteArray& buffer)
{
    // Write audio data to the file
    const short* audioData = reinterpret_cast<const short*>(buffer.constData());
    UInt32 numBytesToWrite = buffer.size();
    off_t offset = 0;  // Set the offset to your desired position

    OSStatus status = AudioFileWriteBytes(audioFile,
                                          false,
                                          offset,
                                          &numBytesToWrite,
                                          audioData);

    if (status != noErr)
    {
        qDebug() << "Error writing audio file. Error code: " << status;

        // Log additional information if available
        CFStringRef description = CFCopyDescription(CFURLCopyFileSystemPath(fileURL, kCFURLPOSIXPathStyle));
        if (description) {
            qDebug() << "File path:" << QString::fromCFString(description);
            CFRelease(description);
        }
    }
    else {
        auto msg = QString("Successfully wrote %1 MB to the file.").arg(static_cast<double>(numBytesToWrite) / (1024 * 1024), 0, 'f', 2);
        qDebug() << msg;
        ui->statusbar->showMessage(tr(msg.toStdString().c_str()));
    }
}
#endif
#ifdef __APPLE__
bool Ai::isBufferFull() const
{
    qint64 bufferSize = accumulatedBuffer.size() / (sizeof(short) * channelCount);
    qint64 durationMilliseconds = (bufferSize * 1000) / sampleRate;
    return durationMilliseconds >= this->recordDuration;
    //    return accumulatedBuffer.size() >= 1 * 1024 * 1024; // 1MB
}
#endif
#ifdef __APPLE__
void Ai::recordBuffer(const QByteArray& buffer)
{
    if (m_recording)
    {
        if (!m_record_started)
        {
            startRecording();
            m_record_started = true;
            ui->recordButton->setText(tr("Stop"));
        }

        // Accumulate data in the buffer
        accumulatedBuffer.append(buffer);

        // Check if the buffer is full
        if (isBufferFull()) {
            m_recording = false;
        }
    }
    else if (m_record_started)
    {
        writeAudioToFile(accumulatedBuffer);
        accumulatedBuffer.clear();
        stopRecording();
        m_recording = false;
        m_record_started = false;
        ui->recordButton->setText(tr("Record"));
    }
}
#endif

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
    m_audioInputSource->setVolume(linearVolume);
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


void Ai::on_voxSensivitySlider_valueChanged(int value)
{
    qreal linearVox = QAudio::convertVolume(value / qreal(100),
                                            QAudio::LogarithmicVolumeScale,
                                            QAudio::LinearVolumeScale);
    m_vox_sensitivity = linearVox;
    ui->labelVoxSensivityInfo->setText(QString::number(linearVox, 'f', 2));
}

void Ai::on_languageButton_clicked()
{
    toggleLanguage();
}

