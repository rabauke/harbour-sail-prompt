#pragma once

#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QSettings>
#include "chat_message_list_model.hpp"
#include "credential_store.hpp"
#include "open_ai_api.hpp"
#include "session_store.hpp"
#include "session_list_model.hpp"
#include "version.hpp"


/// Coordinates application settings, chat requests, credentials, and session history.
class AppModel : public QObject {
  Q_OBJECT

public:
  /// Creates the application model, loads persisted settings, and starts credential loading.
  /// @param parent QObject that owns this model, or `nullptr`.
  explicit AppModel(QObject* parent = nullptr);

  /// Saves the current conversation before destroying the model.
  ~AppModel();

  Q_PROPERTY(QString version MEMBER m_version CONSTANT)
  Q_PROPERTY(QString baseUrl READ baseUrl WRITE setBaseUrl NOTIFY baseUrlChanged)
  Q_PROPERTY(QString apiKey READ apiKey WRITE setApiKey NOTIFY apiKeyChanged)
  Q_PROPERTY(QString model READ model WRITE setModel NOTIFY modelChanged)
  Q_PROPERTY(QStringList models READ models NOTIFY modelsChanged)
  Q_PROPERTY(
      QString systemPrompt READ systemPrompt WRITE setSystemPrompt NOTIFY systemPromptChanged)
  Q_PROPERTY(int webViewFontSize READ webViewFontSize WRITE setWebViewFontSize NOTIFY
                 webViewFontSizeChanged)
  Q_PROPERTY(ChatMessageListModel* messages READ messages CONSTANT)
  Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
  Q_PROPERTY(bool configured READ configured NOTIFY configuredChanged)
  Q_PROPERTY(bool credentialReady READ credentialReady NOTIFY credentialReadyChanged)
  Q_PROPERTY(bool credentialOperationPending READ credentialOperationPending NOTIFY
                 credentialOperationPendingChanged)
  Q_PROPERTY(QString credentialError READ credentialError NOTIFY credentialErrorChanged)
  Q_PROPERTY(bool modelsLoading READ modelsLoading NOTIFY modelsLoadingChanged)
  Q_PROPERTY(bool selectedModelAvailable READ selectedModelAvailable NOTIFY
                 selectedModelAvailableChanged)
  Q_PROPERTY(bool canSend READ canSend NOTIFY canSendChanged)
  Q_PROPERTY(bool canClear READ canClear NOTIFY canClearChanged)
  Q_PROPERTY(bool exporting READ exporting NOTIFY exportingChanged)
  Q_PROPERTY(QString exportError READ exportError NOTIFY exportErrorChanged)
  Q_PROPERTY(bool exportSuccess READ exportSuccess NOTIFY exportSuccessChanged)
  Q_PROPERTY(QString exportPath READ exportPath NOTIFY exportPathChanged)
  Q_PROPERTY(QString exportMessage READ exportMessage NOTIFY exportMessageChanged)
  Q_PROPERTY(QString renderedDocument READ renderedDocument NOTIFY renderedDocumentChanged)
  Q_PROPERTY(bool settingsGuidanceShown READ settingsGuidanceShown CONSTANT)
  Q_PROPERTY(SessionListModel* history READ history CONSTANT)

  /// Adds a user message and starts a chat request when the current configuration permits it.
  /// @param content Text to add to the conversation.
  Q_INVOKABLE void addUserMessage(const QString& content);

  /// Clears all messages in the current conversation.
  Q_INVOKABLE void clearChat();

  /// Applies the API endpoint, key, and system prompt as application configuration.
  /// @param url API base URL.
  /// @param apiKey Credential to save and use for requests.
  /// @param systemPrompt System instruction for chat requests.
  Q_INVOKABLE void applyConfig(const QString& url, const QString& apiKey,
                               const QString& systemPrompt);

  /// Requests the model list from the configured API.
  Q_INVOKABLE void fetchModels();

  /// Records that the initial settings guidance has been shown.
  Q_INVOKABLE void markSettingsGuidanceShown();

  /// Loads a saved conversation by its row in the history model.
  /// @param index Row of the session to load.
  Q_INVOKABLE void loadSession(int index);

  /// Saves the current conversation and starts a new, empty conversation.
  Q_INVOKABLE void newChat();

