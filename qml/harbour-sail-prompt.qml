import QtQuick 2.0
import Sailfish.Silica 1.0
import SailPromptQuick 1.0
import 'pages'


ApplicationWindow {
  id: appView

  AppModel {
    id: appModel
  }

  initialPage: Qt.resolvedUrl('pages/FirstPage.qml')
  cover: Qt.resolvedUrl('cover/CoverPage.qml')
  allowedOrientations: defaultAllowedOrientations
}
