#ifndef CHAT_MESSAGE_HPP
#define CHAT_MESSAGE_HPP

#include <QObject>
#include <QString>

class ChatMessage : public QObject {
  Q_OBJECT

public:
  enum Role { System = 1, User = 2, Agent = 3 };
  Q_ENUM(Role)

  explicit ChatMessage(QObject *parent = nullptr);
  explicit ChatMessage(Role role, const QString& content, QObject* parent = nullptr);

  Q_PROPERTY(Role role READ role WRITE setRole NOTIFY roleChanged)
  Q_PROPERTY(QString content READ content WRITE setContent NOTIFY contentChanged)

  Role role() const;
  QString content() const;

  void setRole(Role role);
  void setContent(const QString& content);

  Q_SIGNAL void roleChanged(Role role);
  Q_SIGNAL void contentChanged(const QString& content);

private:
  Role m_role;
  QString m_content;
};

Q_DECLARE_METATYPE(ChatMessage::Role)

#endif // CHAT_MESSAGE_HPP
