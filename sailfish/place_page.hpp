#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

#include <string_view>

class Framework;

namespace sailfish
{
// The selected map object, following the Framework place page events. Kept free of map headers,
// which Qt 5.6 moc can't parse.
class PlacePage : public QObject
{
  Q_OBJECT
  Q_PROPERTY(bool open READ open NOTIFY changed)
  Q_PROPERTY(QString title READ title NOTIFY changed)
  Q_PROPERTY(QString subtitle READ subtitle NOTIFY changed)
  Q_PROPERTY(QString address READ address NOTIFY changed)
  Q_PROPERTY(QString distance READ distance NOTIFY distanceChanged)
  Q_PROPERTY(QString coordinates READ coordinates NOTIFY changed)
  // Opening hours like the Android preview: state line (OpenState) with a description, and the raw
  // schedule shown when expanded.
  Q_PROPERTY(int openState READ openState NOTIFY changed)
  Q_PROPERTY(QString openTitle READ openTitle NOTIFY changed)
  Q_PROPERTY(QString openDescription READ openDescription NOTIFY changed)
  Q_PROPERTY(QString openingHours READ openingHours NOTIFY changed)
  Q_PROPERTY(QString wikiDescription READ wikiDescription NOTIFY changed)
  Q_PROPERTY(QString wikiUrl READ wikiUrl NOTIFY changed)
  // Detail rows as {icon, text, url}; url is opened externally when set.
  Q_PROPERTY(QVariantList details READ details NOTIFY changed)

public:
  enum OpenState
  {
    OpenUnknown,
    Open,
    Closed
  };
  Q_ENUM(OpenState)

  explicit PlacePage(Framework & framework, QObject * parent = nullptr);
  ~PlacePage() override;

  bool open() const { return m_open; }
  QString title() const { return m_title; }
  QString subtitle() const { return m_subtitle; }
  QString address() const { return m_address; }
  QString distance() const { return m_distance; }
  QString coordinates() const { return m_coordinates; }
  QVariantList details() const { return m_details; }
  int openState() const { return m_openState; }
  QString openTitle() const { return m_openTitle; }
  QString openDescription() const { return m_openDescription; }
  QString openingHours() const { return m_openingHours; }
  QString wikiDescription() const { return m_wikiDescription; }
  QString wikiUrl() const { return m_wikiUrl; }

  Q_INVOKABLE void close();
  // Cycles the coordinates format and remembers it, like a tap on the mobile place pages.
  Q_INVOKABLE void nextCoordinatesFormat();
  Q_INVOKABLE void copyCoordinates();

  // Called on location updates while the page is open.
  void UpdateDistance();

signals:
  void changed();
  void distanceChanged();

private:
  void Update();
  void UpdateOpeningHours(std::string_view openingHours);

  Framework & m_framework;
  bool m_open = false;
  QString m_title;
  QString m_subtitle;
  QString m_address;
  QString m_distance;
  QString m_coordinates;
  QString m_coordinatesValue;
  QVariantList m_details;
  int m_openState = OpenUnknown;
  QString m_openTitle;
  QString m_openDescription;
  QString m_openingHours;
  QString m_wikiDescription;
  QString m_wikiUrl;
};
}  // namespace sailfish
