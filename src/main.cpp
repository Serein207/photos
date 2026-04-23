#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include "src/model/PhotoViewModel.h"
#include "src/image/AsyncImageProvider.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    
    QQuickStyle::setStyle("Basic");

    qmlRegisterType<PhotoViewModel>("photos", 1, 0, "PhotoViewModel");

    QQmlApplicationEngine engine;
    
    AsyncImageProvider *imageProvider = AsyncImageProvider::instance();
    engine.addImageProvider("photo_provider", imageProvider);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("photos", "Main");

    return QCoreApplication::exec();
}
