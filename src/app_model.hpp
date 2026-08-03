#pragma once

#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QSettings>
#include "chat_message_list_model.hpp"
#include "open_ai_api.hpp"
#include "session_store.hpp"
#include "session_list_model.hpp"
#include "version.hpp"


class AppModel : public QObject {
  Q_OBJECT

public:
  explicit AppModel(QObject* parent = nullptr);
  ~AppModel();

  Q_PROPERTY(QString version MEMBER m_version CONSTANT)
  Q_PROPERTY(QString baseUrl READ baseUrl WRITE setBaseUrl NOTIFY baseUrlChanged)
  Q_PROPERTY(QString apiKey READ apiKey WRITE setApiKey NOTIFY apiKeyChanged)
  Q_PROPERTY(QString model READ model WRITE setModel NOTIFY modelChanged)
  Q_PROPERTY(QStringList models READ models NOTIFY modelsChanged)
  Q_PROPERTY(
      QString systemPrompt READ systemPrompt WRITE setSystemPrompt NOTIFY systemPromptChanged)
  Q_PROPERTY(ChatMessageListModel* messages READ messages CONSTANT)
  Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
  Q_PROPERTY(bool configured READ configured NOTIFY configuredChanged)
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

  Q_INVOKABLE void addUserMessage(const QString& content);
  Q_INVOKABLE void clearChat();
  Q_INVOKABLE void applyConfig(const QString& url, const QString& apiKey,
                               const QString& systemPrompt);
  Q_INVOKABLE void fetchModels();
  Q_INVOKABLE void markSettingsGuidanceShown();
  Q_INVOKABLE void loadSession(int index);
  Q_INVOKABLE void newChat();
  Q_INVOKABLE void exportToPdf();

  QString baseUrl() const;
  void setBaseUrl(const QString& baseUrl);
  QString apiKey() const;
  void setApiKey(const QString& apiKey);
  QString model() const;
  void setModel(const QString& model);
  QStringList models() const;
  QString systemPrompt() const;
  void setSystemPrompt(const QString& systemPrompt);
  ChatMessageListModel* messages() const;
  bool busy() const;
  bool configured() const;
  bool modelsLoading() const;
  bool selectedModelAvailable() const;
  bool canSend() const;
  bool canClear() const;
  bool exporting() const;
  QString exportError() const;
  bool exportSuccess() const;
  QString exportPath() const;
  QString exportMessage() const;
  QString renderedDocument() const;
  bool settingsGuidanceShown() const;
  SessionListModel* history() const;

  void saveCurrentSession();

  Q_SIGNAL void baseUrlChanged(const QString& baseUrl);
  Q_SIGNAL void apiKeyChanged(const QString& apiKey);
  Q_SIGNAL void modelChanged(const QString& model);
  Q_SIGNAL void modelsChanged();
  Q_SIGNAL void systemPromptChanged(const QString& systemPrompt);
  Q_SIGNAL void busyChanged();
  Q_SIGNAL void errorOccurred(const QString& message);
  Q_SIGNAL void configuredChanged();
  Q_SIGNAL void modelsLoadingChanged();
  Q_SIGNAL void selectedModelAvailableChanged();
  Q_SIGNAL void canSendChanged();
  Q_SIGNAL void canClearChanged();
  Q_SIGNAL void exportingChanged();
  Q_SIGNAL void exportErrorChanged();
  Q_SIGNAL void exportSuccessChanged();
  Q_SIGNAL void exportPathChanged();
  Q_SIGNAL void exportMessageChanged();
  Q_SIGNAL void renderedDocumentChanged();
  Q_SIGNAL void modelsLoaded(const QStringList& models);

private:
  Q_SLOT void onGetModelsFinished(const QList<open_ai_api::Model>& models,
                                  const open_ai_api::Error& error);
  Q_SLOT void onStreamingChatReply(const QString& reply);
  Q_SLOT void onStreamingChatFinished(const QString& reply, const open_ai_api::Error& error);
  Q_SLOT void onStreamingUpdateTimeout();
  Q_SLOT void updateRenderedDocument();

  void loadConfig();
  void saveConfig() const;
  void setExportFailure(const QString& error);

  QString m_version{QString::fromStdString(project_version)};
  QString m_baseUrl;
  QString m_apiKey;
  QString m_model;
  QStringList m_models;
  QString m_systemPrompt;
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
};
