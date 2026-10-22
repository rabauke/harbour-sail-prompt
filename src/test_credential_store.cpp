#include <QCoreApplication>
#include <QSettings>
#include <QTemporaryDir>
#include <iostream>
#include "credential_store.hpp"


namespace {

  bool require(bool condition, const char* message) {
    if (not condition)
      std::cerr << "FAILED: " << message << std::endl;
    return condition;
  }


  class FakeBackend : public CredentialStore::Backend {
  public:
    bool readSuccess{true};
    bool secretFound{false};
    bool writeSuccess{true};
    QString storedValue;
    QString readError;
    QString writeError;
    int reads{0};
    int writes{0};

    void read(ReadCallback callback) override {
      ++reads;
      callback(readSuccess, secretFound, storedValue, readError);
    }

    void write(const QString& value, WriteCallback callback) override {
      ++writes;
      if (writeSuccess)
        storedValue = value;
      callback(writeSuccess, writeError);
    }
  };


  bool runTests() {
    QTemporaryDir tempDir;
    if (not require(tempDir.isValid(), "temporary settings directory"))
      return false;

    const QString path{tempDir.path() + QStringLiteral("/settings.ini")};
    QSettings settings{path, QSettings::IniFormat};
    FakeBackend backend;
    CredentialStore store{&settings, &backend};
    QString loadedValue;
    bool loadedSuccess{false};
    QObject::connect(
        &store, &CredentialStore::loadFinished,
        [&loadedValue, &loadedSuccess](const QString& value, bool success, const QString&) {
          loadedValue = value;
          loadedSuccess = success;
        });

    settings.setValue(QStringLiteral("apiKey"), QStringLiteral("legacy-key"));
    backend.secretFound = true;
    backend.storedValue = QStringLiteral("secure-key");
    store.load();
    if (not require(loadedSuccess and loadedValue == QStringLiteral("secure-key"),
                    "secure secret takes precedence") ||
        not require(backend.reads == 1 and backend.writes == 0,
                    "secure-first read avoids write") ||
        not require(not settings.contains(QStringLiteral("apiKey")),
                    "stale plaintext key removed when secure key exists"))
      return false;

    backend.secretFound = false;
    settings.setValue(QStringLiteral("apiKey"), QStringLiteral("legacy-key"));
    store.load();
    if (not require(loadedSuccess and loadedValue == QStringLiteral("legacy-key"),
                    "legacy key migrates") ||
        not require(backend.storedValue == QStringLiteral("legacy-key"),
                    "migrated key is written securely") ||
        not require(not settings.contains(QStringLiteral("apiKey")),
                    "legacy key removed after successful migration"))
      return false;

    backend.writeSuccess = false;
    settings.setValue(QStringLiteral("apiKey"), QStringLiteral("recoverable-key"));
    store.load();
    if (not require(not loadedSuccess, "migration write failure is reported") ||
        not require(settings.value(QStringLiteral("apiKey")).toString() ==
                        QStringLiteral("recoverable-key"),
                    "legacy key preserved after migration failure"))
      return false;

    backend.writeSuccess = true;
    backend.secretFound = false;
    settings.remove(QStringLiteral("apiKey"));
    store.load();
    if (not require(loadedSuccess and loadedValue.isEmpty(),
                    "missing credential loads as empty") ||
        not require(backend.writes == 2, "empty legacy value does not trigger a write"))
      return false;

    QString savedValue;
    bool savedSuccess{false};
    QObject::connect(
        &store, &CredentialStore::saveFinished,
        [&savedValue, &savedSuccess](const QString& value, bool success, const QString&) {
          savedValue = value;
          savedSuccess = success;
        });
    settings.setValue(QStringLiteral("apiKey"), QStringLiteral("old-key"));
    backend.writeSuccess = false;
    store.save(QStringLiteral("new-key"));
    if (not require(not savedSuccess, "credential save failure is reported") ||
        not require(
            settings.value(QStringLiteral("apiKey")).toString() == QStringLiteral("old-key"),
            "legacy key preserved after credential save failure"))
      return false;

    backend.writeSuccess = true;
    store.save(QStringLiteral("new-key"));
    if (not require(savedSuccess and savedValue == QStringLiteral("new-key"),
                    "credential save succeeds") ||
        not require(backend.storedValue == QStringLiteral("new-key"),
                    "new credential is written securely") ||
        not require(not settings.contains(QStringLiteral("apiKey")),
                    "legacy key removed after successful save"))
      return false;

    return true;
  }

}  // namespace


int main(int argc, char* argv[]) {
  QCoreApplication app{argc, argv};
  if (not runTests())
    return 1;
  std::cout << "All credential store tests passed" << std::endl;
  return 0;
}
