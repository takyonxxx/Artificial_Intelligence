#include "ai.h"
#include "ui_ai.h"
#include "constants.h"
#include "audiolevel.h"

static qreal getPeakValue(const QAudioFormat &format);
static QVector<qreal> getBufferLevels(const QAudioBuffer &buffer);

template <class T>
static QVector<qreal> getBufferLevels(const T *buffer, int frames, int channels);

Ai::Ai()
    : ui(new Ui::Ai)
{
    ui->setupUi(this);
    //this->centralWidget()->setStyleSheet("background-color:lightgray ; border: none;");
    ui->recordButton->setStyleSheet("font-size: 18pt; font-weight: bold; color: white;background-color:#154360; padding: 12px; spacing: 12px;");
    ui->exitButton->setStyleSheet("font-size: 18pt; font-weight: bold; color: white;background-color:#154360; padding: 12px; spacing: 12px;");
    ui->textTerminal->setStyleSheet("font: 14pt; color: #00cccc; background-color: #001a1a;");

    m_audioRecorder = new QAudioRecorder(this);
    m_probe = new QAudioProbe(this);
    connect(m_probe, &QAudioProbe::audioBufferProbed, this, &Ai::processBuffer);
    m_probe->setSource(m_audioRecorder);
    qnam = new QNetworkAccessManager(this);

    if (!this->location.exists())
        this->location.mkpath(".");

    this->filePath = location.filePath(fileName);

    this->url.setUrl(baseApi);
    this->url.setQuery("key=" + apiKey);

    this->request.setUrl(this->url);
    this->request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    //audio devices
    //ui->audioDeviceBox->addItem(tr("Default"), QVariant(QString()));
    for (auto &device: m_audioRecorder->audioInputs()) {
        ui->audioDeviceBox->addItem(device, QVariant(device));
    }

//    //sample rates
//    for (int sampleRate: m_audioRecorder->supportedAudioSampleRates()) {
//        qDebug() << sampleRate;
//    }

    //record times
    ui->recordTimeBox->addItem(QStringLiteral("1000"), QVariant(1000));
    ui->recordTimeBox->addItem(QStringLiteral("2000"), QVariant(2000));
    ui->recordTimeBox->addItem(QStringLiteral("2000"), QVariant(3000));

    connect(m_audioRecorder, &QAudioRecorder::durationChanged, this, &Ai::updateProgress);
    connect(m_audioRecorder, &QAudioRecorder::statusChanged, this, &Ai::updateStatus);
    connect(m_audioRecorder, &QAudioRecorder::stateChanged, this, &Ai::onStateChanged);
    connect(m_audioRecorder, QOverload<QMediaRecorder::Error>::of(&QAudioRecorder::error), this, &Ai::displayErrorMessage);
    connect(qnam, &QNetworkAccessManager::finished, this, &Ai::onResponseFinish);

    byteArr.reserve(8*1024*1024);
}

void Ai::updateProgress(qint64 duration)
{
    if (m_audioRecorder->error() != QMediaRecorder::NoError)
        return;

    if (duration >= this->recordDuration &&
            this->m_audioRecorder->state() != QAudioRecorder::StoppedState &&
            this->m_audioRecorder->status() == QAudioRecorder::Status::RecordingStatus)
    {
        this->m_audioRecorder->stop();
        translate();
    }

    ui->statusbar->showMessage(tr("Recorded %1 sec").arg(duration / 1000));
}

void Ai::updateStatus(QMediaRecorder::Status status)
{
    QString statusMessage;

    switch (status) {
    case QMediaRecorder::RecordingStatus:
        statusMessage = tr("Recording to %1").arg(m_audioRecorder->outputLocation().toString());
        break;
    case QMediaRecorder::PausedStatus:
        clearAudioLevels();
        statusMessage = tr("Paused");
        break;
    case QMediaRecorder::UnloadedStatus:
    case QMediaRecorder::LoadedStatus:
        clearAudioLevels();
        statusMessage = tr("Stopped");
    default:
        break;
    }

    if (m_audioRecorder->error() == QMediaRecorder::NoError)
        ui->statusbar->showMessage(statusMessage);
}

void Ai::onStateChanged(QMediaRecorder::State state)
{
    switch (state) {
    case QMediaRecorder::RecordingState:
        ui->recordButton->setText(tr("Stop"));
        break;
    case QMediaRecorder::PausedState:
        ui->recordButton->setText(tr("Stop"));
        break;
    case QMediaRecorder::StoppedState:
        ui->recordButton->setText(tr("Record"));
        break;
    }
}

static QVariant boxValue(const QComboBox *box)
{
    int idx = box->currentIndex();
    if (idx == -1)
        return QVariant();

    return box->itemData(idx);
}

void Ai::toggleRecord()
{
    if (m_audioRecorder->state() == QMediaRecorder::StoppedState) {

        m_audioRecorder->setAudioInput(boxValue(ui->audioDeviceBox).toString());
        this->recordDuration = boxValue(ui->recordTimeBox).toInt();

        settings.setCodec("audio/pcm");
        settings.setSampleRate(44100);
        settings.setChannelCount(1);
        QString container = "audio/x-wav";

        m_audioRecorder->setEncodingSettings(settings, QVideoEncoderSettings(), container);
        m_audioRecorder->setOutputLocation(filePath);
        m_audioRecorder->record();       
    }
    else {
        m_audioRecorder->stop();
    }
}

void Ai::translate()
{
    this->file.setFileName(this->m_audioRecorder->outputLocation().toString().remove("file:/"));

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
                    {"sampleRateHertz", settings.sampleRate()}
                }}
        }
    };

    qnam->post(this->request, data.toJson(QJsonDocument::Compact));
}

