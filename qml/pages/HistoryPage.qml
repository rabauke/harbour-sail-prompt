import QtQuick 2.0
import Sailfish.Silica 1.0


Page {
  id: historyPage
  allowedOrientations: Orientation.All

  SilicaListView {
    id: listView
    model: appModel.history
    anchors.fill: parent

    header: PageHeader {
      title: qsTr('History')
    }

    delegate: ListItem {
      id: delegate
      contentHeight: Theme.itemSizeLarge

      Column {
        anchors.fill: parent
        anchors.leftMargin: Theme.horizontalPageMargin
        anchors.rightMargin: Theme.horizontalPageMargin
        anchors.verticalCenter: parent.verticalCenter

        Row {
          width: parent.width
          Label {
            text: model.title
            color: delegate.highlighted ? Theme.highlightColor : Theme.primaryColor
            width: parent.width - timestampLabel.width
            elide: Text.ElideRight
          }
          Label {
            id: timestampLabel
            text: Qt.formatDateTime(model.timestamp, 'd MMM, hh:mm')
            font.pixelSize: Theme.fontSizeExtraSmall
            color: delegate.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
          }
        }

        Label {
          text: model.preview
          width: parent.width
          font.pixelSize: Theme.fontSizeExtraSmall
          color: delegate.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
          elide: Text.ElideRight
        }
      }

      onClicked: {
        appModel.loadSession(index)
        pageStack.pop()
      }

      menu: ContextMenu {
        MenuItem {
          text: qsTr('Delete')
          onClicked: {
            var idToDelete = model.id
            remorseAction(qsTr('Deleting'), function () {
              appModel.history.deleteSessionById(idToDelete)
            })
          }
        }
      }
    }

    VerticalScrollDecorator {
    }

    ViewPlaceholder {
      enabled: listView.count === 0
      text: qsTr('No history yet')
    }
  }

  Component.onCompleted: appModel.history.refresh()
}
