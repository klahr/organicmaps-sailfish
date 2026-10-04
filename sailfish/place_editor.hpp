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
  // Names in other languages as {code, language, value}, see setLocalizedName().
  Q_PROPERTY(QVariantList localizedNames READ localizedNames NOTIFY changed)
  Q_PROPERTY(bool addressEditable READ addressEditable NOTIFY changed)
  Q_PROPERTY(QString street READ street WRITE setStreet NOTIFY changed)
  Q_PROPERTY(QStringList nearbyStreets READ nearbyStreets NOTIFY changed)
  Q_PROPERTY(QString houseNumber READ houseNumber WRITE setHouseNumber NOTIFY changed)
  // Editable fields as {id, kind, section, icon, label, value, inputHint} in the Android order, see Kind,
  // Section and setField().
  Q_PROPERTY(QVariantList fields READ fields NOTIFY changed)
  // The ResetAction at the bottom of the editor, as on Android.
  Q_PROPERTY(int resetAction READ resetAction NOTIFY changed)

public:
  enum Kind
  {
    Text,
    // A switch, value "yes" or "".
    Wifi,
    // A choice of selfServiceValues().
    SelfService,
    // An opening_hours value, edited by OpeningHoursEditor.
    OpeningHours,
    // Keys of cuisines() separated by ';'.
    Cuisine,
    // Phone numbers separated by ';', one field each like on Android.
    Phone,
    // "yes", "no" or "" for unknown, like the iOS segmented fields.
    YesNo
  };
  Q_ENUM(Kind)

  enum ResetAction
  {
    // A new place.
    NoReset,
    ResetEdits,
    RemovePlace,
    PlaceDoesntExist
  };
  Q_ENUM(ResetAction)

  // The cards of the Android editor that hold fields.
  enum Section
  {
    Address,
    Details,
    SocialMedia
  };
  Q_ENUM(Section)

  explicit PlaceEditor(QObject * parent = nullptr);
  ~PlaceEditor() override;

  bool valid() const { return m_valid; }
  QString category() const;
  bool nameEditable() const;
  QString name() const;
  void setName(QString const & name);
  QVariantList localizedNames() const;
  bool addressEditable() const;
  QString street() const;
  void setStreet(QString const & street);
  QStringList nearbyStreets() const;
  QString houseNumber() const;
  void setHouseNumber(QString const & houseNumber);
  QVariantList fields() const;
  int resetAction() const;

  // Loads the place shown in the place page.
  Q_INVOKABLE void start();
  // Starts a new place of a categories() type, false when no map is loaded there.
  Q_INVOKABLE bool create(QString const & type, double lat, double lon);
  // Creatable types as {type, name, recent}: recent ones first, then all sorted by name, like the Android
  // category picker. A query searches instead and leaves out the recent ones.
  Q_INVOKABLE QVariantList categories(QString const & query) const;
  Q_INVOKABLE void setField(int id, QString const & value);
  // An error message for an invalid value, or an empty string.
  Q_INVOKABLE QString fieldError(int id, QString const & value) const;
  Q_INVOKABLE QString nameError(QString const & name) const;
  // A name in another language, an empty one removes it.
  Q_INVOKABLE void setLocalizedName(int code, QString const & name);
  // Languages for "Add a language" as {code, language}, without the ones already named.
  Q_INVOKABLE QVariantList otherLanguages() const;
  Q_INVOKABLE QString houseNumberError(QString const & houseNumber) const;
  // OSM values with their names for the SelfService field.
  Q_INVOKABLE QVariantList selfServiceValues() const;
  // Supported cuisines as {key, name}, by name; and the names of the ';' separated keys of a value.
  Q_INVOKABLE QVariantList cuisines() const;
  Q_INVOKABLE QString cuisineNames(QString const & value) const;
  // Saves the changes locally, false on error. The place page shows the edited place.
  Q_INVOKABLE bool save();
  // A note to OpenStreetMap volunteers about the saved place, uploaded with the edits.
  Q_INVOKABLE void createNote(QString const & note);
  // A note at a position without a place, when no category fits, like on Android.
  Q_INVOKABLE void createStandaloneNote(double lat, double lon, QString const & note);
  // Discards the local edits, or removes a place added here.
  Q_INVOKABLE void reset();
  // Reports the place as gone with a note, which needs a comment.
  Q_INVOKABLE void placeDoesntExist(QString const & comment);

signals:
  void changed();

private:
  std::unique_ptr<osm::EditableMapObject> m_object;
  bool m_valid = false;
  // A new place, from create().
  bool m_creating = false;
};
}  // namespace sailfish
