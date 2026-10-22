import QtQuick 2.0
import Sailfish.Silica 1.0


CoverBackground {
  property var appWindow

  Icon {
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.verticalCenter: parent.verticalCenter
    anchors.verticalCenterOffset: -150
    source: '/usr/share/harbour-sail-prompt/images/cover.png'
    opacity: 0.6667
    scale: 0.6
  }

  Column {
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.verticalCenter: parent.verticalCenter
    anchors.verticalCenterOffset: 120
    width: parent.width - Theme.paddingLarge * 2
    spacing: Theme.paddingLarge

    Label {
      width: parent.width
      horizontalAlignment: Text.AlignHCenter
      text: appModel.model !== '' ? appModel.model : qsTr('Sail Prompt')
      font.pixelSize: Theme.fontSizeMedium
      color: Theme.highlightColor
      wrapMode: Text.WordWrap
    }

    Label {
      width: parent.width
      horizontalAlignment: Text.AlignHCenter
      text: appModel.busy ? qsTr('Thinking...') : qsTr('Ready')
      font.pixelSize: Theme.fontSizeSmall
      color: Theme.secondaryHighlightColor
    }
  }

  CoverActionList {
    enabled: appModel.configured && !appModel.busy

    CoverAction {
      iconSource: 'image://theme/icon-cover-message'
      onTriggered: {
        appModel.newChat()
        appWindow.activate()
      }
    }
  }

}
