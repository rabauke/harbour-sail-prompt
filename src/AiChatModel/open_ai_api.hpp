#ifndef OPEN_AI_API_HPP
#define OPEN_AI_API_HPP

#include <QDateTime>
#include <QMetaType>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>
#include <QUrl>
#include <QList>
#include <QObject>
#include <QJsonObject>
#include <QTimer>

namespace open_ai_api {

  struct Error {
    Q_GADGET
  public:
    enum ErrorKind {
      none,
      network_error,
      invalid_api_key,
      invalid_model,
      invalid_response,
      timeout_error,
      request_in_progress,
      cancelled,
    };

    Q_ENUM(ErrorKind)
    ErrorKind error_code;
    QString description;

    Error() : error_code(none) {}
    Error(ErrorKind code, const QString& desc) : error_code(code), description(desc) {}
  };

  struct Model {
    Q_GADGET
  public:
    QString id;
    QDateTime created;
    QString owned_by;
  };

  struct Message {
    QString role;
    QString content;
  };

  QJsonObject createChatPayload(const QList<Message> &messages, const QString &model);

  class SseParser {
  public:
    SseParser();
    Error append(const QByteArray &data, bool final, QStringList *chunks);
    bool done() const;

  private:
    Error processEvent(const QList<QByteArray> &dataLines, QStringList *chunks);

    QByteArray m_buffer;
    QList<QByteArray> m_dataLines;
    bool m_done;
  };

  class OpenAiApi : public QObject {
    Q_OBJECT

  public:
    explicit OpenAiApi(QObject *parent = nullptr);
    explicit OpenAiApi(const QUrl &baseUrl, const QString &apiKey, QObject *parent = nullptr);
    ~OpenAiApi();

    void setBaseUrl(const QUrl &url);
    void setApiKey(const QString &key);

    void getModels();
    void streamingChat(const QList<Message> &messages, const QString &model);
    void cancel();
    bool active() const;

    signals:
    void getModelsFinished(const QList<open_ai_api::Model> &models, const open_ai_api::Error &error);
    void streamingChatReply(const QString &reply);
    void streamingChatFinished(const QString &reply, const open_ai_api::Error &error);

  private:
    enum RequestKind { NoRequest, ModelsRequest, ChatRequest };

    bool beginRequest(RequestKind kind);
    void finishModels(const QList<Model> &models, const Error &error);
    void finishChat(const Error &error);
    void resetTimeout();
    void cleanupRequest();

    QUrl m_base_url;
    QString m_api_key;
    QNetworkAccessManager *m_network_access_manager;
    QNetworkReply *m_reply;
    QTimer m_timeout;
    RequestKind m_request_kind;
    bool m_timed_out;
    bool m_cancelled;
    bool m_finishing;
    QString m_full_reply;
    SseParser m_sse_parser;
  };

}  // namespace open_ai_api

Q_DECLARE_METATYPE(open_ai_api::Error)
Q_DECLARE_METATYPE(open_ai_api::Model)
Q_DECLARE_METATYPE(QList<open_ai_api::Model>)

#endif // OPEN_AI_API_HPP
