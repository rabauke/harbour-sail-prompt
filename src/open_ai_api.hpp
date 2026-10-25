#pragma once

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

  /// Describes the category and details of an API or request failure.
  struct Error {
    Q_GADGET
  public:
    /// Classifies errors reported by API operations.
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
    /// Error category, defaulting to none.
    ErrorKind error_code;
    /// Human-readable error description.
    QString description;

    /// Creates an error with no failure.
    Error() : error_code{none} {
    }

    /// Creates an error with the given category and description.
    /// \param code Error category.
    /// \param desc Human-readable description.
    Error(ErrorKind code, const QString& desc) : error_code{code}, description{desc} {
    }
  };


  /// Describes a model returned by the configured API.
  struct Model {
    Q_GADGET
  public:
    /// Model identifier accepted by chat requests.
    QString id;
    /// Model creation date and time.
    QDateTime created;
    /// Account or organization that owns the model.
    QString owned_by;
  };


  /// Represents one role/content entry in a chat request.
  struct Message {
    /// API role associated with the message.
    QString role;
    /// Text content of the message.
    QString content;
  };


  /// Builds a streaming chat-completion request payload.
  /// \param messages Ordered messages to include in the request.
  /// \param model Identifier of the model to use.
  /// \return JSON object containing the model, messages, and streaming flag.
  QJsonObject createChatPayload(const QList<Message>& messages, const QString& model);


  /// Parses server-sent events from a streamed chat-completion response.
  class SseParser {
  public:
    /// Creates a parser in the not-yet-completed state.
    SseParser();

    /// Appends response data and extracts any complete content chunks.
    /// \param data Newly received response bytes.
    /// \param final Whether this is the final data supplied for the response.
    /// \param chunks Receives extracted content chunks.
    /// \return An error when the stream contains a malformed or failed event.
    Error append(const QByteArray& data, bool final, QStringList* chunks);

    /// Returns whether the stream has delivered its completion marker.
    bool done() const;

  private:
    Error processEvent(const QList<QByteArray>& dataLines, QStringList* chunks);

    QByteArray m_buffer;
    QList<QByteArray> m_dataLines;
    bool m_done;
  };


  /// Sends model-list and streaming chat requests to an OpenAI-compatible API.
  class OpenAiApi : public QObject {
    Q_OBJECT

  public:
    /// Creates an API client with an unset endpoint and key.
    /// \param parent QObject that owns this client, or `nullptr`.
    explicit OpenAiApi(QObject* parent = nullptr);

    /// Creates an API client configured with an endpoint and API key.
    /// \param baseUrl Base URL of the API service.
    /// \param apiKey API key used to authenticate requests.
    /// \param parent QObject that owns this client, or `nullptr`.
    explicit OpenAiApi(const QUrl& baseUrl, const QString& apiKey, QObject* parent = nullptr);

    /// Cancels any active request and releases the client resources.
    ~OpenAiApi();

    /// Sets the base URL used for subsequent requests.
    /// \param url API base URL.
    void setBaseUrl(const QUrl& url);

    /// Sets the credential used to authenticate subsequent requests.
    /// \param key API key.
    void setApiKey(const QString& key);

    /// Requests the available models and emits getModelsFinished() on completion.
    void getModels();

    /// Starts a streaming chat request and emits reply chunks and a completion signal.
    /// \param messages Ordered conversation messages.
    /// \param model Model identifier to use.
    void streamingChat(const QList<Message>& messages, const QString& model);

    /// Cancels the currently active request, if any.
    void cancel();

    /// Returns whether a request is currently active.
    bool active() const;

    /// Emitted when a model-list request completes.
    /// \param models Models returned by the API, if successful.
    /// \param error Request error, or a no-error value on success.
    Q_SIGNAL void getModelsFinished(const QList<open_ai_api::Model>& models,
                                    const open_ai_api::Error& error);

    /// Emitted when a new text chunk arrives from a streaming chat request.
    /// \param reply Newly received text chunk.
    Q_SIGNAL void streamingChatReply(const QString& reply);

    /// Emitted when a streaming chat request completes or fails.
    /// \param reply Full reply received so far.
    /// \param error Request error, or a no-error value on success.
    Q_SIGNAL void streamingChatFinished(const QString& reply, const open_ai_api::Error& error);

  private:
    enum RequestKind { NoRequest, ModelsRequest, ChatRequest };

    bool beginRequest(RequestKind kind);
    void finishModels(const QList<Model>& models, const Error& error);
    void finishChat(const Error& error);
    void resetTimeout();
    void cleanupRequest();

    QUrl m_base_url;
    QString m_api_key;
    QNetworkAccessManager* m_network_access_manager;
    QNetworkReply* m_reply;
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
