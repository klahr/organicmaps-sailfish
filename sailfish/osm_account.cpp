#include "sailfish/osm_account.hpp"

#include "sailfish/app_info.hpp"

#include "editor/osm_auth.hpp"
#include "editor/osm_editor.hpp"
#include "editor/server_api.hpp"

#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "base/logging.hpp"
#include "base/timer.hpp"

#include <QGuiApplication>
#include <QPointer>

namespace sailfish
{
namespace
{
std::string_view constexpr kToken = "SailfishOsmToken";
std::string_view constexpr kUserName = "SailfishOsmUserName";
std::string_view constexpr kChangesets = "SailfishOsmChangesets";

std::string Token()
{
  std::string token;
  settings::TryGet(kToken, token);
  return token;
}
}  // namespace

OsmAccount::OsmAccount(QObject * parent) : QObject(parent)
{
  updateEdits();
  // Android uploads in a background job; here edits go up when the app is put aside and on start.
  connect(qApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState state)
  {
    if (state != Qt::ApplicationActive)
      uploadChanges();
  });
  uploadChanges();
}

bool OsmAccount::loggedIn() const
{
  return !Token().empty();
}

QString OsmAccount::userName() const
{
  std::string name;
  settings::TryGet(kUserName, name);
  return QString::fromStdString(name);
}

int OsmAccount::changesets() const
{
  int count = -1;
  settings::TryGet(kChangesets, count);
  return count;
}

QString OsmAccount::historyUrl() const
{
  return QString::fromStdString(osm::OsmOAuth::ServerAuth().GetHistoryURL(userName().toStdString()));
}

QString OsmAccount::registrationUrl() const
{
  return QString::fromStdString(osm::OsmOAuth::ServerAuth().GetRegistrationURL());
}

QString OsmAccount::resetPasswordUrl() const
{
  return QString::fromStdString(osm::OsmOAuth::ServerAuth().GetResetPasswordURL());
}

void OsmAccount::login(QString const & user, QString const & password)
{
  if (m_loggingIn)
    return;
  m_loggingIn = true;
  emit busyChanged();

  QPointer<OsmAccount> self(this);
  GetPlatform().RunTask(Platform::Thread::Network,
                        [self, user = user.trimmed().toStdString(), password = password.toStdString()]
  {
    std::string token;
    std::string error;
    try
    {
      auto auth = osm::OsmOAuth::ServerAuth();
      if (auth.AuthorizePassword(user, password))
        token = auth.GetAuthToken();
    }
    catch (std::exception const & e)
    {
      LOG(LWARNING, ("OSM login failed:", e.what()));
      error = e.what();
    }

    GetPlatform().RunTask(Platform::Thread::Gui, [self, token, error]
    {
      if (!self)
        return;
      self->m_loggingIn = false;
      emit self->busyChanged();
      if (token.empty())
      {
        emit self->loginFailed(error.empty() ? Localized("invalid_username_or_password")
                                             : Localized("editor_login_error_dialog"));
        return;
      }
      settings::Set(kToken, token);
      emit self->changed();
      self->LoadProfile();
      self->uploadChanges();
    });
  });
}

void OsmAccount::logout()
{
  settings::Delete(kToken);
  settings::Delete(kUserName);
  settings::Delete(kChangesets);
  emit changed();
}

void OsmAccount::LoadProfile()
{
  QPointer<OsmAccount> self(this);
  GetPlatform().RunTask(Platform::Thread::Network, [self, token = Token()]
  {
    osm::UserPreferences prefs;
    try
    {
      prefs = osm::ServerApi06(osm::OsmOAuth::ServerAuth(token)).GetUserPreferences();
    }
    catch (std::exception const & e)
    {
      LOG(LWARNING, ("Can't load OSM user preferences:", e.what()));
      return;
    }

    GetPlatform().RunTask(Platform::Thread::Gui, [self, prefs]
    {
      // Logged out meanwhile.
      if (!self || !self->loggedIn())
        return;
      settings::Set(kUserName, prefs.m_displayName);
      settings::Set(kChangesets, static_cast<int>(prefs.m_changesets));
      emit self->changed();
    });
  });
}

void OsmAccount::uploadChanges()
{
  auto const token = Token();
  if (token.empty() || m_uploading)
    return;

  QPointer<OsmAccount> self(this);
  auto const onFinish = [self](osm::Editor::UploadResult result)
  {
    GetPlatform().RunTask(Platform::Thread::Gui, [self, result]
    {
      if (!self)
        return;
      self->m_uploading = false;
      emit self->busyChanged();
      self->updateEdits();
      if (result == osm::Editor::UploadResult::Success)
        self->LoadProfile();
    });
  };

  auto const started = osm::Editor::Instance().UploadChanges(
      token, {{"created_by", "Organic Maps Sailfish " + GetPlatform().Version()}}, onFinish);
  if (started == osm::Editor::UploadStart::Started)
  {
    m_uploading = true;
    emit busyChanged();
  }
}

void OsmAccount::updateEdits()
{
  auto const stats = osm::Editor::Instance().GetStats();
  m_pendingEdits = static_cast<int>(stats.m_edits.size() - stats.m_uploadedCount);
  m_lastUpload = stats.m_lastUploadTimestamp == base::INVALID_TIME_STAMP
                   ? QDateTime()
                   : QDateTime::fromTime_t(static_cast<uint>(stats.m_lastUploadTimestamp));
  emit editsChanged();
}
}  // namespace sailfish
