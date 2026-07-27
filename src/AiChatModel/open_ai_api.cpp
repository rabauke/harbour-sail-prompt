#include "open_ai_api.hpp"
#include <algorithm>
#include <QList>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>


namespace {

  QString apiErrorMessage(const QJsonObject &object) {
    const QJsonValue errorValue = object.value("error");
    if (errorValue.isString())
      return errorValue.toString();
    if (errorValue.isObject()) {
      const QJsonObject error = errorValue.toObject();
      if (error.value("message").isString())
        return error.value("message").toString();
    }
    return QString();
  }


  open_ai_api::Error httpError(QNetworkReply *reply, const QByteArray &body) {
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
    const QString apiMessage =
        document.isObject() ? apiErrorMessage(document.object()) : QString();
    QString description = apiMessage;
    if (description.isEmpty())
      description = QString("HTTP %1").arg(status);
    else
      description = QString("HTTP %1: %2").arg(status).arg(description);
    if (status == 401 || status == 403)
      return open_ai_api::Error(open_ai_api::Error::invalid_api_key, description);
    if (status == 404 || apiMessage.contains("model", Qt::CaseInsensitive))
      return open_ai_api::Error(open_ai_api::Error::invalid_model, description);
    return open_ai_api::Error(open_ai_api::Error::network_error, description);
  }

}  // namespace


namespace open_ai_api {

  QJsonObject createChatPayload(const QList<Message> &messages, const QString &model) {
    QJsonObject payload;
    payload["model"] = model;
    payload["stream"] = true;
    QJsonArray array;
    for (int i = 0; i < messages.size(); ++i) {
      QJsonObject item;
      item["role"] = messages.at(i).role;
      item["content"] = messages.at(i).content;
      array.append(item);
    }
    payload["messages"] = array;
    return payload;
  }


  SseParser::SseParser() : m_done(false) {
  }


  bool SseParser::done() const {
    return m_done;
  }


