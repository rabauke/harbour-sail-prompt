#pragma once

#include <QObject>
#include <QSettings>
#include <QString>
#include <functional>
#include <utility>


class CredentialStore : public QObject {
  Q_OBJECT

public:
  class Backend : public QObject {
  public:
    using ReadCallback = std::function<void(bool success, bool found, const QString& value,
                                            const QString& error)>;
    using WriteCallback = std::function<void(bool success, const QString& error)>;

    explicit Backend(QObject* parent = nullptr) : QObject{parent} {
    }
    ~Backend() override = default;

    virtual void read(ReadCallback callback) = 0;
    virtual void write(const QString& value, WriteCallback callback) = 0;
  };

  explicit CredentialStore(QSettings* settings, Backend* backend = nullptr,
                           QObject* parent = nullptr);

  void load();
  void save(const QString& apiKey);

  Q_SIGNAL void loadFinished(const QString& apiKey, bool success, const QString& error);
  Q_SIGNAL void saveFinished(const QString& apiKey, bool success, const QString& error);

private:
  bool removeLegacyKey(QString* error);

  QSettings* m_settings;
  Backend* m_backend;
};
