#pragma once

#include <QObject>
#include <QSettings>
#include <QString>
#include <functional>
#include <utility>


/// Loads and saves API credentials through a backend while supporting legacy settings.
class CredentialStore : public QObject {
  Q_OBJECT

public:
  /// Defines the asynchronous interface used to read, write, and remove a credential.
  class Backend : public QObject {
  public:
    /// Completion callback for a credential read.
    /// \param success Whether the read operation succeeded.
    /// \param found Whether a credential was present.
    /// \param value The credential value when found.
    /// \param error Error description when the operation failed.
    using ReadCallback = std::function<void(bool success, bool found, const QString& value,
                                            const QString& error)>;
    /// Completion callback for a credential write or removal.
    /// \param success Whether the operation succeeded.
    /// \param error Error description when the operation failed.
    using WriteCallback = std::function<void(bool success, const QString& error)>;

    /// Creates a backend with the given QObject parent.
    /// \param parent QObject that owns this backend, or `nullptr`.
    explicit Backend(QObject* parent = nullptr) : QObject{parent} {
    }

    /// Destroys the backend.
    ~Backend() override = default;

    /// Asynchronously reads the stored credential.
    /// \param callback Called with the result, presence flag, value, and error description.
    virtual void read(ReadCallback callback) = 0;
    /// Asynchronously removes the stored credential.
    /// \param callback Called with the operation result and error description.
    virtual void remove(WriteCallback callback) = 0;
    /// Asynchronously stores a credential value.
    /// \param value Credential to store.
    /// \param callback Called with the operation result and error description.
    virtual void write(const QString& value, WriteCallback callback) = 0;
  };

  /// Creates a credential store using the supplied settings and optional backend.
  /// \param settings Settings used for legacy migration.
  /// \param backend Credential backend; the platform backend is used when omitted.
  /// \param parent QObject that owns this store, or `nullptr`.
  explicit CredentialStore(QSettings* settings, Backend* backend = nullptr,
                           QObject* parent = nullptr);

  /// Loads the credential asynchronously and emits loadFinished() when complete.
  void load();

  /// Saves a credential asynchronously and emits saveFinished() when complete.
  /// \param apiKey Credential to store; an empty value removes the saved credential.
  void save(const QString& apiKey);

  /// Emitted when a credential load completes.
  /// \param apiKey Loaded value, or an empty string if none was found.
  /// \param success Whether the load succeeded.
  /// \param error Error description when the load failed.
  Q_SIGNAL void loadFinished(const QString& apiKey, bool success, const QString& error);

  /// Emitted when a credential save or removal completes.
  /// \param apiKey Value requested for saving.
  /// \param success Whether the operation succeeded.
  /// \param error Error description when the operation failed.
  Q_SIGNAL void saveFinished(const QString& apiKey, bool success, const QString& error);

private:
  void removeLegacyKey();

  QSettings* m_settings;
  Backend* m_backend;
};
