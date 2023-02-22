#ifndef CONSTANTS_H
#define CONSTANTS_H
#include <QString>

static QString speechBaseApi = "https://speech.googleapis.com/v1/speech:recognize";
static QString speechApiKey = "api key";
static QString baseWikiApi = "https://tr.wikipedia.org/w/api.php";
static QString baseChatGPT = "https://api.openai.com/v1/completions";
static QString chatGPTApiKey = "api key";

//curl https://api.openai.com/v1/completions \
//  -H 'Content-Type: application/json' \
//  -H 'Authorization: Bearer api key' \
//  -d '{
//  "model": "text-davinci-003",
//  "prompt": "Say this is a test",
//  "max_tokens": 7,
//  "temperature": 0
//}'
#endif // CONSTANTS_H
