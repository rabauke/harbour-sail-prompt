#pragma once

#include <QObject>
#include <QString>


/// Stores the role and text content of one conversation message.
class ChatMessage : public QObject {
  Q_OBJECT

public:
  /// Identifies the source or purpose of a message in a conversation.
  enum Role { System = 1, User = 2, Agent = 3 };
  Q_ENUM(Role)

  /// Creates an empty message with the given QObject parent.
  /// \param parent QObject that owns this message, or `nullptr`.
  explicit ChatMessage(QObject* parent = nullptr);

  /// Creates a message with the specified role and content.
  /// \param role The message role.
  /// \param content The message text.
  /// \param parent QObject that owns this message, or `nullptr`.
  explicit ChatMessage(Role role, const QString& content, QObject* parent = nullptr);

  Q_PROPERTY(Role role READ role WRITE setRole NOTIFY roleChanged)
  Q_PROPERTY(QString content READ content WRITE setContent NOTIFY contentChanged)

  /// Returns this message's role.
  Role role() const;

  /// Returns this message's text content.
  QString content() const;

  /// Sets the role and emits roleChanged() if it differs from the current role.
  /// \param role The new message role.
  void setRole(Role role);

  /// Sets the text and emits contentChanged() if it differs from the current content.
  /// \param content The new message text.
  void setContent(const QString& content);

  /// Emitted when the message role changes.
  /// \param role The new role.
  Q_SIGNAL void roleChanged(Role role);

  /// Emitted when the message content changes.
  /// \param content The new message text.
  Q_SIGNAL void contentChanged(const QString& content);

private:
  Role m_role;
  QString m_content;
};


Q_DECLARE_METATYPE(ChatMessage::Role)
