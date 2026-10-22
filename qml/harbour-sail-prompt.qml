import QtQuick 2.0
import Sailfish.Silica 1.0
import SailPromptQuick 1.0
import 'cover'
import 'pages'


ApplicationWindow {
  id: appView

  AppModel {
    id: appModel
  }

  initialPage: Qt.resolvedUrl('pages/MainPage.qml')

  cover: Component {
    CoverPage {
      appWindow: appView
    }
  }

  allowedOrientations: defaultAllowedOrientations
}
