#pragma once

#include <QDateTime>
#include <QObject>
#include <QString>

#include <functional>
#include <string>

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
  // The notes of the user and the profile picture, like the Android OSM profile.
  Q_PROPERTY(QString notesUrl READ notesUrl NOTIFY changed)
  Q_PROPERTY(QString imageUrl READ imageUrl NOTIFY changed)
  // A login or upload is in progress.
  Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
  // Local edits not uploaded yet.
  Q_PROPERTY(int pendingEdits READ pendingEdits NOTIFY editsChanged)
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
  QString notesUrl() const;
  QString imageUrl() const;
  bool busy() const { return m_loggingIn || m_uploading; }
  int pendingEdits() const { return m_pendingEdits; }
  QDateTime lastUpload() const { return m_lastUpload; }
  QString registrationUrl() const;
  QString resetPasswordUrl() const;

  // Emits changed or loginFailed when done.
  Q_INVOKABLE void login(QString const & user, QString const & password);
  // Logs in on the OpenStreetMap website, like the Android OAuth2 login: the browser comes back with a code by an
  // om:// link, for loginWithCode().
  Q_INVOKABLE void loginInBrowser();
  void loginWithCode(QString const & code);
  Q_INVOKABLE void logout();
  // Counts the pending edits again and uploads them and the notes when logged in.
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
  // Gets a token on the network thread and keeps it.
  void Authorize(std::function<std::string()> getToken);

  bool m_loggingIn = false;
  bool m_uploading = false;
  int m_pendingEdits = 0;
  QDateTime m_lastUpload;
};
}  // namespace sailfish
