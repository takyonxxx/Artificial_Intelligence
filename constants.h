#ifndef CONSTANTS_H
#define CONSTANTS_H
#include <QString>

static QString baseApi = "https://speech.googleapis.com/v1/speech:recognize";
static QString apiKey = "AIzaSysilCY8Xg5wfn6Ld67287SGDBQPZvGCEN6Fsg";

#endif // CONSTANTS_H


//QAudioFormat desiredFormat;
//desiredFormat.setChannelCount(2);
//desiredFormat.setCodec("audio/x-raw");
//desiredFormat.setSampleType(QAudioFormat::UnSignedInt);
//desiredFormat.setSampleRate(48000);
//desiredFormat.setSampleSize(16);

//QAudioDecoder *decoder = new QAudioDecoder(this);
//decoder->setAudioFormat(desiredFormat);
//decoder->setSourceFilename("level1.mp3");

//connect(decoder, SIGNAL(bufferReady()), this, SLOT(readBuffer()));
//decoder->start();

//// Now wait for bufferReady() signal and call decoder->read()
