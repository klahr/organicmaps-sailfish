#pragma once

#include <QDateTime>
#include <QObject>
#include <QString>

namespace sailfish
{
// The OpenStreetMap login and the upload of map edits, available to QML as the osmAccount context
// property. Edits are kept locally and uploaded once logged in, like on Android.
class OsmAccount : public QObject
{
  Q_OBJECT
  Q_PROPERTY(bool loggedIn READ loggedIn NOTIFY changed)
  Q_PROPERTY(QString userName READ userName NOTIFY changed)
  // Changesets of the account on the server, -1 until loaded.
  Q_PROPERTY(int changesets READ changesets NOTIFY changed)
  Q_PROPERTY(QString historyUrl READ historyUrl NOTIFY changed)
  // A login or upload is in progress.
  Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
  // Local edits not uploaded yet, and the uploaded ones.
  Q_PROPERTY(int pendingEdits READ pendingEdits NOTIFY editsChanged)
  Q_PROPERTY(int uploadedEdits READ uploadedEdits NOTIFY editsChanged)
  // Invalid before the first upload.
  Q_PROPERTY(QDateTime lastUpload READ lastUpload NOTIFY editsChanged)
  Q_PROPERTY(QString registrationUrl READ registrationUrl CONSTANT)
  Q_PROPERTY(QString resetPasswordUrl READ resetPasswordUrl CONSTANT)

public:
  explicit OsmAccount(QObject * parent = nullptr);

  bool loggedIn() const;
  QString userName() const;
  int changesets() const;
  QString historyUrl() const;
  bool busy() const { return m_loggingIn || m_uploading; }
  int pendingEdits() const { return m_pendingEdits; }
  int uploadedEdits() const { return m_uploadedEdits; }
  QDateTime lastUpload() const { return m_lastUpload; }
  QString registrationUrl() const;
  QString resetPasswordUrl() const;

  // Emits loggedInChanged or loginFailed when done.
  Q_INVOKABLE void login(QString const & user, QString const & password);
  Q_INVOKABLE void logout();
  // Uploads pending edits and notes when logged in.
  Q_INVOKABLE void uploadChanges();
  // Rereads the local edits, e.g. after saving one.
  Q_INVOKABLE void updateEdits();

signals:
  void changed();
  void busyChanged();
  void editsChanged();
  void loginFailed(QString const & message);

private:
  // Loads the display name and the changesets count of the logged in account.
  void LoadProfile();

  bool m_loggingIn = false;
  bool m_uploading = false;
  int m_pendingEdits = 0;
  int m_uploadedEdits = 0;
  QDateTime m_lastUpload;
};
}  // namespace sailfish
