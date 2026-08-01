#include "chat_message_list_model.hpp"
#include <algorithm>


ChatMessageListModel::ChatMessageListModel(QObject *parent) : QAbstractListModel(parent) {
}


ChatMessageListModel::~ChatMessageListModel() {
  qDeleteAll(m_messages);
}


int ChatMessageListModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid()) {
    return 0;
  }
  return m_messages.size();
}


int ChatMessageListModel::count() const {
  return m_messages.size();
}


QVariant ChatMessageListModel::data(const QModelIndex &index, int role) const {
  if (not index.isValid() or index.row() >= m_messages.size()) {
    return QVariant();
  }

  ChatMessage *item = m_messages.at(index.row());
  switch (role) {
    case RoleRole:
      return item->role();
    case ContentRole:
      return item->content();
    default:
      return QVariant();
  }
}


QHash<int, QByteArray> ChatMessageListModel::roleNames() const {
  QHash<int, QByteArray> roles;
  roles[RoleRole] = "role";
  roles[ContentRole] = "content";
  return roles;
}


void ChatMessageListModel::add(ChatMessage *message) {
  beginInsertRows(QModelIndex(), m_messages.size(), m_messages.size());
  message->setParent(this);
  m_messages.append(message);
  endInsertRows();
  emit countChanged();
}


void ChatMessageListModel::remove(int index) {
  if (index >= 0 and index < m_messages.size()) {
    beginRemoveRows(QModelIndex(), index, index);
    ChatMessage *message = m_messages.takeAt(index);
    message->deleteLater();
    endRemoveRows();
    emit countChanged();
  }
}


void ChatMessageListModel::clear() {
  if (m_messages.isEmpty())
    return;
  beginRemoveRows(QModelIndex(), 0, m_messages.size() - 1);
  qDeleteAll(m_messages);
  m_messages.clear();
  endRemoveRows();
  emit countChanged();
}


void ChatMessageListModel::setContent(int index, const QString &content) {
  if (index < 0 or index >= m_messages.size())
    return;
  ChatMessage *msg = m_messages.at(index);
  if (msg->content() == content)
    return;
  msg->setContent(content);
  QModelIndex mi = this->index(index, 0);
  emit dataChanged(mi, mi, QVector<int>() << ContentRole);
}


ChatMessage *ChatMessageListModel::get(int index) const {
  if (index >= 0 and index < m_messages.size())
    return m_messages.at(index);
  return nullptr;
}
