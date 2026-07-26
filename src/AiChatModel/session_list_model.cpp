#include "session_list_model.hpp"


SessionListModel::SessionListModel(SessionStore* store, QObject* parent)
    : QAbstractListModel(parent), m_store(store) {
  refresh();
}


SessionListModel::~SessionListModel() {
  clearSessions();
}


int SessionListModel::rowCount(const QModelIndex& parent) const {
  if (parent.isValid())
    return 0;
  return m_sessions.size();
}


QVariant SessionListModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || index.row() >= m_sessions.size())
    return QVariant();

  const Session& session = m_sessions.at(index.row());
  switch (role) {
    case IdRole:
      return session.id;
    case TitleRole:
      return session.title;
    case TimestampRole:
      return session.timestamp;
    case PreviewRole:
      for (int i = 0; i < session.messages.size(); ++i) {
        ChatMessage* msg = session.messages.at(i);
        if (msg->role() == ChatMessage::User) {
          return msg->content().left(100).trimmed();
        }
      }
      return QString();
  }
  return QVariant();
}


QHash<int, QByteArray> SessionListModel::roleNames() const {
  QHash<int, QByteArray> roles;
  roles[IdRole] = "id";
  roles[TitleRole] = "title";
  roles[TimestampRole] = "timestamp";
  roles[PreviewRole] = "preview";
  return roles;
}


void SessionListModel::refresh() {
  beginResetModel();
  clearSessions();
  m_sessions = m_store->loadAllSessions();
  endResetModel();
}


void SessionListModel::deleteSession(int index) {
  if (index < 0 || index >= m_sessions.size())
    return;
  deleteSessionById(m_sessions.at(index).id);
}


void SessionListModel::deleteSessionById(const QString& id) {
  int index = -1;
  for (int i = 0; i < m_sessions.size(); ++i) {
    if (m_sessions.at(i).id == id) {
      index = i;
      break;
    }
  }
  if (index < 0)
    return;
  if (m_store->deleteSession(id)) {
    beginRemoveRows(QModelIndex(), index, index);
    Session session = m_sessions.takeAt(index);
    SessionStore::clearSession(session);
    endRemoveRows();
  } else {
    emit errorOccurred(m_store->lastError());
  }
}


QString SessionListModel::sessionId(int index) const {
  if (index < 0 || index >= m_sessions.size())
    return QString();
  return m_sessions.at(index).id;
}


QList<ChatMessage*> SessionListModel::messages(int index) const {
  QList<ChatMessage*> result;
  if (index < 0 || index >= m_sessions.size())
    return result;

  for (int i = 0; i < m_sessions.at(index).messages.size(); ++i) {
    ChatMessage* msg = m_sessions.at(index).messages.at(i);
    result.append(new ChatMessage(msg->role(), msg->content()));
  }
  return result;
}


QList<ChatMessage*> SessionListModel::messagesById(const QString& id) const {
  for (int i = 0; i < m_sessions.size(); ++i) {
    if (m_sessions.at(i).id == id)
      return messages(i);
  }
  return QList<ChatMessage*>();
}


void SessionListModel::clearSessions() {
  for (int i = 0; i < m_sessions.size(); ++i) {
    SessionStore::clearSession(m_sessions[i]);
  }
  m_sessions.clear();
}