void Ai::onResponseFinish(QNetworkReply *response)
{
    auto data = QJsonDocument::fromJson(response->readAll());
    response->deleteLater();

    QString strFromJson = QJsonDocument(data).toJson(QJsonDocument::Compact).toStdString().c_str();

    auto error = data["error"]["message"];

    if (error.isUndefined()) {
        auto command = data["results"][0]["alternatives"][0]["transcript"].toString();
        if (command.size() > 0)
            appendText(command);
        else
            appendText("Null");
    }
}

void Ai::appendText(QString text)
{
    ui->textTerminal->append(text);
}

void Ai::togglePause()
{
    if (m_audioRecorder->state() != QMediaRecorder::PausedState)
        m_audioRecorder->pause();
    else
        m_audioRecorder->record();
}

void Ai::displayErrorMessage()
{
    ui->statusbar->showMessage(m_audioRecorder->errorString());
}

void Ai::clearAudioLevels()
{
    for (int i = 0; i < m_audioLevels.size(); ++i)
        m_audioLevels.at(i)->setLevel(0);
}

// This function returns the maximum possible sample value for a given audio format
qreal getPeakValue(const QAudioFormat& format)
{
    // Note: Only the most common sample formats are supported
    if (!format.isValid())
        return qreal(0);

    if (format.codec() != "audio/pcm")
        return qreal(0);

    switch (format.sampleType()) {
    case QAudioFormat::Unknown:
        break;
    case QAudioFormat::Float:
        if (format.sampleSize() != 32) // other sample formats are not supported
            return qreal(0);
        return qreal(1.00003);
    case QAudioFormat::SignedInt:
        if (format.sampleSize() == 32)
            return qreal(INT_MAX);
        if (format.sampleSize() == 16)
            return qreal(SHRT_MAX);
        if (format.sampleSize() == 8)
            return qreal(CHAR_MAX);
        break;
    case QAudioFormat::UnSignedInt:
        if (format.sampleSize() == 32)
            return qreal(UINT_MAX);
        if (format.sampleSize() == 16)
            return qreal(USHRT_MAX);
        if (format.sampleSize() == 8)
            return qreal(UCHAR_MAX);
        break;
    }

    return qreal(0);
}

// returns the audio level for each channel
QVector<qreal> getBufferLevels(const QAudioBuffer& buffer)
{
    QVector<qreal> values;

    if (!buffer.format().isValid() || buffer.format().byteOrder() != QAudioFormat::LittleEndian)
        return values;

    if (buffer.format().codec() != "audio/pcm")
        return values;

    int channelCount = buffer.format().channelCount();
    values.fill(0, channelCount);
    qreal peak_value = getPeakValue(buffer.format());
    if (qFuzzyCompare(peak_value, qreal(0)))
        return values;

    switch (buffer.format().sampleType()) {
    case QAudioFormat::Unknown:
    case QAudioFormat::UnSignedInt:
        if (buffer.format().sampleSize() == 32)
            values = getBufferLevels(buffer.constData<quint32>(), buffer.frameCount(), channelCount);
        if (buffer.format().sampleSize() == 16)
            values = getBufferLevels(buffer.constData<quint16>(), buffer.frameCount(), channelCount);
        if (buffer.format().sampleSize() == 8)
            values = getBufferLevels(buffer.constData<quint8>(), buffer.frameCount(), channelCount);
        for (int i = 0; i < values.size(); ++i)
            values[i] = qAbs(values.at(i) - peak_value / 2) / (peak_value / 2);
        break;
    case QAudioFormat::Float:
        if (buffer.format().sampleSize() == 32) {
            values = getBufferLevels(buffer.constData<float>(), buffer.frameCount(), channelCount);
            for (int i = 0; i < values.size(); ++i)
                values[i] /= peak_value;
        }
        break;
    case QAudioFormat::SignedInt:
        if (buffer.format().sampleSize() == 32)
            values = getBufferLevels(buffer.constData<qint32>(), buffer.frameCount(), channelCount);
        if (buffer.format().sampleSize() == 16)
            values = getBufferLevels(buffer.constData<qint16>(), buffer.frameCount(), channelCount);
        if (buffer.format().sampleSize() == 8)
            values = getBufferLevels(buffer.constData<qint8>(), buffer.frameCount(), channelCount);
        for (int i = 0; i < values.size(); ++i)
            values[i] /= peak_value;
        break;
    }

    return values;
}

template <class T>
QVector<qreal> getBufferLevels(const T *buffer, int frames, int channels)
{
    QVector<qreal> max_values;
    max_values.fill(0, channels);

    for (int i = 0; i < frames; ++i) {
        for (int j = 0; j < channels; ++j) {
            qreal value = qAbs(qreal(buffer[i * channels + j]));
            if (value > max_values.at(j))
                max_values.replace(j, value);
        }
    }

    return max_values;
}

void Ai::processBuffer(const QAudioBuffer& buffer)
{
//    byteArr.setRawData(buffer.constData<char>(), buffer.byteCount());

    if (m_audioLevels.count() != buffer.format().channelCount()) {
        qDeleteAll(m_audioLevels);
        m_audioLevels.clear();
        for (int i = 0; i < buffer.format().channelCount(); ++i) {
            AudioLevel *level = new AudioLevel(ui->centralwidget);
            m_audioLevels.append(level);
            ui->levelsLayout->addWidget(level);
        }
    }

    QVector<qreal> levels = getBufferLevels(buffer);
    for (int i = 0; i < levels.count(); ++i)
        m_audioLevels.at(i)->setLevel(levels.at(i));
}

void Ai::on_recordButton_clicked()
{
    toggleRecord();
}

void Ai::on_exitButton_clicked()
{
    QApplication::quit();
}

