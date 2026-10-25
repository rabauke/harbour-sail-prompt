#pragma once

#include <QAbstractListModel>
#include <QList>
#include "chat_message.hpp"


/// Exposes conversation messages as a Qt list model.
class ChatMessageListModel : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(int count READ count NOTIFY countChanged)
public:
  /// Roles provided by each message row.
  enum MyItemRoles { RoleRole = Qt::UserRole + 1, ContentRole = Qt::UserRole + 2 };

  /// Creates an empty message model.
  /// \param parent QObject that owns this model, or `nullptr`.
  explicit ChatMessageListModel(QObject* parent = nullptr);

  /// Deletes all messages owned by this model.
  ~ChatMessageListModel();

  /// Returns the number of top-level message rows.
  /// \param parent Parent index; valid parents have no child rows.
  int rowCount(const QModelIndex& parent = QModelIndex{}) const override;

  /// Returns the number of messages in the model.
  int count() const;

  /// Returns the data for a message row and role, or an invalid QVariant for an invalid row or role.
  /// \param index Index of the requested row.
  /// \param role Data role to retrieve.
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

  /// Returns the names of the model's custom roles.
  QHash<int, QByteArray> roleNames() const override;

  /// Adds a message to the model and makes the model its QObject parent.
  /// \param message Message to append; ownership is transferred to the model.
  Q_INVOKABLE void add(ChatMessage* message);

  /// Removes and schedules deletion of the message at the given row.
  /// \param index Row to remove; out-of-range indices are ignored.
  Q_INVOKABLE void remove(int index);

  /// Removes and deletes every message in the model.
  Q_INVOKABLE void clear();

  /// Replaces the content of a message at the given row.
  /// \param index Row to update; out-of-range indices are ignored.
  /// \param content Replacement text.
  Q_INVOKABLE void setContent(int index, const QString& content);

  /// Returns the message at a row, or `nullptr` when the index is out of range.
  /// \param index Row to retrieve.
  Q_INVOKABLE ChatMessage* get(int index) const;

  /// Emitted whenever the number of messages changes.
  Q_SIGNAL void countChanged();

private:
  QList<ChatMessage*> m_messages;
};
