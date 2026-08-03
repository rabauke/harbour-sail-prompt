#include "chat_message.hpp"


ChatMessage::ChatMessage(QObject* parent) : QObject(parent), m_role(User) {
}


ChatMessage::ChatMessage(Role role, const QString& content, QObject* parent)
    : QObject(parent), m_role(role), m_content(content) {
}


ChatMessage::Role ChatMessage::role() const {
  return m_role;
}


QString ChatMessage::content() const {
  return m_content;
}


void ChatMessage::setRole(Role role) {
  if (m_role != role) {
    m_role = role;
    Q_EMIT roleChanged(m_role);
  }
}


void ChatMessage::setContent(const QString& content) {
  if (m_content != content) {
    m_content = content;
    Q_EMIT contentChanged(m_content);
  }
}