  /// Exports the current conversation to a Markdown file.
  Q_INVOKABLE void exportToMarkdown();

  /// Returns the configured API base URL.
  QString baseUrl() const;

  /// Sets the API base URL and clears the selected model and available-model list when it changes.
  /// @param baseUrl New API base URL.
  void setBaseUrl(const QString& baseUrl);

  /// Returns the configured API key.
  QString apiKey() const;

  /// Saves and applies a new API key, deferring it while credential loading is pending.
  /// @param apiKey New API key.
  void setApiKey(const QString& apiKey);

  /// Returns the selected model identifier.
  QString model() const;

  /// Selects an available model, or clears the selection.
  /// @param model Model identifier; non-empty values must be in the fetched model list.
  void setModel(const QString& model);

  /// Returns the identifiers of models fetched from the configured API.
  QStringList models() const;

  /// Returns the configured system prompt.
  QString systemPrompt() const;

  /// Sets the system prompt used in subsequent chat requests.
  /// @param systemPrompt New system prompt.
  void setSystemPrompt(const QString& systemPrompt);

  /// Returns the font size used to render chat messages in the web view.
  int webViewFontSize() const;

  /// Sets the web-view font size, clamped to the supported range.
  /// @param fontSize Requested font size in pixels.
  void setWebViewFontSize(int fontSize);

  /// Returns the model containing the current conversation's messages.
  ChatMessageListModel* messages() const;

  /// Returns whether a chat request is in progress.
  bool busy() const;

  /// Returns whether the API endpoint and credentials are configured.
  bool configured() const;

  /// Returns whether credential loading has completed successfully.
  bool credentialReady() const;

  /// Returns whether a credential load or save operation is pending.
  bool credentialOperationPending() const;

  /// Returns the most recent credential-operation error, if any.
  QString credentialError() const;

  /// Returns whether the API model list is currently being loaded.
  bool modelsLoading() const;

  /// Returns whether the selected model is present in the fetched model list.
  bool selectedModelAvailable() const;

  /// Returns whether the current configuration and state permit sending a message.
  bool canSend() const;

  /// Returns whether the current conversation contains messages that can be cleared.
  bool canClear() const;

  /// Returns whether a Markdown export is in progress.
  bool exporting() const;

  /// Returns the most recent Markdown export error, if any.
  QString exportError() const;

  /// Returns whether the most recent Markdown export succeeded.
  bool exportSuccess() const;

  /// Returns the path produced by the most recent successful Markdown export.
  QString exportPath() const;

  /// Returns the status message for the most recent Markdown export.
  QString exportMessage() const;

  /// Returns the rendered HTML document for the current conversation.
  QString renderedDocument() const;

  /// Returns whether the initial settings guidance has already been shown.
  bool settingsGuidanceShown() const;

  /// Returns the model containing saved conversation history.
  SessionListModel* history() const;

  /// Saves the current conversation to session history when it contains a user message.
  void saveCurrentSession();

  /// Emitted when the API base URL changes.
  /// @param baseUrl The new API base URL.
  Q_SIGNAL void baseUrlChanged(const QString& baseUrl);

  /// Emitted when the API key changes.
  /// @param apiKey The new API key.
  Q_SIGNAL void apiKeyChanged(const QString& apiKey);

  /// Emitted when the selected model changes.
  /// @param model The new model identifier.
  Q_SIGNAL void modelChanged(const QString& model);

  /// Emitted when the available model list changes.
  Q_SIGNAL void modelsChanged();

  /// Emitted when the system prompt changes.
  /// @param systemPrompt The new system prompt.
  Q_SIGNAL void systemPromptChanged(const QString& systemPrompt);

  /// Emitted when the web-view font size changes.
  Q_SIGNAL void webViewFontSizeChanged();

  /// Emitted when the chat-request busy state changes.
  Q_SIGNAL void busyChanged();

  /// Emitted when an application operation fails.
  /// @param message Description of the error.
  Q_SIGNAL void errorOccurred(const QString& message);

  /// Emitted when the configured state changes.
  Q_SIGNAL void configuredChanged();

  /// Emitted when credential readiness changes.
  Q_SIGNAL void credentialReadyChanged();