  Error SseParser::processEvent(const QList<QByteArray> &dataLines, QStringList *chunks) {
    if (dataLines.isEmpty())
      return Error();
    const QByteArray data = dataLines.join("\n");
    if (data.trimmed() == "[DONE]") {
      m_done = true;
      return Error();
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
      return Error(Error::invalid_response,
                   QString("Malformed streaming response: %1").arg(parseError.errorString()));
    const QJsonObject response = document.object();
    const QString apiMessage = apiErrorMessage(response);
    if (!apiMessage.isEmpty())
      return Error(apiMessage.contains("model", Qt::CaseInsensitive) ? Error::invalid_model
                                                                     : Error::invalid_response,
                   apiMessage);
    const QJsonValue choicesValue = response.value("choices");
    if (!choicesValue.isArray() || choicesValue.toArray().isEmpty() ||
        !choicesValue.toArray().first().isObject())
      return Error(Error::invalid_response, "Streaming response has no valid choices array");
    const QJsonObject choice = choicesValue.toArray().first().toObject();
    if (!choice.value("delta").isObject())
      return Error(Error::invalid_response, "Streaming response has no valid delta object");
    const QJsonValue content = choice.value("delta").toObject().value("content");
    if (!content.isUndefined() && !content.isString())
      return Error(Error::invalid_response, "Streaming response content is not a string");
    if (content.isString() && !content.toString().isEmpty())
      chunks->append(content.toString());
    return Error();
  }


  Error SseParser::append(const QByteArray &data, bool final, QStringList *chunks) {
    if (m_done)
      return Error();
    m_buffer.append(data);
    while (true) {
      const int newline = m_buffer.indexOf('\n');
      if (newline < 0)
        break;
      QByteArray line = m_buffer.left(newline);
      m_buffer.remove(0, newline + 1);
      if (line.endsWith('\r'))
        line.chop(1);
      if (line.isEmpty()) {
        const Error error = processEvent(m_dataLines, chunks);
        m_dataLines.clear();
        if (error.error_code != Error::none || m_done)
          return error;
      } else if (line.startsWith("data:")) {
        QByteArray value = line.mid(5);
        if (value.startsWith(' '))
          value.remove(0, 1);
        m_dataLines.append(value);
      }
    }
    if (final) {
      if (!m_buffer.isEmpty()) {
        QByteArray line = m_buffer;
        if (line.endsWith('\r'))
          line.chop(1);
        if (line.startsWith("data:")) {
          line.remove(0, 5);
          if (line.startsWith(' '))
            line.remove(0, 1);
          m_dataLines.append(line);
        }
        m_buffer.clear();
      }
      return processEvent(m_dataLines, chunks);
    }
    return Error();
  }


  OpenAiApi::OpenAiApi(QObject *parent)
      : QObject(parent),
        m_network_access_manager(new QNetworkAccessManager(this)),
        m_reply(0),
        m_request_kind(NoRequest),
        m_timed_out(false),
        m_cancelled(false),
        m_finishing(false) {
    m_timeout.setSingleShot(true);
    connect(&m_timeout, &QTimer::timeout, this, [this]() {
      m_timed_out = true;
      if (m_reply)
        m_reply->abort();
    });
  }


  OpenAiApi::OpenAiApi(const QUrl &baseUrl, const QString &apiKey, QObject *parent)
      : QObject(parent),
        m_base_url(baseUrl),
        m_api_key(apiKey),
        m_network_access_manager(new QNetworkAccessManager(this)),
        m_reply(0),
        m_request_kind(NoRequest),
        m_timed_out(false),
        m_cancelled(false),
        m_finishing(false) {
    m_timeout.setSingleShot(true);
    connect(&m_timeout, &QTimer::timeout, this, [this]() {
      m_timed_out = true;
      if (m_reply)
        m_reply->abort();
    });
  }


  OpenAiApi::~OpenAiApi() {
    cancel();
  }


  void OpenAiApi::setBaseUrl(const QUrl &url) {
    m_base_url = url;
  }


  void OpenAiApi::setApiKey(const QString &key) {
    m_api_key = key;
  }


  bool OpenAiApi::active() const {
    return m_reply != 0;
  }


  bool OpenAiApi::beginRequest(RequestKind kind) {
    if (active()) {
      const Error error(Error::request_in_progress,
                        "Another API request is already in progress");
      if (kind == ModelsRequest)
        emit getModelsFinished(QList<Model>(), error);
      else
        emit streamingChatFinished(QString(), error);
      return false;
    }
    m_request_kind = kind;
    m_timed_out = false;
    m_cancelled = false;
    m_finishing = false;
    return true;
  }


  void OpenAiApi::resetTimeout() {
    m_timeout.start(m_request_kind == ModelsRequest ? 30000 : 300000);
  }


  void OpenAiApi::cleanupRequest() {
    m_timeout.stop();
    if (m_reply) {
      m_reply->deleteLater();
      m_reply = 0;
    }
    m_request_kind = NoRequest;
  }


  void OpenAiApi::cancel() {
    if (!m_reply)
      return;
    m_cancelled = true;
    m_reply->abort();
  }


  void OpenAiApi::finishModels(const QList<Model> &models, const Error &error) {
    if (m_finishing)
      return;
    m_finishing = true;
    cleanupRequest();
    emit getModelsFinished(models, error);
  }


  void OpenAiApi::finishChat(const Error &error) {
    if (m_finishing)
      return;
    m_finishing = true;
    const QString reply = m_full_reply;
    cleanupRequest();
    emit streamingChatFinished(reply, error);
  }


  void OpenAiApi::getModels() {
    if (!beginRequest(ModelsRequest))
      return;
    QString urlStr = m_base_url.toString();
    if (!urlStr.endsWith("/"))
      urlStr.append("/");
    urlStr.append("models");

    QNetworkRequest request((QUrl(urlStr)));
    request.setRawHeader("Authorization", ("Bearer " + m_api_key).toUtf8());

#if QT_VERSION >= QT_VERSION_CHECK(5, 8, 0)
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
#endif

    m_reply = m_network_access_manager->get(request);
    resetTimeout();

    connect(m_reply, &QNetworkReply::finished, this, [this]() {
      QNetworkReply *reply = m_reply;

      QList<Model> model_list;

      const QByteArray data = reply->readAll();
      const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
      if (m_timed_out) {
        finishModels(model_list, Error(Error::timeout_error, "Model request timed out"));
        return;
      }
      if (m_cancelled) {
        finishModels(model_list, Error(Error::cancelled, "Model request was cancelled"));
        return;
      }
      if (status >= 400) {
        finishModels(model_list, httpError(reply, data));
        return;
      }
      if (reply->error() != QNetworkReply::NoError) {
        finishModels(model_list, Error(Error::network_error, reply->errorString()));
        return;
      }
      QJsonParseError parseError;
      QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
      if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        finishModels(model_list, Error(Error::invalid_response,
                                       "Models response is not valid JSON object"));
        return;
      }

      QJsonObject response = doc.object();
      if (!response.value("data").isArray()) {
        finishModels(model_list,
                     Error(Error::invalid_response, "Models response has no data array"));
        return;
      }
      QJsonArray modelsArray = response["data"].toArray();
      for (int i = 0; i < modelsArray.size(); ++i) {
        QJsonValue value = modelsArray.at(i);
        if (!value.isObject()) {
          finishModels(QList<Model>(), Error(Error::invalid_response,
                                             "Models response contains a malformed entry"));
          return;
        }
        QJsonObject model_obj = value.toObject();
        Model model;
        if (!model_obj.value("id").isString() ||
            model_obj.value("id").toString().trimmed().isEmpty()) {
          finishModels(QList<Model>(),
                       Error(Error::invalid_response,
                             "Models response contains an entry without an id"));
          return;
        }
        model.id = model_obj["id"].toString();
        if (model_obj.contains("created") && model_obj["created"].isDouble())
          model.created = QDateTime::fromTime_t((uint)model_obj["created"].toDouble());
        if (model_obj.contains("owned_by") && model_obj["owned_by"].isString())
          model.owned_by = model_obj["owned_by"].toString();
        model_list.append(model);
      }

      std::sort(model_list.begin(), model_list.end(), [](const Model &a, const Model &b) {
        return QString::compare(a.id, b.id, Qt::CaseInsensitive) < 0;
      });

      auto iter = std::unique(model_list.begin(), model_list.end(),
                              [](const Model &a, const Model &b) { return a.id == b.id; });
      model_list.erase(iter, model_list.end());

      finishModels(model_list, Error());
    });
  }


