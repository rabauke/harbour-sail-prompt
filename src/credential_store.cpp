#include "credential_store.hpp"
#include <Sailfish/Secrets/secret.h>
#include <Sailfish/Secrets/secretmanager.h>
#include <Sailfish/Secrets/result.h>
#include <Sailfish/Secrets/storedsecretrequest.h>
#include <Sailfish/Secrets/storesecretrequest.h>


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
      connect(
          request, &Sailfish::Secrets::Request::statusChanged, this,
          [request, callback = std::move(callback)]() mutable {
            if (request->status() != Sailfish::Secrets::Request::Finished)
              return;

            const Sailfish::Secrets::Result result{request->result()};
            const QByteArray data{request->secret().data()};
            request->deleteLater();
            if (result.code() == Sailfish::Secrets::Result::Succeeded) {
              const QString value{QString::fromUtf8(data)};
              callback(true, true, value, {});
            } else if (result.errorCode() == Sailfish::Secrets::Result::InvalidSecretError) {
              callback(true, false, {}, {});
            } else {
              callback(false, false, {}, result.errorMessage());
            }
          });
      request->startRequest();
    }


    void write(const QString& value, WriteCallback callback) override {
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
      connect(request, &Sailfish::Secrets::Request::statusChanged, this,
              [request, callback = std::move(callback)]() mutable {
                if (request->status() != Sailfish::Secrets::Request::Finished)
                  return;

                const Sailfish::Secrets::Result result{request->result()};
                request->deleteLater();
                callback(result.code() == Sailfish::Secrets::Result::Succeeded,
                         result.errorMessage());
              });
      request->startRequest();
    }

  private:
    static Sailfish::Secrets::Secret::Identifier identifier() {
      return Sailfish::Secrets::Secret::Identifier{
          credentialName, QString{},
          Sailfish::Secrets::SecretManager::DefaultStoragePluginName};
    }

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
      QString removalError;
      if (not removeLegacyKey(&removalError)) {
        Q_EMIT loadFinished({}, false, removalError);
        return;
      }
      Q_EMIT loadFinished(value, true, {});
      return;
    }

    const QString legacyValue{m_settings->value(QStringLiteral("apiKey")).toString()};
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

      QString removalError;
      if (not removeLegacyKey(&removalError)) {
        Q_EMIT loadFinished({}, false, removalError);
        return;
      }
      Q_EMIT loadFinished(legacyValue, true, {});
    });
  });
}


void CredentialStore::save(const QString& apiKey) {
  m_backend->write(apiKey, [this, apiKey](bool written, const QString& writeError) {
    if (not written) {
      Q_EMIT saveFinished(
          {}, false, failureMessage(writeError, QStringLiteral("Unable to save the API key")));
      return;
    }

    QString removalError;
    if (not removeLegacyKey(&removalError)) {
      Q_EMIT saveFinished({}, false, removalError);
      return;
    }
    Q_EMIT saveFinished(apiKey, true, {});
  });
}


bool CredentialStore::removeLegacyKey(QString* error) {
  if (not m_settings->contains(QStringLiteral("apiKey")))
    return true;

  m_settings->remove(QStringLiteral("apiKey"));
  m_settings->sync();
  if (m_settings->status() == QSettings::NoError)
    return true;

  *error = QStringLiteral("Unable to remove the legacy API key from application settings");
  return false;
}
