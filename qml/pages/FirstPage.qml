import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.WebView 1.0


Page {
  id: page
  allowedOrientations: Orientation.Portrait | Orientation.PortraitInverted

  property bool isConfigured: appModel.configured && appModel.selectedModelAvailable

  function openSettings() {
    pageStack.animatorPush(Qt.resolvedUrl('SettingsPage.qml'))
  }

  function followLatest() {
    webView.runJavaScript('window.scrollTo(0, document.body.scrollHeight)')
  }

  SilicaFlickable {
    id: chatArea
    anchors.top: parent.top
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.bottom: composerContainer.top
    contentHeight: height

    PullDownMenu {
      MenuItem {
        text: qsTr('About Sail Prompt')
        onClicked: pageStack.push(Qt.resolvedUrl('About.qml'))
      }
      MenuItem {
        text: qsTr('Settings')
        onClicked: page.openSettings()
      }
      MenuItem {
        text: qsTr('Export to PDF')
        enabled: !appModel.busy && !appModel.exporting && appModel.messages.count > 0
        onClicked: appModel.exportToPdf()
      }
      MenuItem {
        text: qsTr('New Chat')
        enabled: !appModel.busy
        onClicked: appModel.newChat()
      }
    }

    WebView {
      id: webView
      anchors.fill: parent

      // The underlying web engine is not fully initialized yet when
      // Component.onCompleted runs, so loading content immediately can
      // crash the engine's worker thread. Defer the initial load to the
      // next event loop iteration, once the view is actually ready.
      Timer {
        id: initialLoadTimer
        interval: 1
        onTriggered: webView.loadHtml(appModel.renderedDocument, 'about:blank')
      }
      Component.onCompleted: initialLoadTimer.start()

      Connections {
        target: appModel
        onRenderedDocumentChanged: webView.loadHtml(appModel.renderedDocument, 'about:blank')
        onMessageAppended: {
          if (!webView.loaded)
            return
          webView.runJavaScript(
                'var end = document.getElementById("end");' +
                'if (end) { end.insertAdjacentHTML("beforebegin", ' + JSON.stringify(html) + '); ' +
                'window.scrollTo(0, document.body.scrollHeight); }')
        }
        onMessageContentUpdated: {
          if (!webView.loaded)
            return
          var script =
              'var el = document.getElementById("msg-' + index + '");' +
              'if (el) { el.innerHTML = ' + JSON.stringify(html) + '; ' +
              'window.scrollTo(0, document.body.scrollHeight); '
          if (finalUpdate)
            script +=
                'if (window.MathJax) { MathJax.typesetPromise().then(function() { ' +
                'window.scrollTo(0, document.body.scrollHeight); }); }'
          script += ' }'
          webView.runJavaScript(script)
        }
      }

      onLinkClicked: {
        var scheme = url.toString().split(':', 1)[0].toLowerCase()
        if (scheme === 'http' || scheme === 'https' || scheme === 'mailto')
          Qt.openUrlExternally(url)
      }

      onLoadedChanged: {
        if (loaded)
          page.followLatest()
      }
    }

    VerticalScrollDecorator {
      flickable: chatArea
    }
  }

  // Guidance for first launch
  ViewPlaceholder {
    enabled: !isConfigured && errorBanner.height === 0
    text: qsTr('Welcome to Sail Prompt')
    hintText: qsTr('Please configure your API settings to start chatting.')
  }

  // Composer at the bottom
  Rectangle {
    id: composerContainer
    anchors.bottom: parent.bottom
    width: parent.width
    height: Math.min(page.height / 3, Math.max(Theme.itemSizeLarge, composer.implicitHeight + Theme.paddingMedium * 2))
    color: Theme.rgba(Theme.overlayBackgroundColor, 0.9)
    z: 5

    TextArea {
      id: composer
      anchors.left: parent.left
      anchors.right: sendButton.left
      anchors.verticalCenter: parent.verticalCenter
      height: Math.min(implicitHeight, parent.height - Theme.paddingSmall * 2)
      placeholderText: qsTr('Type a message...')
      enabled: isConfigured && !appModel.busy
      label: ''
    }

    IconButton {
      id: sendButton
      anchors.right: parent.right
      anchors.bottom: parent.bottom
      anchors.margins: Theme.paddingMedium
      icon.source: appModel.busy ? 'image://theme/icon-m-sync' : 'image://theme/icon-m-enter'
      enabled: isConfigured && !appModel.busy && composer.text.trim().length > 0

      onClicked: {
        appModel.addUserMessage(composer.text)
        composer.text = ''
        composer.focus = false
      }

      RotationAnimation on rotation {
        from: 0;
        to: 360; duration: 1000; loops: Animation.Infinite; running: appModel.busy
      }

      Connections {
        target: appModel
        onBusyChanged: {
          if (!appModel.busy)
            sendButton.rotation = 0
        }
      }
    }
  }

  Item {
    id: errorBanner
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.bottom: composerContainer.top
    height: errorLabel.text === '' ? 0 : errorLabel.height + Theme.paddingMedium * 2
    z: 10
    clip: true
    property bool isError: true

    Rectangle {
      anchors.fill: parent; color: errorBanner.isError ? Theme.errorColor : Theme.highlightBackgroundColor; opacity: 0.9
    }
    Label {
      id: errorLabel
      anchors.centerIn: parent
      width: parent.width - Theme.horizontalPageMargin * 2
      wrapMode: Text.WordWrap
      horizontalAlignment: Text.AlignHCenter
      color: Theme.primaryColor
      font.pixelSize: Theme.fontSizeSmall
    }
  }

  Timer {
    id: errorTimer
    interval: 5000
    onTriggered: errorLabel.text = ''
  }
  Timer {
    id: guidanceTimer
    interval: 1
    onTriggered: {
      if (!page.isConfigured && !appModel.settingsGuidanceShown) {
        appModel.markSettingsGuidanceShown()
        page.openSettings()
      }
    }
  }

  Connections {
    target: appModel
    onErrorOccurred: {
      errorLabel.text = message
      errorBanner.isError = true
      errorTimer.restart()
    }
    onExportErrorChanged: {
      if (appModel.exportError !== '') {
        errorLabel.text = appModel.exportError
        errorBanner.isError = true
        errorTimer.restart()
      }
    }
    onExportSuccessChanged: {
      if (appModel.exportSuccess) {
        errorLabel.text = appModel.exportMessage
        errorBanner.isError = false
        errorTimer.restart()
      }
    }
  }

  Component.onCompleted: guidanceTimer.start()

  onStatusChanged: {
    if (status === PageStatus.Active) {
      pageStack.pushAttached(Qt.resolvedUrl('HistoryPage.qml'))
    }
  }
}
