import QtQuick 2.0
import Sailfish.Silica 1.0


Page {
  id: settingsPage
  allowedOrientations: Orientation.All
  property bool updatingModel: false

  function updateModelIndex() {
    updatingModel = true
    var selected = -1
    for (var i = 0; i < appModel.models.length; ++i) {
      if (appModel.models[i] === appModel.model) {
        selected = i
        break
      }
    }
    modelCombo.currentIndex = selected
    updatingModel = false
  }

  SilicaFlickable {
    anchors.fill: parent
    contentHeight: column.height

    Column {
      id: column
      width: parent.width
      spacing: Theme.paddingMedium

      PageHeader {
        title: qsTr('Settings')
      }

      TextField {
        id: urlField
        width: parent.width
        label: qsTr('Base URL')
        placeholderText: qsTr('https://api.openai.com/v1')
        inputMethodHints: Qt.ImhUrlCharactersOnly
        EnterKey.iconSource: 'image://theme/icon-m-enter-next'
        EnterKey.onClicked: apiKeyField.focus = true
      }

      PasswordField {
        id: apiKeyField
        width: parent.width
        label: qsTr('API Key')
        placeholderText: qsTr('sk-...')
        EnterKey.iconSource: 'image://theme/icon-m-enter-next'
        EnterKey.onClicked: systemPromptField.focus = true
      }

      Label {
        x: Theme.horizontalPageMargin
        width: parent.width - Theme.horizontalPageMargin * 2
        color: Theme.secondaryColor
        font.pixelSize: Theme.fontSizeExtraSmall
        wrapMode: Text.WordWrap
        text: qsTr('The API key is stored in the application settings and is not encrypted.')
      }

      TextArea {
        id: systemPromptField
        width: parent.width
        label: qsTr('System Prompt')
        placeholderText: qsTr('You are a helpful assistant.')
      }

      Button {
        anchors.horizontalCenter: parent.horizontalCenter
        text: appModel.modelsLoading ? qsTr('Loading models...') : qsTr('Refresh Models')
        enabled: !appModel.modelsLoading && urlField.text.length > 0 && apiKeyField.text.length > 0
        onClicked: {
          appModel.applyConfig(urlField.text, apiKeyField.text, systemPromptField.text)
          appModel.fetchModels()
        }
      }

      ComboBox {
        id: modelCombo
        width: parent.width
        label: qsTr('Model')
        description: appModel.models.length > 0 ? '' : qsTr('Refresh models to select')
        enabled: appModel.models.length > 0

        menu: ContextMenu {
          Repeater {
            model: appModel.models
            MenuItem {
              text: modelData
            }
          }
        }

        onCurrentIndexChanged: {
          if (!settingsPage.updatingModel && currentIndex >= 0 && currentIndex < appModel.models.length) {
            appModel.model = appModel.models[currentIndex];
          }
        }
      }

      SectionHeader {
        text: qsTr('Configuration State')
      }

      DetailItem {
        label: qsTr('Configured')
        value: appModel.configured ? qsTr('Yes') : qsTr('No')
      }

      DetailItem {
        label: qsTr('Model Selected')
        value: appModel.selectedModelAvailable ? appModel.model : qsTr('None')
      }

      Label {
        id: errorLabel
        x: Theme.horizontalPageMargin
        width: parent.width - Theme.horizontalPageMargin * 2
        color: Theme.errorColor
        wrapMode: Text.WordWrap
        visible: text !== ''
      }

      Item {
        width: 1; height: Theme.paddingLarge
      }
    }

    VerticalScrollDecorator {
    }
  }

  Timer {
    id: errorTimer
    interval: 5000
    onTriggered: errorLabel.text = ''
  }

  Connections {
    target: appModel
    onErrorOccurred: {
      errorLabel.text = message
      errorTimer.restart()
    }
    onModelsChanged: settingsPage.updateModelIndex()
    onModelChanged: settingsPage.updateModelIndex()
  }

  Component.onCompleted: {
    urlField.text = appModel.baseUrl
    apiKeyField.text = appModel.apiKey
    systemPromptField.text = appModel.systemPrompt
    updateModelIndex()
  }

  onStatusChanged: {
    if (status === PageStatus.Deactivating) {
      appModel.applyConfig(urlField.text, apiKeyField.text, systemPromptField.text)
    }
  }
}
