#ifndef GPT3CLIENT_H
#define GPT3CLIENT_H
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

class GPT3Client : public QObject {
    Q_OBJECT

public:
    GPT3Client(QObject* parent = nullptr) : QObject(parent) {}

    QStringList getWordsFromChatGptNodel(QString prompt) {
        QString apiKey = "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";

        // Make a request to the OpenAI GPT-3 API
        QUrl apiUrl;
        apiUrl.setUrl("https://api.openai.com/v1/engines/davinci/completions");
        QNetworkRequest request(apiUrl);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("Authorization", "Bearer " + apiKey.toUtf8());

        // Construct the JSON request payload
        QJsonObject requestData;
        requestData["prompt"] = prompt;
        requestData["max_tokens"] = 10; // Adjust the number of tokens based on your needs

        QJsonDocument requestDoc(requestData);
        QByteArray requestDataBytes = requestDoc.toJson();

        // Perform the API request synchronously (for simplicity)
        QNetworkAccessManager manager;
        QNetworkReply* reply = manager.post(request, requestDataBytes);
        QEventLoop loop;
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        // Process the API response
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);
            QJsonObject responseObject = responseDoc.object();
            QJsonArray choices = responseObject["choices"].toArray();
            qDebug() << "API Response:" << responseData;

            QStringList newWords;
            for (const auto& choice : choices) {
                QString word = choice.toObject()["text"].toString();
                newWords << word.trimmed();
            }

            return newWords;
        } else {
            qDebug() << "Error fetching new words:" << reply->errorString();
            qDebug() << "API Response:" << reply->readAll();
            return QStringList();
        }
    }

    QString getAnswerFromChatGpt(QString prompt) {
        QString apiKey = "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";

        // Make a request to the OpenAI GPT-3 API
        QUrl apiUrl("https://api.openai.com/v1/engines/davinci/completions");
        QNetworkRequest request(apiUrl);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("Authorization", "Bearer " + apiKey.toUtf8());

        // Construct the JSON request payload
        QJsonObject requestData;
        requestData["prompt"] = prompt;
        requestData["max_tokens"] = 10; // Adjust the number of tokens based on your needs

        QJsonDocument requestDoc(requestData);
        QByteArray requestDataBytes = requestDoc.toJson();

        // Perform the API request synchronously (for simplicity)
        QNetworkAccessManager manager;
        QNetworkReply* reply = manager.post(request, requestDataBytes);
        QEventLoop loop;
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        // Check if the request was successful
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);

            if (responseDoc.isObject()) {
                QJsonObject responseObject = responseDoc.object();
                QString jsonString = QString::fromUtf8(responseDoc.toJson());
                QJsonArray choices = responseObject["choices"].toArray();
                qDebug() << "API Response:" << responseData;

                QStringList newWords;
                for (const auto& choice : choices) {
                    QString word = choice.toObject()["text"].toString();
                    qDebug() << word;
                }

                return jsonString;
            } else {
                qDebug() << "Invalid JSON response:" << responseData;
                return "Invalid JSON response";
            }
        } else {
            QString errorMessage = reply->errorString();
            qDebug() << errorMessage;
            return errorMessage;
        }
    }

};

#endif // GPT3CLIENT_H
