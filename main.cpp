#include "ai.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    Ai ai;
    ai.show();

    return app.exec();
}
