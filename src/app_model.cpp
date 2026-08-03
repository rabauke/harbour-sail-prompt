#include "app_model.hpp"
#include "html_renderer.hpp"
#include "pdf_exporter.hpp"
#include <QCoreApplication>
#include <QStandardPaths>
#include <QUrl>


namespace {

QString settings_path() {
  return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/" +
         QCoreApplication::applicationName() + ".conf";
}

}  // namespace


AppModel::AppModel(QObject* parent)
    : QObject(parent),
      m_messages(new ChatMessageListModel(this)),
      m_api(new open_ai_api::OpenAiApi(this)),
      m_busy(false),
      m_modelsLoading(false),
      m_settingsGuidanceShown(false),
      m_assistantMessageIndex(-1),
      m_exporting(false),
      m_exportSuccess(false),
      m_sessionStore(new SessionStore(this)),
      m_sessionListModel(new SessionListModel(m_sessionStore, this)),
      m_settings(settings_path(), QSettings::NativeFormat) {
  connect(m_sessionListModel, &SessionListModel::errorOccurred, this, &AppModel::errorOccurred);
  loadConfig();

  m_streamingUpdateTimer.setInterval(100);
  m_streamingUpdateTimer.setSingleShot(false);
  connect(&m_streamingUpdateTimer, SIGNAL(timeout()), this, SLOT(onStreamingUpdateTimeout()));

  connect(m_api, SIGNAL(getModelsFinished(QList<open_ai_api::Model>, open_ai_api::Error)), this,
          SLOT(onGetModelsFinished(QList<open_ai_api::Model>, open_ai_api::Error)));
  connect(m_api, SIGNAL(streamingChatReply(QString)), this,
          SLOT(onStreamingChatReply(QString)));
  connect(m_api, SIGNAL(streamingChatFinished(QString, open_ai_api::Error)), this,
          SLOT(onStreamingChatFinished(QString, open_ai_api::Error)));

  updateRenderedDocument();
}


AppModel::~AppModel() {
  saveCurrentSession();
}


QString AppModel::baseUrl() const {
  return m_baseUrl;
}


void AppModel::setBaseUrl(const QString& baseUrl) {
  const QString normalized = baseUrl.trimmed();
  if (m_baseUrl != normalized) {
    m_baseUrl = normalized;
    m_api->setBaseUrl(QUrl(m_baseUrl));
    m_models.clear();
    m_model.clear();
    Q_EMIT baseUrlChanged(m_baseUrl);
    Q_EMIT modelsChanged();
    Q_EMIT modelChanged(m_model);
    Q_EMIT configuredChanged();
    Q_EMIT selectedModelAvailableChanged();
    Q_EMIT canSendChanged();
  }
}


QString AppModel::apiKey() const {
  return m_apiKey;
}


void AppModel::setApiKey(const QString& apiKey) {
  if (m_apiKey != apiKey) {
    m_apiKey = apiKey;
    m_api->setApiKey(m_apiKey);
    m_models.clear();
    m_model.clear();
    Q_EMIT apiKeyChanged(m_apiKey);
    Q_EMIT modelsChanged();
    Q_EMIT modelChanged(m_model);
    Q_EMIT configuredChanged();
    Q_EMIT selectedModelAvailableChanged();
    Q_EMIT canSendChanged();
  }
}


QString AppModel::model() const {
  return m_model;
}


void AppModel::setModel(const QString& model) {
  if (not model.isEmpty() and not m_models.contains(model)) {
    Q_EMIT errorOccurred("Select a model returned by the configured API");
    return;
  }
  if (m_model != model) {
    m_model = model;
    Q_EMIT modelChanged(m_model);
    Q_EMIT selectedModelAvailableChanged();
    Q_EMIT canSendChanged();
    saveConfig();
  }
}


QStringList AppModel::models() const {
  return m_models;
}


QString AppModel::systemPrompt() const {
  return m_systemPrompt;
}


void AppModel::setSystemPrompt(const QString& systemPrompt) {
  if (m_systemPrompt != systemPrompt) {
    m_systemPrompt = systemPrompt;
    Q_EMIT systemPromptChanged(m_systemPrompt);
    saveConfig();
  }
}


ChatMessageListModel* AppModel::messages() const {
  return m_messages;
}


bool AppModel::busy() const {
  return m_busy;
}


bool AppModel::configured() const {
  const QUrl url(m_baseUrl);
  return url.isValid() and not url.scheme().isEmpty() and not url.host().isEmpty() and
         not m_apiKey.trimmed().isEmpty();
}


bool AppModel::modelsLoading() const {
  return m_modelsLoading;
}


bool AppModel::selectedModelAvailable() const {
  return not m_model.isEmpty() and m_models.contains(m_model);
}


bool AppModel::canSend() const {
  return configured() and selectedModelAvailable() and not m_busy and not m_modelsLoading;
}


bool AppModel::canClear() const {
  return not m_busy;
}


bool AppModel::exporting() const {
  return m_exporting;
}


QString AppModel::exportError() const {
  return m_exportError;
}


