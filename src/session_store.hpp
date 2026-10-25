#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QDateTime>
#include "chat_message.hpp"


/// Holds the identifier, metadata, and messages of one saved conversation.
struct Session {
  /// Unique identifier used to store and retrieve this session.
  QString id;

  /// Human-readable session title.
  QString title;

  /// Date and time when the session was saved.
  QDateTime timestamp;

  /// Messages belonging to the conversation.
  QList<ChatMessage*> messages;
};


/// Persists conversation sessions as JSON files.
class SessionStore : public QObject {
  Q_OBJECT
public:
  /// Creates a store in the application's default session directory.
  /// \param parent QObject that owns this store, or `nullptr`.
  explicit SessionStore(QObject* parent = nullptr);

  /// Destroys the session store.
  virtual ~SessionStore();

  /// Returns the directory used to store session files.
  QString storagePath() const;

  /// Sets the session directory and creates it if needed.
  /// \param path Directory to use for session files.
  void setStoragePath(const QString& path);

  /// Loads all valid sessions from the current storage directory.
  /// \return Sessions read from valid JSON files.
  QList<Session> loadAllSessions();

  /// Saves a session, assigning an identifier when it does not already have one.
  /// \param session Session to persist; its identifier and timestamp may be updated.
  /// \return Whether the session was saved successfully.
  bool saveSession(Session& session);

  /// Deletes a session by its identifier.
  /// \param id Identifier of the session to delete.
  /// \return Whether the session file was deleted successfully.
  bool deleteSession(const QString& id);

  /// Returns the most recent storage error description.
  QString lastError() const;

  /// Creates a new session from a message list, deriving its metadata from the conversation.
  /// \param messages Conversation messages used to create the session.
  /// \return Newly initialized session.
  static Session createSession(const QList<ChatMessage*>& messages);

  /// Deletes the messages owned by a session and clears its message list.
  /// \param session Session whose message objects should be released.
  static void clearSession(Session& session);

  /// Checks whether a string has the expected non-null UUID format for a session identifier.
  /// \param id Identifier to validate.
  /// \return `true` when the identifier is valid.
  static bool isValidId(const QString& id);

private:
  QString m_storagePath;
  QString m_lastError;
  QString getFilePath(const QString& id) const;
};
