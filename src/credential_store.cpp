#include "credential_store.hpp"
#include <Sailfish/Secrets/deletesecretrequest.h>
#include <Sailfish/Secrets/secret.h>
#include <Sailfish/Secrets/secretmanager.h>
#include <Sailfish/Secrets/result.h>
#include <Sailfish/Secrets/storedsecretrequest.h>
#include <Sailfish/Secrets/storesecretrequest.h>
#include <QDebug>
#include <QTimer>
#include <functional>
#include <memory>


namespace {

  const QString credentialName{QStringLiteral("harbour-sail-prompt-api-key")};


  class SailfishSecretsBackend : public CredentialStore::Backend {
  public:
    explicit SailfishSecretsBackend(QObject* parent = nullptr) : Backend{parent} {
    }


    void read(ReadCallback callback) override {
      auto* request{new Sailfish::Secrets::StoredSecretRequest};
      request->setParent(this);
      request->setManager(&m_manager);
      request->setIdentifier(identifier());
      request->setUserInteractionMode(Sailfish::Secrets::SecretManager::SystemInteraction);
      auto done{std::make_shared<bool>(false)};
      connect(
          request, &Sailfish::Secrets::Request::statusChanged, this,
          [request, done, callback]() {
            if (*done or request->status() != Sailfish::Secrets::Request::Finished)
              return;
            *done = true;

            const Sailfish::Secrets::Result result{request->result()};
            const QByteArray data{request->secret().data()};
            request->deleteLater();
            if (result.code() == Sailfish::Secrets::Result::Succeeded) {
              callback(true, true, QString::fromUtf8(data), {});
            } else if (result.errorCode() == Sailfish::Secrets::Result::InvalidSecretError) {
              callback(true, false, {}, {});
            } else {
              callback(false, false, {}, result.errorMessage());
            }
          });
      watchTimeout(request, done, [callback]() {
        callback(false, false, {}, timeoutMessage());
      });
      request->startRequest();
    }


    void remove(WriteCallback callback) override {
      auto* request{new Sailfish::Secrets::DeleteSecretRequest};
      request->setParent(this);
      request->setManager(&m_manager);
      request->setIdentifier(identifier());
      request->setUserInteractionMode(Sailfish::Secrets::SecretManager::SystemInteraction);
      auto done{std::make_shared<bool>(false)};
      connect(request, &Sailfish::Secrets::Request::statusChanged, this,
              [request, done, callback]() {
                if (*done or request->status() != Sailfish::Secrets::Request::Finished)
                  return;
                *done = true;

                const Sailfish::Secrets::Result result{request->result()};
                request->deleteLater();
                callback(result.code() == Sailfish::Secrets::Result::Succeeded ||
                             result.errorCode() ==
                                 Sailfish::Secrets::Result::InvalidSecretError,
                         result.errorMessage());
              });
      watchTimeout(request, done, [callback]() { callback(false, timeoutMessage()); });
      request->startRequest();
    }


    // Sailfish Secrets refuses to overwrite an existing standalone secret, so an
    // existing one has to be deleted first. The previous value is read beforehand
    // and restored if storing the new one fails, so a failure does not lose the key.
    void write(const QString& value, WriteCallback callback) override {
      read([this, value, callback](bool readOk, bool found, const QString& previous,
                                   const QString& readError) {
        if (not readOk) {
          callback(false, readError);
          return;
        }
        remove([this, value, callback, found, previous](bool removed, const QString& error) {
          if (not removed) {
            callback(false, error);
            return;
          }
          storeSecret(value, [this, callback, found, previous](bool stored,
                                                               const QString& storeError) {
            if (stored or not found) {
              callback(stored, storeError);
              return;
            }
            storeSecret(previous, [callback, storeError](bool restored, const QString&) {
              if (not restored)
                qWarning() << "Unable to restore the previous API key after a failed save";
              callback(false, storeError);
            });
          });
        });
      });
    }

