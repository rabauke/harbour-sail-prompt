#include <QCoreApplication>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QThread>
#include <iostream>
#include "AiChatModel/session_store.hpp"
#include "AiChatModel/session_list_model.hpp"
#include "AiChatModel/chat_message.hpp"

namespace {
  bool require(bool condition, const char* message) {
    if (!condition)
      std::cerr << "FAILED: " << message << std::endl;
    return condition;
  }

  void clearSessions(QList<Session>& sessions) {
    for (int i = 0; i < sessions.size(); ++i)
      SessionStore::clearSession(sessions[i]);
    sessions.clear();
  }

  bool writeJson(const QString& path, const QJsonObject& object) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
      return false;
    return file.write(QJsonDocument(object).toJson()) >= 0;
  }

  QJsonObject validObject(const QString& id) {
    QJsonObject message;
    message["role"] = static_cast<int>(ChatMessage::User);
    message["content"] = "Valid user message";
    QJsonArray messages;
    messages.append(message);
    QJsonObject object;
    object["id"] = id;
    object["title"] = "Valid";
    object["timestamp"] = "2025-01-02T03:04:05";
    object["messages"] = messages;
    return object;
  }
}  // namespace

int main(int argc, char* argv[]) {
  QCoreApplication app(argc, argv);

  {
    // testSaveAndLoad
    QTemporaryDir tempDir;
    if (!require(tempDir.isValid(), "tempDir valid"))
      return 1;

    SessionStore store;
    store.setStoragePath(tempDir.path());

    QList<ChatMessage*> messages;
    messages.append(new ChatMessage(ChatMessage::User, "Hello"));
    messages.append(new ChatMessage(ChatMessage::Agent, "Hi there"));

    Session session = SessionStore::createSession(messages);
    if (!require(store.saveSession(session), "save session") ||
        !require(!session.id.isEmpty(), "session id generated") ||
        !require(session.title == "Hello", "title derivation"))
      return 1;

    QList<Session> loaded = store.loadAllSessions();
    if (!require(loaded.size() == 1, "loaded size 1") ||
        !require(loaded[0].id == session.id, "loaded id match") ||
        !require(loaded[0].title == session.title, "loaded title match") ||
        !require(loaded[0].messages.size() == 2, "loaded messages count") ||
        !require(loaded[0].messages[0]->content() == "Hello", "loaded message content"))
      return 1;

    clearSessions(loaded);
    SessionStore::clearSession(session);
    qDeleteAll(messages);
  }

  {
    // testEmptyChatSkip
    QTemporaryDir tempDir;
    SessionStore store;
    store.setStoragePath(tempDir.path());

    Session session;
    if (!require(!store.saveSession(session), "skip empty save") ||
        !require(store.loadAllSessions().size() == 0, "store empty after skip"))
      return 1;
  }

  {
    // testNoUserMessageSkip
    QTemporaryDir tempDir;
    SessionStore store;
    store.setStoragePath(tempDir.path());

    QList<ChatMessage*> messages;
    messages.append(new ChatMessage(ChatMessage::Agent, "I shouldn't be here alone"));
    Session session = SessionStore::createSession(messages);
    if (!require(!store.saveSession(session), "skip no-user-message save"))
      return 1;

    SessionStore::clearSession(session);
    qDeleteAll(messages);
  }

  {
    // testNewestSort
    QTemporaryDir tempDir;
    SessionStore store;
    store.setStoragePath(tempDir.path());

    auto saveSession = [&](const QString& userText) {
      QList<ChatMessage*> msgs;
      msgs.append(new ChatMessage(ChatMessage::User, userText));
      Session s = SessionStore::createSession(msgs);
      store.saveSession(s);
      SessionStore::clearSession(s);
      qDeleteAll(msgs);
      QThread::msleep(1100);
    };

    saveSession("Oldest");
    saveSession("Middle");
    saveSession("Newest");

    QList<Session> loaded = store.loadAllSessions();
    if (!require(loaded.size() == 3, "loaded size 3") ||
        !require(loaded[0].title == "Newest", "newest first") ||
        !require(loaded[1].title == "Middle", "middle second") ||
        !require(loaded[2].title == "Oldest", "oldest last"))
      return 1;
    clearSessions(loaded);
  }

  {
    // testCorruptFile
    QTemporaryDir tempDir;
    SessionStore store;
    store.setStoragePath(tempDir.path());

    QList<ChatMessage*> msgs;
    msgs.append(new ChatMessage(ChatMessage::User, "Valid"));
    Session s = SessionStore::createSession(msgs);
    store.saveSession(s);
    SessionStore::clearSession(s);
    qDeleteAll(msgs);

    QFile corruptFile(tempDir.path() + "/corrupt.json");
    corruptFile.open(QIODevice::WriteOnly);
    corruptFile.write("this is not json");
    corruptFile.close();

    QList<Session> loaded = store.loadAllSessions();
    if (!require(loaded.size() == 1, "skip corrupt file") ||
        !require(loaded[0].title == "Valid", "valid session preserved"))
      return 1;
    clearSessions(loaded);
  }

  {
    // testDelete
    QTemporaryDir tempDir;
    SessionStore store;
    store.setStoragePath(tempDir.path());

    QList<ChatMessage*> msgs;
    msgs.append(new ChatMessage(ChatMessage::User, "To Delete"));
    Session s = SessionStore::createSession(msgs);
    store.saveSession(s);
    QString id = s.id;
    SessionStore::clearSession(s);
    qDeleteAll(msgs);

    if (!require(store.deleteSession(id), "delete session") ||
        !require(store.loadAllSessions().size() == 0, "empty after delete"))
      return 1;
  }

  {
    // testStrictIdsAndTraversal
    QTemporaryDir tempDir;
    SessionStore store;
    store.setStoragePath(tempDir.path());
    Session session;
    session.id = "../outside";
    session.messages.append(new ChatMessage(ChatMessage::User, "Unsafe"));
    if (!require(!store.saveSession(session), "reject traversal save") ||
        !require(!store.deleteSession("../outside"), "reject traversal delete") ||
        !require(!SessionStore::isValidId("../outside"), "traversal id invalid"))
      return 1;
    SessionStore::clearSession(session);
  }

  {
    // testStrictJsonValidation
    QTemporaryDir tempDir;
    SessionStore store;
    store.setStoragePath(tempDir.path());
    const QString ids[] = {
        "11111111-1111-4111-8111-111111111111", "22222222-2222-4222-8222-222222222222",
        "33333333-3333-4333-8333-333333333333", "44444444-4444-4444-8444-444444444444",
        "55555555-5555-4555-8555-555555555555", "66666666-6666-4666-8666-666666666666"};
    QJsonObject mismatch = validObject("aaaaaaaa-aaaa-4aaa-8aaa-aaaaaaaaaaaa");
    QJsonObject badTimestamp = validObject(ids[1]);
    badTimestamp["timestamp"] = "not-a-date";
    QJsonObject badTitle = validObject(ids[2]);
    badTitle["title"] = 12;
    QJsonObject badMessages = validObject(ids[3]);
    badMessages["messages"] = "not-an-array";
    QJsonObject badRole = validObject(ids[4]);
    QJsonArray roleMessages = badRole["messages"].toArray();
    QJsonObject systemMessage = roleMessages[0].toObject();
    systemMessage["role"] = static_cast<int>(ChatMessage::System);
    roleMessages[0] = systemMessage;
    badRole["messages"] = roleMessages;
    QJsonObject badContent = validObject(ids[5]);
    QJsonArray contentMessages = badContent["messages"].toArray();
    QJsonObject numericContent = contentMessages[0].toObject();
    numericContent["content"] = 42;
    contentMessages[0] = numericContent;
    badContent["messages"] = contentMessages;
    const QJsonObject objects[] = {mismatch,    badTimestamp, badTitle,
                                   badMessages, badRole,      badContent};
    for (int i = 0; i < 6; ++i)
      if (!require(writeJson(tempDir.path() + "/" + ids[i] + ".json", objects[i]),
                   "write malformed fixture"))
        return 1;
    if (!require(store.loadAllSessions().isEmpty(), "skip all structurally invalid sessions"))
      return 1;
  }

  {
    // testSystemExclusionAndTitleNormalization
    QTemporaryDir tempDir;
    SessionStore store;
    store.setStoragePath(tempDir.path());
    Session session;
    session.messages.append(new ChatMessage(ChatMessage::System, "SECRET SYSTEM PROMPT"));
    session.messages.append(new ChatMessage(ChatMessage::User, "  First\n\t user   title  "));
    if (!require(store.saveSession(session), "save filtered session") ||
        !require(session.title == "First user title", "normalize title whitespace"))
      return 1;
    QFile file(tempDir.path() + "/" + session.id + ".json");
    if (!require(file.open(QIODevice::ReadOnly), "open saved json"))
      return 1;
    QByteArray json = file.readAll();
    if (!require(!json.contains("SECRET SYSTEM PROMPT"), "exclude system content") ||
        !require(!json.contains("apiKey"), "exclude api key field") ||
        !require(!json.contains("systemPrompt"), "exclude system prompt field"))
      return 1;
    SessionStore::clearSession(session);
  }

  {
    // testStableLookupAndDeletionAfterReorder
    QTemporaryDir tempDir;
    SessionStore store;
    store.setStoragePath(tempDir.path());
    Session older;
    older.messages.append(new ChatMessage(ChatMessage::User, "Selected older"));
    if (!require(store.saveSession(older), "save older"))
      return 1;
    const QString selectedId = older.id;
    QThread::msleep(1100);
    Session newer;
    newer.messages.append(new ChatMessage(ChatMessage::User, "Newer"));
    if (!require(store.saveSession(newer), "save newer"))
      return 1;
    SessionListModel model(&store);
    QList<ChatMessage*> selected = model.messagesById(selectedId);
    if (!require(selected.size() == 1 and selected[0]->content() == "Selected older",
                 "stable lookup after reorder"))
      return 1;
    qDeleteAll(selected);
    model.deleteSessionById(selectedId);
    if (!require(model.rowCount() == 1, "stable delete removes one") ||
        !require(model.sessionId(0) == newer.id, "stable delete keeps newer"))
      return 1;
    SessionStore::clearSession(older);
    SessionStore::clearSession(newer);
  }

  std::cout << "All session store backend tests passed" << std::endl;
  return 0;
}
