#pragma once

#include <QAbstractListModel>
#include <QList>
#include "chat_message.hpp"


class ChatMessageListModel : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(int count READ count NOTIFY countChanged)
public:
  enum MyItemRoles { RoleRole = Qt::UserRole + 1, ContentRole = Qt::UserRole + 2 };

  explicit ChatMessageListModel(QObject *parent = nullptr);
  ~ChatMessageListModel();

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int count() const;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  Q_INVOKABLE void add(ChatMessage *message);
  Q_INVOKABLE void remove(int index);
  Q_INVOKABLE void clear();
  Q_INVOKABLE void setContent(int index, const QString &content);

  Q_INVOKABLE ChatMessage *get(int index) const;

  Q_SIGNAL void countChanged();

private:
  QList<ChatMessage *> m_messages;
};
