#ifdef QT_QML_DEBUG
  #include <QtQuick>
#endif

#include <QScopedPointer>
#include <QGuiApplication>
#include <QMetaType>
#include <QQmlEngine>
#include <QQuickView>
#include <sailfishapp.h>
#include "AiChatModel/app_model.hpp"


int main(int argc, char *argv[]) {
  QScopedPointer<QGuiApplication> app{SailfishApp::application(argc, argv)};
  app->setApplicationName(QStringLiteral("harbour-sail-prompt"));
  app->setOrganizationName(QStringLiteral("rabauke"));

  qmlRegisterType<AppModel>("SailPromptQuick", 1, 0, "AppModel");

  QScopedPointer<QQuickView> view{SailfishApp::createView()};
  view->setSource(SailfishApp::pathToMainQml());
  view->show();
  return app->exec();
}
