#include "sailfish/app_info.hpp"
#include "sailfish/bookmarks_model.hpp"
#include "sailfish/countries_model.hpp"
#include "sailfish/framework_access.hpp"
#include "sailfish/map_item.hpp"
#include "sailfish/place_page.hpp"
#include "sailfish/search_model.hpp"

#include "map/framework.hpp"

#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"

#include "base/logging.hpp"

#include <qqml.h>
#include <QDir>
#include <QGuiApplication>
#include <QQmlContext>
#include <QQuickView>
#include <QStandardPaths>
#include <QSurfaceFormat>

#include <sailfishapp.h>

#include <clocale>
#include <memory>

namespace
{
Framework * g_framework = nullptr;

void SetEnvIfUnset(char const * name, QString const & value)
{
  if (qEnvironmentVariableIsEmpty(name))
    qputenv(name, value.toUtf8());
}
}  // namespace

namespace sailfish
{
Framework & GetFramework()
{
  return *g_framework;
}
}  // namespace sailfish

__attribute__((visibility("default"))) int OrganicMapsMain(int argc, char * argv[])
{
  // Our double parsing code (base/string_utils.hpp) needs dots as a floating point delimiters.
  std::setlocale(LC_NUMERIC, "C");

  // Drape's contexts share resources with the scene graph through the global share context.
  QGuiApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
  QSurfaceFormat format;
  format.setRenderableType(QSurfaceFormat::OpenGLES);
  format.setVersion(3, 0);
  format.setDepthBufferSize(24);
  format.setStencilBufferSize(8);
  QSurfaceFormat::setDefaultFormat(format);

  std::unique_ptr<QGuiApplication> app(SailfishApp::application(argc, argv));
  app->setOrganizationName(QStringLiteral("app.organicmaps"));
  app->setApplicationName(QStringLiteral("organicmaps"));

  // Platform reads these on first use; the defaults match the installed RPM layout and Sailjail.
  SetEnvIfUnset("MWM_RESOURCES_DIR", SailfishApp::pathTo(QStringLiteral("data")).toLocalFile());
  QString const writableDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  QDir().mkpath(writableDir);
  SetEnvIfUnset("MWM_WRITABLE_DIR", writableDir);

  Platform & platform = GetPlatform();
  LOG(LINFO, ("Resources:", platform.ResourcesDir(), "Writable:", platform.WritableDir(),
              "Settings:", platform.SettingsDir()));

  Framework framework;
  framework.SetupMeasurementSystem();
  framework.GetStorage().SetLocale(languages::GetCurrentTwine());
  g_framework = &framework;
  sailfish::BookmarksNotifier::LoadBookmarks(framework);

  qmlRegisterType<sailfish::MapItem>("app.organicmaps", 1, 0, "MapItem");
  qmlRegisterType<sailfish::CountriesModel>("app.organicmaps", 1, 0, "CountriesModel");
  qmlRegisterType<sailfish::SearchModel>("app.organicmaps", 1, 0, "SearchModel");
  qmlRegisterType<sailfish::BookmarkCategoriesModel>("app.organicmaps", 1, 0, "BookmarkCategoriesModel");
  qmlRegisterType<sailfish::BookmarksModel>("app.organicmaps", 1, 0, "BookmarksModel");
  qmlRegisterUncreatableType<sailfish::PlacePage>("app.organicmaps", 1, 0, "PlacePage", "Owned by MapItem");

  // Declared before the view, which uses it until it is destroyed.
  sailfish::AppInfo appInfo;
  std::unique_ptr<QQuickView> view(SailfishApp::createView());
  view->rootContext()->setContextProperty(QStringLiteral("appInfo"), &appInfo);
  view->setSource(SailfishApp::pathToMainQml());
  view->show();

  return app->exec();
}
