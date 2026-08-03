import QtQuick 2.2
import Sailfish.Silica 1.0
import '../components'


Page {
  id: about_page

  allowedOrientations: Orientation.All

  SilicaFlickable {
    anchors.fill: parent
    contentHeight: column.height

    Column {
      id: column

      width: parent.width
      spacing: 0
      PageHeader {
        title: 'Sail Prompt v' + appModel.version
      }
      Label {
        x: Theme.horizontalPageMargin
        width: column.width - 2 * x
        color: Theme.primaryColor
        wrapMode: TextEdit.Wrap
        font.pixelSize: Theme.fontSizeMedium
        text: qsTr('_description_')
      }
      SectionHeader {
        text: qsTr('Copyright')
      }
      Label {
        x: Theme.horizontalPageMargin
        width: column.width - 2 * x
        color: Theme.primaryColor
        wrapMode: TextEdit.Wrap
        font.pixelSize: Theme.fontSizeMedium
        text: '© Heiko Bauke, 2026'
      }
      SectionHeader {
        text: qsTr('License')
      }
      Label {
        x: Theme.horizontalPageMargin
        width: column.width - 2 * x
        color: Theme.primaryColor
        wrapMode: TextEdit.Wrap
        font.pixelSize: Theme.fontSizeMedium
        text: 'GNU General Public License v2.0 or any later version'
      }
      Label {
        x: Theme.horizontalPageMargin
        width: column.width - 2 * x
        color: Theme.primaryColor
        wrapMode: TextEdit.Wrap
        font.pixelSize: Theme.fontSizeMedium
        textFormat: Text.StyledText
        linkColor: Theme.highlightColor
        onLinkActivated: {
          Qt.openUrlExternally(link)
        }
        text: '<br>Fork me on <a href=\"https://github.com/rabauke/harbour-sail-prompt\">github</a>!'
      }
      SectionHeader {
        text: qsTr('External components')
      }
      ThirdPartyComponent {
        componnet: 'md4c'
        version: '0.5.3'
        license: 'MIT License'
        url: 'https://github.com/mity/md4c'
      }
      ThirdPartyComponent {
        componnet: 'Qt'
        version: '5.6.3'
        license: 'LGPL-2.1-only, GPL-2.0-only'
        url: 'https://www.qt.io'
      }
      ThirdPartyComponent {
        componnet: 'Sailfish Silica UI'
        version: '2.0'
        license: 'BSD-3-Clause, proprietary'
        url: 'https://sailfishos.org'
      }
      ThirdPartyComponent {
        componnet: 'Libsailfishapp'
        version: '1.2.15'
        license: 'LGPL-2.1-only'
        url: 'https://github.com/sailfishos/libsailfishapp'
      }
    }
  }
}
