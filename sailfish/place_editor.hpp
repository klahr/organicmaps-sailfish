#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>

#include <memory>

namespace osm
{
class EditableMapObject;
}  // namespace osm

namespace sailfish
{
// Edits the place selected on the map, like the Android EditorFragment. Kept free of map headers,
// which Qt 5.6 moc can't parse.
class PlaceEditor : public QObject
{
  Q_OBJECT
  // A place could be loaded by start().
  Q_PROPERTY(bool valid READ valid NOTIFY changed)
  Q_PROPERTY(QString category READ category NOTIFY changed)
  Q_PROPERTY(bool nameEditable READ nameEditable NOTIFY changed)
  // The name in the local language of the map.
  Q_PROPERTY(QString name READ name WRITE setName NOTIFY changed)
  Q_PROPERTY(bool addressEditable READ addressEditable NOTIFY changed)
  Q_PROPERTY(QString street READ street WRITE setStreet NOTIFY changed)
  Q_PROPERTY(QStringList nearbyStreets READ nearbyStreets NOTIFY changed)
  Q_PROPERTY(QString houseNumber READ houseNumber WRITE setHouseNumber NOTIFY changed)
  // Editable details as {id, kind, label, value, inputHint}, see Kind and setField().
  Q_PROPERTY(QVariantList fields READ fields NOTIFY changed)
  // Local changes or a created place that are not uploaded yet, which can be discarded.
  Q_PROPERTY(bool canReset READ canReset NOTIFY changed)

public:
  enum Kind
  {
    Text,
    // A switch, value "yes" or "".
    Wifi,
    // A choice of selfServiceValues().
    SelfService
  };
  Q_ENUM(Kind)

  explicit PlaceEditor(QObject * parent = nullptr);
  ~PlaceEditor() override;

  bool valid() const { return m_valid; }
  QString category() const;
  bool nameEditable() const;
  QString name() const;
  void setName(QString const & name);
  bool addressEditable() const;
  QString street() const;
  void setStreet(QString const & street);
  QStringList nearbyStreets() const;
  QString houseNumber() const;
  void setHouseNumber(QString const & houseNumber);
  QVariantList fields() const;
  bool canReset() const;

  // Loads the place shown in the place page.
  Q_INVOKABLE void start();
  Q_INVOKABLE void setField(int id, QString const & value);
  // An error message for an invalid value, or an empty string.
  Q_INVOKABLE QString fieldError(int id, QString const & value) const;
  Q_INVOKABLE QString nameError(QString const & name) const;
  Q_INVOKABLE QString houseNumberError(QString const & houseNumber) const;
  // OSM values with their names for the SelfService field.
  Q_INVOKABLE QVariantList selfServiceValues() const;
  // Saves the changes locally, false on error. The place page shows the edited place.
  Q_INVOKABLE bool save();
  // Discards the local changes of the place, or deletes a created place.
  Q_INVOKABLE void reset();

signals:
  void changed();

private:
  std::unique_ptr<osm::EditableMapObject> m_object;
  bool m_valid = false;
};
}  // namespace sailfish
