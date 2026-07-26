#include "session_store.hpp"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QRegExp>
#include <QUuid>
#include <QDebug>
#include <algorithm>


SessionStore::SessionStore(QObject* parent) : QObject(parent) {
  m_storagePath =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/sessions";
  QDir dir;
  if (!dir.exists(m_storagePath)) {
    dir.mkpath(m_storagePath);
  }
}


SessionStore::~SessionStore() {
}


QString SessionStore::storagePath() const {
  return m_storagePath;
}


QString SessionStore::lastError() const {
  return m_lastError;
}


void SessionStore::setStoragePath(const QString& path) {
  m_storagePath = path;
  QDir dir;
  if (!dir.exists(m_storagePath)) {
    dir.mkpath(m_storagePath);
  }
}


QString SessionStore::getFilePath(const QString& id) const {
  if (!isValidId(id))
    return QString();
  return m_storagePath + "/" + id + ".json";
}


bool SessionStore::isValidId(const QString& id) {
  static const QRegExp uuidPattern(
      "^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$");
  return uuidPattern.exactMatch(id) && !QUuid(id).isNull();
}


QList<Session> SessionStore::loadAllSessions() {
  QList<Session> sessions;
  QDir dir(m_storagePath);
  QStringList filters;
  filters << "*.json";
  QStringList files = dir.entryList(filters, QDir::Files);

  for (int fileIndex = 0; fileIndex < files.size(); ++fileIndex) {
    const QString fileName = files.at(fileIndex);
    const QString fileId = fileName.left(fileName.length() - 5);
    if (!isValidId(fileId))
      continue;
    QFile file(dir.absoluteFilePath(fileName));
    if (!file.open(QIODevice::ReadOnly))
      continue;

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject())
      continue;

    QJsonObject obj = doc.object();
    if (!obj.contains("id") || !obj.value("id").isString() ||
        obj.value("id").toString() != fileId || !obj.contains("title") ||
        !obj.value("title").isString() || !obj.contains("timestamp") ||
        !obj.value("timestamp").isString() || !obj.contains("messages") ||
        !obj.value("messages").isArray())
      continue;
    Session session;
    session.id = fileId;
    session.title = obj.value("title").toString();
    session.timestamp = QDateTime::fromString(obj.value("timestamp").toString(), Qt::ISODate);
    if (!session.timestamp.isValid())
      continue;

    bool valid = true;
    bool hasUserMessage = false;
    QJsonArray msgArray = obj.value("messages").toArray();
    for (int i = 0; i < msgArray.size(); ++i) {
      if (!msgArray.at(i).isObject()) {
        valid = false;
        break;
      }
      QJsonObject msgObj = msgArray.at(i).toObject();
      if (!msgObj.contains("role") || !msgObj.value("role").isDouble() ||
          !msgObj.contains("content") || !msgObj.value("content").isString()) {
        valid = false;
        break;
      }
      int roleValue = msgObj.value("role").toInt();
      if (msgObj.value("role").toDouble() != roleValue ||
          (roleValue != ChatMessage::User && roleValue != ChatMessage::Agent)) {
        valid = false;
        break;
      }
      ChatMessage::Role role = static_cast<ChatMessage::Role>(roleValue);
      QString content = msgObj.value("content").toString();
      session.messages.append(new ChatMessage(role, content));
      if (role == ChatMessage::User)
        hasUserMessage = true;
    }
    if (!valid || !hasUserMessage) {
      clearSession(session);
      continue;
    }
    sessions.append(session);
  }

  // Sort newest first
  std::sort(sessions.begin(), sessions.end(),
            [](const Session& a, const Session& b) { return a.timestamp > b.timestamp; });

  return sessions;
}


bool SessionStore::saveSession(Session& session) {
  m_lastError.clear();
  if (session.messages.isEmpty())
    return false;

  // Check if there's at least one user message
  bool hasUserMessage = false;
  for (int i = 0; i < session.messages.size(); ++i) {
    ChatMessage* msg = session.messages.at(i);
    if (msg->role() == ChatMessage::User) {
      hasUserMessage = true;
      break;
    }
  }
  if (!hasUserMessage)
    return false;

  if (session.id.isEmpty()) {
    session.id = QUuid::createUuid().toString().mid(1, 36);
  }
  if (!isValidId(session.id)) {
    m_lastError = "Invalid session identifier";
    return false;
  }
  session.timestamp = QDateTime::currentDateTime();

  if (session.title.isEmpty()) {
    for (int i = 0; i < session.messages.size(); ++i) {
      ChatMessage* msg = session.messages.at(i);
      if (msg->role() == ChatMessage::User) {
        QString normalized = msg->content().simplified();
        session.title = normalized.left(50);
        if (normalized.length() > 50)
          session.title += "...";
        break;
      }
    }
  }

  QJsonObject obj;
  obj["id"] = session.id;
  obj["title"] = session.title;
  obj["timestamp"] = session.timestamp.toString(Qt::ISODate);

  QJsonArray msgArray;
  for (int i = 0; i < session.messages.size(); ++i) {
    ChatMessage* msg = session.messages.at(i);
    if (msg->role() != ChatMessage::User && msg->role() != ChatMessage::Agent)
      continue;
    QJsonObject msgObj;
    msgObj["role"] = static_cast<int>(msg->role());
    msgObj["content"] = msg->content();
    msgArray.append(msgObj);
  }
  obj["messages"] = msgArray;

  QDir dir;
  if ((!dir.exists(m_storagePath) && !dir.mkpath(m_storagePath))) {
    m_lastError = "Could not create the session history directory";
    return false;
  }
  QSaveFile file(getFilePath(session.id));
  if (!file.open(QIODevice::WriteOnly)) {
    m_lastError = file.errorString();
    return false;
  }

  QByteArray json = QJsonDocument(obj).toJson();
  if (file.write(json) != json.size() || !file.commit()) {
    m_lastError = file.errorString();
    return false;
  }
  return true;
}


bool SessionStore::deleteSession(const QString& id) {
  m_lastError.clear();
  QString path = getFilePath(id);
  if (path.isEmpty()) {
    m_lastError = "Invalid session identifier";
    return false;
  }
  QFile file(path);
  if (file.exists()) {
    if (file.remove())
      return true;
    m_lastError = file.errorString();
    return false;
  }
  m_lastError = "Session does not exist";
  return false;
}


Session SessionStore::createSession(const QList<ChatMessage*>& messages) {
  Session session;
  for (int i = 0; i < messages.size(); ++i) {
    ChatMessage* msg = messages.at(i);
    if (msg->role() != ChatMessage::User && msg->role() != ChatMessage::Agent)
      continue;
    session.messages.append(new ChatMessage(msg->role(), msg->content()));
  }
  return session;
}


void SessionStore::clearSession(Session& session) {
  qDeleteAll(session.messages);
  session.messages.clear();
}
