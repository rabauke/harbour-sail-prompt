#pragma once

#include <QAbstractListModel>
#include <QList>
#include "session_store.hpp"


/// Exposes saved conversations as a Qt list model.
class SessionListModel : public QAbstractListModel {
  Q_OBJECT
public:
  /// Roles provided for each saved conversation.
  enum SessionRoles { IdRole = Qt::UserRole + 1, TitleRole, TimestampRole, PreviewRole };

  /// Creates a model backed by the given session store and loads its sessions.
  /// \param store Store used to retrieve and delete sessions.
  /// \param parent QObject that owns this model, or `nullptr`.
  explicit SessionListModel(SessionStore* store, QObject* parent = nullptr);

  /// Releases message objects held by the model.
  ~SessionListModel();

  /// Returns the number of top-level session rows.
  /// \param parent Parent index; valid parents have no child rows.
  int rowCount(const QModelIndex& parent = QModelIndex{}) const override;

  /// Returns data for a session row and role, or an invalid QVariant for an invalid row or role.
  /// \param index Index of the requested row.
  /// \param role Data role to retrieve.
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

  /// Returns the names of the model's custom roles.
  QHash<int, QByteArray> roleNames() const override;

  /// Reloads the sessions from the backing store.
  Q_INVOKABLE void refresh();

  /// Deletes a session by its row index.
  /// \param index Row to delete; out-of-range indices are ignored.
  Q_INVOKABLE void deleteSession(int index);

  /// Deletes a session by its identifier.
  /// \param id Identifier of the session to delete.
  Q_INVOKABLE void deleteSessionById(const QString& id);

  /// Returns the identifier of a session, or an empty string for an invalid index.
  /// \param index Session row.
  QString sessionId(int index) const;

  /// Returns copies of a session's messages, or an empty list for an invalid index.
  /// \param index Session row.
  QList<ChatMessage*> messages(int index) const;

  /// Returns copies of a session's messages by identifier, or an empty list if not found.
  /// \param id Session identifier.
  QList<ChatMessage*> messagesById(const QString& id) const;

  /// Emitted when an operation on the backing store fails.
  /// \param message Description of the error.
  Q_SIGNAL void errorOccurred(const QString& message);

private:
  SessionStore* m_store;
  QList<Session> m_sessions;

  void clearSessions();
};
