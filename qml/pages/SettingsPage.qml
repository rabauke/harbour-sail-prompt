import QtQuick 2.0
import Sailfish.Silica 1.0


Page {
  id: settingsPage

  allowedOrientations: Orientation.All

  property bool updatingModel: false
  property bool updatingApiKey: false
  property bool apiKeyEdited: false
  property bool initialized: false
  property var providerNames: [qsTr('Manual setup'), 'OpenAI', 'Open Router', 'Mistral', 'Mammouth.AI']
  property var providerUrls: ['', 'https://api.openai.com/v1', 'https://openrouter.ai/api/v1',
                             'https://api.mistral.ai/v1', 'https://api.mammouth.ai/v1']

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

      SectionHeader {
        text: qsTr('LLM provider')
      }
      ComboBox {
        id: providerCombo
        width: parent.width
        label: qsTr('Provider')

        menu: ContextMenu {
          Repeater {
            model: settingsPage.providerNames
            MenuItem {
              text: modelData
            }
          }
        }

        onCurrentIndexChanged: {
          if (currentIndex > 0 && currentIndex < settingsPage.providerUrls.length)
            urlField.text = settingsPage.providerUrls[currentIndex]
        }
      }
      TextField {
        id: urlField
        width: parent.width
        label: qsTr('Base URL')
        placeholderText: qsTr('https://api.openai.com/v1')
        readOnly: providerCombo.currentIndex > 0
        inputMethodHints: Qt.ImhUrlCharactersOnly
        EnterKey.iconSource: 'image://theme/icon-m-enter-next'
        EnterKey.onClicked: apiKeyField.focus = true
        validator: RegExpValidator { regExp: /https?:\/\/\S*/ }
      }
      PasswordField {
        id: apiKeyField
        width: parent.width
        label: qsTr('API key')
        placeholderText: qsTr('sk-...')
        enabled: !appModel.credentialOperationPending
        EnterKey.iconSource: 'image://theme/icon-m-enter-next'
        EnterKey.onClicked: systemPromptField.focus = true
        validator: RegExpValidator { regExp: /\S*/ }
        onTextChanged: {
          if (settingsPage.initialized && !settingsPage.updatingApiKey)
            settingsPage.apiKeyEdited = true
        }
      }
      Label {
        id: credentialStatusLabel
        x: Theme.horizontalPageMargin
        width: parent.width - Theme.horizontalPageMargin * 2
        color: appModel.credentialError !== '' ? Theme.errorColor : Theme.secondaryColor
        font.pixelSize: Theme.fontSizeExtraSmall
        wrapMode: Text.WordWrap
        text: {
          if (appModel.credentialOperationPending)
            return appModel.credentialReady ? qsTr('Saving API key securely...') :
                                              qsTr('Loading API key securely...')
          if (appModel.credentialError !== '')
            return qsTr('Secure storage error: %1').arg(appModel.credentialError)
          if (apiKeyField.text.length > 0 && appModel.credentialReady &&
              apiKeyField.text === appModel.apiKey)
            return qsTr('API key is stored securely.')
          return apiKeyField.text.length > 0 ?
                     qsTr('Save the API key securely before refreshing models.') :
                     qsTr('Enter an API key to save it securely.')
        }
      }
      Button {
        anchors.horizontalCenter: parent.horizontalCenter
        text: qsTr('Save API key')
        enabled: !appModel.credentialOperationPending && !appModel.modelsLoading &&
                 apiKeyField.text.trim().length > 0 &&
                 (appModel.credentialError !== '' || !appModel.credentialReady ||
                  apiKeyField.text !== appModel.apiKey)
        onClicked: appModel.applyConfig(urlField.text, apiKeyField.text, systemPromptField.text)
      }

      SectionHeader {
        text: qsTr('System prompt')
      }
      TextArea {
        id: systemPromptField
        width: parent.width
        label: qsTr('System prompt')
        placeholderText: qsTr('You are a helpful assistant.')
      }

      SectionHeader {
        text: qsTr('Model')
      }
      Button {
        anchors.horizontalCenter: parent.horizontalCenter
        text: appModel.modelsLoading ? qsTr('Loading models...') : qsTr('Refresh Models')
        enabled: !appModel.modelsLoading && !appModel.credentialOperationPending &&
                 appModel.credentialReady && appModel.credentialError === '' &&
                 urlField.text.length > 0 && apiKeyField.text.trim().length > 0
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
        text: qsTr('Web view')
      }
      Slider {
        width: parent.width
        minimumValue: 8
        maximumValue: 32
        stepSize: 1
        value: appModel.webViewFontSize
        valueText: Math.round(value) + ' px'
        label: qsTr('Font size')
        onValueChanged: appModel.webViewFontSize = Math.round(value)
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
        width: 1
        height: Theme.paddingLarge
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
      if (message !== appModel.credentialError) {
        errorLabel.text = message
        errorTimer.restart()
      }
    }
    onModelsChanged: settingsPage.updateModelIndex()
    onModelChanged: settingsPage.updateModelIndex()
    onApiKeyChanged: {
      if (!settingsPage.apiKeyEdited) {
        settingsPage.updatingApiKey = true
        apiKeyField.text = apiKey
        settingsPage.updatingApiKey = false
      }
    }
    onCredentialOperationPendingChanged: {
      if (!appModel.credentialOperationPending && appModel.credentialReady &&
          apiKeyField.text === appModel.apiKey)
        settingsPage.apiKeyEdited = false
    }
  }

  Component.onCompleted: {
    urlField.text = appModel.baseUrl
    var providerIndex = 0
    for (var i = 1; i < providerUrls.length; ++i) {
      if (providerUrls[i] === appModel.baseUrl) {
        providerIndex = i
        break
      }
    }
    providerCombo.currentIndex = providerIndex
    apiKeyField.text = appModel.apiKey
    systemPromptField.text = appModel.systemPrompt
    initialized = true
    updateModelIndex()
  }

  onStatusChanged: {
    if (status === PageStatus.Deactivating) {
      appModel.baseUrl = urlField.text
      appModel.systemPrompt = systemPromptField.text
      if (!appModel.credentialOperationPending &&
          (appModel.credentialReady || apiKeyEdited))
        appModel.apiKey = apiKeyField.text
    }
  }
}
