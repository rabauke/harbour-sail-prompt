#ifdef QT_QML_DEBUG
  #include <QtQuick>
#endif

#include <sailfishapp.h>
#include <QLocale>
#include <QtQuick>
#include <QQmlContext>
#include <QTranslator>
#include "AiChatModel/app_model.hpp"


int main(int argc, char *argv[]) {
  QGuiApplication *app = SailfishApp::application(argc, argv);
  QTranslator translator;
  translator.load(QLocale(), "harbour-sail-prompt", "-",
                  SailfishApp::pathTo("translations").toLocalFile());
  app->installTranslator(&translator);
  QQuickView *view = SailfishApp::createView();

  AppModel *appModel = new AppModel(app);
  view->rootContext()->setContextProperty("appModel", appModel);

  view->setSource(SailfishApp::pathToMainQml());
  view->show();

  return app->exec();
}
