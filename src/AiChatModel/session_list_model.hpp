#ifndef SESSION_LIST_MODEL_HPP
#define SESSION_LIST_MODEL_HPP

#include <QAbstractListModel>
#include <QList>
#include "session_store.hpp"

class SessionListModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum SessionRoles {
        IdRole = Qt::UserRole + 1,
        TitleRole,
        TimestampRole,
        PreviewRole
    };

    explicit SessionListModel(SessionStore *store, QObject *parent = nullptr);
    ~SessionListModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void deleteSession(int index);
    Q_INVOKABLE void deleteSessionById(const QString& id);

    QString sessionId(int index) const;
    QList<ChatMessage*> messages(int index) const;
    QList<ChatMessage*> messagesById(const QString& id) const;

signals:
    void errorOccurred(const QString& message);

private:
    SessionStore *m_store;
    QList<Session> m_sessions;
    void clearSessions();
};

#endif // SESSION_LIST_MODEL_HPP