bool AppModel::exportSuccess() const {
  return m_exportSuccess;
}


QString AppModel::exportPath() const {
  return m_exportPath;
}


QString AppModel::exportMessage() const {
  return m_exportMessage;
}


QString AppModel::renderedDocument() const {
  return m_renderedDocument;
}


bool AppModel::settingsGuidanceShown() const {
  return m_settingsGuidanceShown;
}


SessionListModel* AppModel::history() const {
  return m_sessionListModel;
}


void AppModel::saveCurrentSession() {
  if (m_busy)
    return;
  if (m_messages->rowCount() == 0)
    return;

  Session session;
  session.id = m_currentSessionId;
  for (int i = 0; i < m_messages->rowCount(); ++i) {
    ChatMessage* msg = m_messages->get(i);
    if (msg->role() != ChatMessage::User and msg->role() != ChatMessage::Agent)
      continue;
    session.messages.append(new ChatMessage(msg->role(), msg->content()));
  }

  if (m_sessionStore->saveSession(session)) {
    m_currentSessionId = session.id;
    m_sessionListModel->refresh();
  } else if (not m_sessionStore->lastError().isEmpty()) {
    Q_EMIT errorOccurred(m_sessionStore->lastError());
  }
  SessionStore::clearSession(session);
}


void AppModel::loadSession(int index) {
  if (m_busy) {
    Q_EMIT errorOccurred("Cannot load a session while a response is streaming");
    return;
  }
  const QString selectedId = m_sessionListModel->sessionId(index);
  QList<ChatMessage*> newMessages = m_sessionListModel->messagesById(selectedId);
  if (newMessages.isEmpty())
    return;
  saveCurrentSession();

  m_messages->clear();
  for (int i = 0; i < newMessages.size(); ++i) {
    m_messages->add(newMessages.at(i));
  }
  m_currentSessionId = selectedId;
  updateRenderedDocument();
}


void AppModel::newChat() {
  if (m_busy) {
    Q_EMIT errorOccurred("Cannot start a new chat while a response is streaming");
    return;
  }
  saveCurrentSession();
  clearChat();
  m_currentSessionId = "";
}


void AppModel::exportToPdf() {
  if (m_busy) {
    setExportFailure(tr("Cannot export while a response is streaming"));
    return;
  }
  if (m_exporting) {
    setExportFailure(tr("A PDF export is already in progress"));
    return;
  }

  m_exporting = true;
  m_exportError.clear();
  m_exportSuccess = false;
  m_exportPath.clear();
  m_exportMessage.clear();
  Q_EMIT exportingChanged();
  Q_EMIT exportErrorChanged();
  Q_EMIT exportSuccessChanged();
  Q_EMIT exportPathChanged();
  Q_EMIT exportMessageChanged();

  QList<ChatMessage*> msgs;
  for (int i = 0; i < m_messages->rowCount(); ++i) {
    msgs.append(m_messages->get(i));
  }
  const PdfExporter::Result result = PdfExporter::exportMessages(msgs);
  if (not result.success) {
    setExportFailure(result.error);
    return;
  }
  m_exportSuccess = true;
  m_exportPath = result.path;
  m_exportMessage = tr("PDF exported to %1").arg(result.path);
  m_exporting = false;
  Q_EMIT exportingChanged();
  Q_EMIT exportSuccessChanged();
  Q_EMIT exportPathChanged();
  Q_EMIT exportMessageChanged();
}


void AppModel::setExportFailure(const QString& error) {
  m_exporting = false;
  m_exportSuccess = false;
  m_exportError = error;
  m_exportPath.clear();
  m_exportMessage.clear();
  Q_EMIT exportingChanged();
  Q_EMIT exportSuccessChanged();
  Q_EMIT exportErrorChanged();
  Q_EMIT exportPathChanged();
  Q_EMIT exportMessageChanged();
  Q_EMIT errorOccurred(error);
}


void AppModel::markSettingsGuidanceShown() {
  if (m_settingsGuidanceShown)
    return;
  m_settingsGuidanceShown = true;
  m_settings.setValue("settingsGuidanceShown", true);
}


void AppModel::addUserMessage(const QString& content) {
  if (content.trimmed().isEmpty())
    return;
  if (not canSend()) {
    Q_EMIT errorOccurred("Configure the API and select a discovered model before sending");
    return;
  }

  m_messages->add(new ChatMessage(ChatMessage::User, content));

  QList<open_ai_api::Message> apiMessages;
  if (not m_systemPrompt.isEmpty()) {
    open_ai_api::Message sysMsg;
    sysMsg.role = "system";
    sysMsg.content = m_systemPrompt;
    apiMessages.append(sysMsg);
  }

  for (int i = 0; i < m_messages->rowCount(); ++i) {
    ChatMessage* msg = m_messages->get(i);
    open_ai_api::Message apiMsg;
    switch (msg->role()) {
      case ChatMessage::System:
        apiMsg.role = "system";
        break;
      case ChatMessage::User:
        apiMsg.role = "user";
        break;
      case ChatMessage::Agent:
        apiMsg.role = "assistant";
        break;
    }
    apiMsg.content = msg->content();
    apiMessages.append(apiMsg);
  }

  m_busy = true;
  Q_EMIT busyChanged();
  Q_EMIT canSendChanged();
  Q_EMIT canClearChanged();

  // Add empty assistant message for streaming
  m_assistantMessageIndex = m_messages->rowCount();
  m_messages->add(new ChatMessage(ChatMessage::Agent, ""));
  m_pendingStreamingContent.clear();

  m_api->streamingChat(apiMessages, m_model);
  m_streamingUpdateTimer.start();
  updateRenderedDocument();
}


