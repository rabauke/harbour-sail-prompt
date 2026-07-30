#ifdef QT_QML_DEBUG
  #include <QtQuick>
#endif

#include <QScopedPointer>
#include <QLocale>
#include <QtQuick>
#include <QQmlContext>
#include <QTranslator>
#include <sailfishapp.h>
#include "AiChatModel/app_model.hpp"


int main(int argc, char *argv[]) {
  QScopedPointer<QGuiApplication> app{SailfishApp::application(argc, argv)};
  app->setApplicationName(QStringLiteral("harbour-sail-prompt"));
  app->setOrganizationName(QStringLiteral("rabauke"));

  QQuickView *view{SailfishApp::createView()};

  AppModel *appModel{new AppModel(app.take())};
  view->rootContext()->setContextProperty("appModel", appModel);
  view->setSource(SailfishApp::pathToMainQml());
  view->show();

  return app->exec();
}