  /// Emitted when credential-operation pending state changes.
  Q_SIGNAL void credentialOperationPendingChanged();

  /// Emitted when the credential error changes.
  Q_SIGNAL void credentialErrorChanged();

  /// Emitted when model-list loading state changes.
  Q_SIGNAL void modelsLoadingChanged();

  /// Emitted when availability of the selected model changes.
  Q_SIGNAL void selectedModelAvailableChanged();

  /// Emitted when the ability to send a message changes.
  Q_SIGNAL void canSendChanged();

  /// Emitted when the ability to clear the conversation changes.
  Q_SIGNAL void canClearChanged();

  /// Emitted when Markdown-exporting state changes.
  Q_SIGNAL void exportingChanged();

  /// Emitted when the Markdown export error changes.
  Q_SIGNAL void exportErrorChanged();

  /// Emitted when the most recent export-success state changes.
  Q_SIGNAL void exportSuccessChanged();

  /// Emitted when the Markdown export path changes.
  Q_SIGNAL void exportPathChanged();

  /// Emitted when the Markdown export status message changes.
  Q_SIGNAL void exportMessageChanged();

  /// Emitted when the rendered HTML document changes.
  Q_SIGNAL void renderedDocumentChanged();

  /// Emitted when a request to load models completes.
  /// @param models Model identifiers returned by the API.
  Q_SIGNAL void modelsLoaded(const QStringList& models);

  /// Emitted when a rendered message is appended to the view.
  /// @param index Index assigned to the message.
  /// @param html Rendered HTML for the message.
  Q_SIGNAL void messageAppended(int index, const QString& html);

  /// Emitted when the rendered content of an existing message changes.
  /// @param index Index of the updated message.
  /// @param html Updated rendered HTML.
  /// @param finalUpdate Whether this is the final update for the message.
  Q_SIGNAL void messageContentUpdated(int index, const QString& html, bool finalUpdate);

private:
  Q_SLOT void onGetModelsFinished(const QList<open_ai_api::Model>& models,
                                  const open_ai_api::Error& error);
  Q_SLOT void onStreamingChatReply(const QString& reply);
  Q_SLOT void onStreamingChatFinished(const QString& reply, const open_ai_api::Error& error);
  Q_SLOT void onStreamingUpdateTimeout();
  Q_SLOT void onCredentialLoadFinished(const QString& apiKey, bool success,
                                       const QString& error);
  Q_SLOT void onCredentialSaveFinished(const QString& apiKey, bool success,
                                       const QString& error);
  Q_SLOT void updateRenderedDocument();

  void loadConfig();
  void saveConfig() const;
  void setCredentialOperationPending(bool pending);
  void setCredentialError(const QString& error);
  void setCredentialReady(bool ready);
  void updateApiKey(const QString& apiKey, bool resetModels);
  void finishCredentialOperation(bool success);
  void startCredentialSave(const QString& apiKey);
  void fetchModelsAfterCredentialOperation();
  void setExportFailure(const QString& error);
  void refreshRenderedDocumentCache();

  QString m_version{QString::fromStdString(project_version)};
  QString m_baseUrl;
  QString m_apiKey;
  QString m_model;
  QStringList m_models;
  QString m_systemPrompt;
  int m_webViewFontSize{16};
  ChatMessageListModel* m_messages;
  open_ai_api::OpenAiApi* m_api;
  bool m_busy;
  bool m_modelsLoading;
  bool m_settingsGuidanceShown;

  int m_assistantMessageIndex;

  bool m_exporting;
  QString m_exportError;
  bool m_exportSuccess;
  QString m_exportPath;
  QString m_exportMessage;

  QString m_renderedDocument;

  QTimer m_streamingUpdateTimer;
  QString m_pendingStreamingContent;

  SessionStore* m_sessionStore;
  SessionListModel* m_sessionListModel;
  QString m_currentSessionId;

  mutable QSettings m_settings;
  CredentialStore* m_credentialStore{nullptr};
  bool m_credentialReady{false};
  bool m_credentialOperationPending{true};
  bool m_hasPendingApiKey{false};
  QString m_pendingApiKey;
  QString m_credentialError;
  bool m_fetchModelsAfterCredentialSave{false};
};
