import QtQuick 2.0
import Sailfish.Silica 1.0
import SailPromptQuick 1.0


Page {
  id: historyPage
  allowedOrientations: Orientation.All

  DateTimeFormater {
    id: dateTimeFormater
  }

  SilicaListView {
    id: listView
    model: appModel.history
    anchors.fill: parent

    header: PageHeader {
      title: qsTr('History')
    }

    delegate: ListItem {
      id: delegate
      contentHeight: Theme.itemSizeMedium

      Column {
        anchors.fill: parent
        anchors.topMargin: Theme.paddingSmall
        anchors.leftMargin: Theme.horizontalPageMargin
        anchors.rightMargin: Theme.horizontalPageMargin
        anchors.verticalCenter: parent.verticalCenter

        Row {
          width: parent.width
          spacing: Theme.paddingSmall

          Label {
            text: model.title
            color: delegate.highlighted ? Theme.highlightColor : Theme.primaryColor
            width: parent.width - timestampLabel.width - Theme.paddingSmall
            truncationMode: TruncationMode.Fade
          }
          Label {
            id: timestampLabel
            text: dateTimeFormater.formatTime(model.timestamp)
            font.pixelSize: Theme.fontSizeExtraSmall
            color: delegate.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
          }
        }

        Label {
          text: model.preview
          width: parent.width
          font.pixelSize: Theme.fontSizeExtraSmall
          color: delegate.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
          truncationMode: TruncationMode.Fade
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
              var idToDelete = index
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
