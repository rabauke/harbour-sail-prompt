#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QDateTime>
#include "chat_message.hpp"


struct Session {
  QString id;
  QString title;
  QDateTime timestamp;
  QList<ChatMessage*> messages;
};


class SessionStore : public QObject {
  Q_OBJECT
public:
  explicit SessionStore(QObject* parent = nullptr);
  virtual ~SessionStore();

  QString storagePath() const;
  void setStoragePath(const QString& path);

  QList<Session> loadAllSessions();
  bool saveSession(Session& session);
  bool deleteSession(const QString& id);
  QString lastError() const;

  static Session createSession(const QList<ChatMessage*>& messages);
  static void clearSession(Session& session);
  static bool isValidId(const QString& id);

private:
  QString m_storagePath;
  QString m_lastError;
  QString getFilePath(const QString& id) const;
};
