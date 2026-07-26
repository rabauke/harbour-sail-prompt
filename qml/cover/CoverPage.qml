import QtQuick 2.0
import Sailfish.Silica 1.0

CoverBackground {
    Column {
        anchors.centerIn: parent
        width: parent.width - Theme.paddingLarge * 2
        spacing: Theme.paddingMedium

        Icon {
            anchors.horizontalCenter: parent.horizontalCenter
            source: "image://theme/icon-l-answer"
            opacity: 0.2
        }

        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: appModel.model !== "" ? appModel.model : qsTr("Sail Prompt")
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.highlightColor
            wrapMode: Text.WordWrap
        }

        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: appModel.busy ? qsTr("Thinking...") : qsTr("Ready")
            font.pixelSize: Theme.fontSizeTiny
            color: Theme.secondaryHighlightColor
        }
    }

    CoverActionList {
        id: coverAction

        CoverAction {
            iconSource: "image://theme/icon-s-plus"
            onTriggered: appModel.clearChat()
        }
    }
}