void AppModel::clearChat() {
  if (m_busy) {
    Q_EMIT errorOccurred("Cannot clear the chat while a response is streaming");
    return;
  }
  m_messages->clear();
  m_assistantMessageIndex = -1;
  m_pendingStreamingContent.clear();
  updateRenderedDocument();
}


void AppModel::applyConfig(const QString& url, const QString& apiKey,
                           const QString& systemPrompt) {
  setBaseUrl(url);
  setApiKey(apiKey);
  setSystemPrompt(systemPrompt);
  saveConfig();
}


void AppModel::fetchModels() {
  if (not configured()) {
    Q_EMIT errorOccurred("Configure a valid API URL and key before loading models");
    return;
  }
  if (m_busy or m_modelsLoading) {
    Q_EMIT errorOccurred("Another API request is already in progress");
    return;
  }
  m_modelsLoading = true;
  Q_EMIT modelsLoadingChanged();
  Q_EMIT canSendChanged();
  m_api->getModels();
}


void AppModel::loadConfig() {
  m_baseUrl = m_settings.value("baseUrl", "https://api.openai.com/v1").toString();
  m_apiKey = m_settings.value("apiKey", "").toString();
  m_model = m_settings.value("model", "").toString();
  m_systemPrompt = m_settings.value("systemPrompt", "").toString();
  m_models = m_settings.value("models", QStringList()).toStringList();
  m_settingsGuidanceShown = m_settings.value("settingsGuidanceShown", false).toBool();
  if (not m_models.contains(m_model))
    m_model.clear();

  m_api->setBaseUrl(QUrl(m_baseUrl));
  m_api->setApiKey(m_apiKey);
}


void AppModel::saveConfig() const {
  m_settings.setValue("baseUrl", m_baseUrl);
  m_settings.setValue("apiKey", m_apiKey);
  m_settings.setValue("model", m_model);
  m_settings.setValue("systemPrompt", m_systemPrompt);
  m_settings.setValue("models", m_models);
  m_settings.sync();
}


void AppModel::onGetModelsFinished(const QList<open_ai_api::Model>& models,
                                   const open_ai_api::Error& error) {
  m_modelsLoading = false;
  Q_EMIT modelsLoadingChanged();
  Q_EMIT canSendChanged();
  if (error.error_code != open_ai_api::Error::none) {
    Q_EMIT errorOccurred(error.description);
    return;
  }

  m_models.clear();
  for (int i = 0; i < models.size(); ++i) {
    m_models.append(models.at(i).id);
  }
  if (not m_models.contains(m_model)) {
    m_model.clear();
    Q_EMIT modelChanged(m_model);
  }
  Q_EMIT modelsChanged();
  Q_EMIT selectedModelAvailableChanged();
  Q_EMIT canSendChanged();
  Q_EMIT modelsLoaded(m_models);
  saveConfig();
}


void AppModel::onStreamingChatReply(const QString& reply) {
  m_pendingStreamingContent.append(reply);
}


void AppModel::onStreamingChatFinished(const QString& reply, const open_ai_api::Error& error) {
  m_streamingUpdateTimer.stop();
  m_pendingStreamingContent.clear();
  if (m_assistantMessageIndex >= 0)
    m_messages->setContent(m_assistantMessageIndex, reply);

  updateRenderedDocument();

  m_busy = false;
  Q_EMIT busyChanged();
  Q_EMIT canSendChanged();
  Q_EMIT canClearChanged();

  if (error.error_code != open_ai_api::Error::none) {
    Q_EMIT errorOccurred(error.description);
  }
}


void AppModel::onStreamingUpdateTimeout() {
  if (m_pendingStreamingContent.isEmpty())
    return;

  if (m_assistantMessageIndex != -1) {
    ChatMessage* msg = m_messages->get(m_assistantMessageIndex);
    if (msg)
      m_messages->setContent(m_assistantMessageIndex,
                             msg->content() + m_pendingStreamingContent);
  }
  m_pendingStreamingContent.clear();
  updateRenderedDocument();
}


void AppModel::updateRenderedDocument() {
  QList<ChatMessage*> msgs;
  for (int i = 0; i < m_messages->rowCount(); ++i) {
    msgs.append(m_messages->get(i));
  }
  m_renderedDocument = HtmlRenderer::render(msgs);
  Q_EMIT renderedDocumentChanged();
}
