#include "ai.h"

#include <QtWidgets>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    Ai recorder;
    recorder.show();

    return app.exec();
}