  void OpenAiApi::streamingChat(const QList<Message> &messages, const QString &model) {
    if (!beginRequest(ChatRequest))
      return;
    const QJsonObject payload = createChatPayload(messages, model);

    QString urlStr = m_base_url.toString();
    if (!urlStr.endsWith("/"))
      urlStr.append("/");
    urlStr.append("chat/completions");

    QNetworkRequest request((QUrl(urlStr)));
    request.setRawHeader("Authorization", ("Bearer " + m_api_key).toUtf8());
    request.setRawHeader("Content-Type", "application/json");

#if QT_VERSION >= QT_VERSION_CHECK(5, 8, 0)
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
#endif

    m_full_reply.clear();
    m_sse_parser = SseParser();
    m_reply = m_network_access_manager->post(
        request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    resetTimeout();

    connect(m_reply, &QNetworkReply::readyRead, this, [this]() {
      const QByteArray data = m_reply->readAll();
      if (!data.isEmpty())
        resetTimeout();
      QStringList chunks;
      const Error error = m_sse_parser.append(data, false, &chunks);
      for (int i = 0; i < chunks.size(); ++i) {
        m_full_reply.append(chunks.at(i));
        emit streamingChatReply(chunks.at(i));
      }
      if (error.error_code != Error::none) {
        m_cancelled = false;
        m_reply->abort();
        finishChat(error);
      }
    });

    connect(m_reply, &QNetworkReply::finished, this, [this]() {
      if (!m_reply || m_finishing)
        return;
      QNetworkReply *reply = m_reply;
      const QByteArray remaining = reply->readAll();
      const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
      if (m_timed_out) {
        finishChat(Error(Error::timeout_error, "Chat request timed out due to inactivity"));
        return;
      }
      if (m_cancelled) {
        finishChat(Error(Error::cancelled, "Chat request was cancelled"));
        return;
      }
      if (status >= 400) {
        finishChat(httpError(reply, remaining));
        return;
      }
      if (reply->error() != QNetworkReply::NoError) {
        finishChat(Error(Error::network_error, reply->errorString()));
        return;
      }
      QStringList chunks;
      const Error error = m_sse_parser.append(remaining, true, &chunks);
      for (int i = 0; i < chunks.size(); ++i) {
        m_full_reply.append(chunks.at(i));
        emit streamingChatReply(chunks.at(i));
      }
      finishChat(error);
    });
  }

}  // namespace open_ai_api