  private:
    void storeSecret(const QString& value, WriteCallback callback) {
      Sailfish::Secrets::Secret secret{identifier()};
      secret.setData(value.toUtf8());
      secret.setType(Sailfish::Secrets::Secret::TypeBlob);

      auto* request{new Sailfish::Secrets::StoreSecretRequest};
      request->setParent(this);
      request->setManager(&m_manager);
      request->setSecretStorageType(
          Sailfish::Secrets::StoreSecretRequest::StandaloneDeviceLockSecret);
      request->setDeviceLockUnlockSemantic(
          Sailfish::Secrets::SecretManager::DeviceLockKeepUnlocked);
      request->setAccessControlMode(Sailfish::Secrets::SecretManager::OwnerOnlyMode);
      request->setEncryptionPluginName(
          Sailfish::Secrets::SecretManager::DefaultEncryptionPluginName);
      request->setUserInteractionMode(Sailfish::Secrets::SecretManager::SystemInteraction);
      request->setSecret(secret);
      auto done{std::make_shared<bool>(false)};
      connect(request, &Sailfish::Secrets::Request::statusChanged, this,
              [request, done, callback]() {
                if (*done or request->status() != Sailfish::Secrets::Request::Finished)
                  return;
                *done = true;

                const Sailfish::Secrets::Result result{request->result()};
                request->deleteLater();
                callback(result.code() == Sailfish::Secrets::Result::Succeeded,
                         result.errorMessage());
              });
      watchTimeout(request, done, [callback]() { callback(false, timeoutMessage()); });
      request->startRequest();
    }

    static Sailfish::Secrets::Secret::Identifier identifier() {
      return Sailfish::Secrets::Secret::Identifier{
          credentialName, QString{},
          Sailfish::Secrets::SecretManager::DefaultStoragePluginName};
    }

    static QString timeoutMessage() {
      return QStringLiteral("The secrets service did not respond in time");
    }

    // Guarantees that the callback is invoked even if the request never finishes.
    static void watchTimeout(QObject* request, const std::shared_ptr<bool>& done,
                             std::function<void()> onTimeout) {
      auto* timer{new QTimer{request}};
      timer->setSingleShot(true);
      QObject::connect(timer, &QTimer::timeout, request,
                       [request, done, onTimeout = std::move(onTimeout)]() {
                         if (*done)
                           return;
                         *done = true;
                         request->deleteLater();
                         onTimeout();
                       });
      timer->start(requestTimeoutMs);
    }

    static constexpr int requestTimeoutMs{60000};
    Sailfish::Secrets::SecretManager m_manager;
  };


  QString failureMessage(const QString& error, const QString& fallback) {
    return error.isEmpty() ? fallback : error;
  }

}  // namespace


CredentialStore::CredentialStore(QSettings* settings, Backend* backend, QObject* parent)
    : QObject{parent},
      m_settings{settings},
      m_backend{backend ? backend : new SailfishSecretsBackend{this}} {
}


void CredentialStore::load() {
  m_backend->read([this](bool success, bool found, const QString& value, const QString& error) {
    if (not success) {
      Q_EMIT loadFinished({}, false,
                          failureMessage(error, QStringLiteral("Unable to read the API key")));
      return;
    }

    if (found) {
      removeLegacyKey();
      Q_EMIT loadFinished(value, true, {});
      return;
    }

    const QString legacyValue{m_settings->value(QStringLiteral("apiKey")).toString().trimmed()};
    if (legacyValue.isEmpty()) {
      Q_EMIT loadFinished({}, true, {});
      return;
    }

    m_backend->write(legacyValue, [this, legacyValue](bool written, const QString& writeError) {
      if (not written) {
        Q_EMIT loadFinished(
            {}, false,
            failureMessage(writeError, QStringLiteral("Unable to migrate the API key")));
        return;
      }

      removeLegacyKey();
      Q_EMIT loadFinished(legacyValue, true, {});
    });
  });
}


void CredentialStore::save(const QString& apiKey) {
  const QString key{apiKey.trimmed()};
  if (key.isEmpty()) {
    m_backend->remove([this](bool removed, const QString& removeError) {
      if (not removed) {
        Q_EMIT saveFinished(
            {}, false,
            failureMessage(removeError, QStringLiteral("Unable to remove the API key")));
        return;
      }
      removeLegacyKey();
      Q_EMIT saveFinished({}, true, {});
    });
    return;
  }

  // The backend replaces an existing secret and restores the previous value if the
  // replacement fails.
  m_backend->write(key, [this, key](bool written, const QString& writeError) {
    if (not written) {
      Q_EMIT saveFinished(
          {}, false, failureMessage(writeError, QStringLiteral("Unable to save the API key")));
      return;
    }

    removeLegacyKey();
    Q_EMIT saveFinished(key, true, {});
  });
}


void CredentialStore::removeLegacyKey() {
  if (not m_settings->contains(QStringLiteral("apiKey")))
    return;

  m_settings->remove(QStringLiteral("apiKey"));
  m_settings->sync();
  if (m_settings->status() != QSettings::NoError)
    qWarning("Unable to remove the legacy API key from application settings");
}
