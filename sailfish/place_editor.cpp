#include "sailfish/place_editor.hpp"

#include "sailfish/app_info.hpp"
#include "sailfish/framework_access.hpp"

#include "map/framework.hpp"

#include "editor/osm_editor.hpp"

#include "indexer/classificator.hpp"
#include "indexer/editable_map_object.hpp"
#include "indexer/validate_and_format_contacts.hpp"

#include "opening_hours/opening_hours.hpp"

#include "platform/localization.hpp"

#include "coding/string_utf8_multilang.hpp"

#include <QVariantMap>

#include <algorithm>
#include <optional>

namespace sailfish
{
namespace
{
using feature::Metadata;

QString FromUtf8View(std::string_view s)
{
  return QString::fromUtf8(s.data(), static_cast<int>(s.size()));
}

struct FieldInfo
{
  PlaceEditor::Kind m_kind;
  // A UI string key, or a brand name when not localized.
  char const * m_label;
  bool m_localized;
  // Sailfish.Silica input method hint: "url", "email", "phone" or "".
  char const * m_inputHint;
  // Shown for an invalid value.
  char const * m_error;
};

// The detail fields of the Android editor, in its order, see EditorFragment.
std::optional<FieldInfo> GetFieldInfo(Metadata::EType id)
{
  using K = PlaceEditor::Kind;
  switch (id)
  {
  case Metadata::FMD_OPEN_HOURS: return FieldInfo{K::Text, "editor_time_title", true, "", nullptr};
  case Metadata::FMD_PHONE_NUMBER: return FieldInfo{K::Text, "phone", true, "phone", "error_enter_correct_phone"};
  case Metadata::FMD_WEBSITE: return FieldInfo{K::Text, "website", true, "url", "error_enter_correct_web"};
  case Metadata::FMD_WEBSITE_MENU: return FieldInfo{K::Text, "website_menu", true, "url", "error_enter_correct_web"};
  case Metadata::FMD_EMAIL: return FieldInfo{K::Text, "email", true, "email", "error_enter_correct_email"};
  case Metadata::FMD_CONTACT_FACEBOOK:
    return FieldInfo{K::Text, "Facebook", false, "url", "error_enter_correct_facebook_page"};
  case Metadata::FMD_CONTACT_INSTAGRAM:
    return FieldInfo{K::Text, "Instagram", false, "url", "error_enter_correct_instagram_page"};
  case Metadata::FMD_CONTACT_TWITTER:
    return FieldInfo{K::Text, "X (Twitter)", false, "url", "error_enter_correct_twitter_page"};
  case Metadata::FMD_CONTACT_VK: return FieldInfo{K::Text, "VK", false, "url", "error_enter_correct_vk_page"};
  case Metadata::FMD_CONTACT_LINE: return FieldInfo{K::Text, "LINE", false, "url", "error_enter_correct_line_page"};
  case Metadata::FMD_OPERATOR: return FieldInfo{K::Text, "editor_operator", true, "", nullptr};
  case Metadata::FMD_LEVEL: return FieldInfo{K::Text, "level", true, "", nullptr};
  case Metadata::FMD_POSTCODE: return FieldInfo{K::Text, "editor_zip_code", true, "", "error_enter_correct_zip_code"};
  case Metadata::FMD_INTERNET: return FieldInfo{K::Wifi, "category_wifi", true, "", nullptr};
  case Metadata::FMD_SELF_SERVICE: return FieldInfo{K::SelfService, "self_service", true, "", nullptr};
  // TODO: Cuisine needs a picker of the supported values.
  default: return {};
  }
}
}  // namespace

PlaceEditor::PlaceEditor(QObject * parent) : QObject(parent), m_object(std::make_unique<osm::EditableMapObject>()) {}

PlaceEditor::~PlaceEditor() = default;

void PlaceEditor::start()
{
  auto & framework = GetFramework();
  m_valid = framework.HasPlacePageInfo() &&
            framework.GetEditableMapObject(framework.GetCurrentPlacePageInfo().GetID(), *m_object);
  emit changed();
}

QString PlaceEditor::category() const
{
  if (!m_valid)
    return {};
  auto types = m_object->GetTypes();
  types.SortBySpec();
  return QString::fromStdString(platform::GetLocalizedTypeName(classif().GetReadableObjectName(types.GetBestType())));
}

bool PlaceEditor::nameEditable() const
{
  return m_valid && m_object->IsNameEditable();
}

QString PlaceEditor::name() const
{
  return FromUtf8View(m_object->GetNameMultilang().Get(StringUtf8Multilang::kDefaultCode));
}

void PlaceEditor::setName(QString const & name)
{
  m_object->SetName(name.trimmed().toStdString(), StringUtf8Multilang::kDefaultCode);
}

bool PlaceEditor::addressEditable() const
{
  return m_valid && m_object->IsAddressEditable();
}

QString PlaceEditor::street() const
{
  return QString::fromStdString(m_object->GetStreet().m_defaultName);
}

void PlaceEditor::setStreet(QString const & street)
{
  auto const name = street.trimmed().toStdString();
  for (auto const & nearby : m_object->GetNearbyStreets())
  {
    if (nearby.m_defaultName == name)
    {
      m_object->SetStreet(nearby);
      return;
    }
  }
  m_object->SetStreet({name, {}});
}

QStringList PlaceEditor::nearbyStreets() const
{
  QStringList streets;
  for (auto const & street : m_object->GetNearbyStreets())
    streets.append(QString::fromStdString(street.m_defaultName));
  return streets;
}

QString PlaceEditor::houseNumber() const
{
  return QString::fromStdString(m_object->GetHouseNumber());
}

void PlaceEditor::setHouseNumber(QString const & houseNumber)
{
  m_object->SetHouseNumber(houseNumber.trimmed().toStdString());
}

QVariantList PlaceEditor::fields() const
{
  QVariantList fields;
  if (!m_valid)
    return fields;

  auto ids = m_object->GetEditableProperties();
  // Android shows the postcode with the address.
  if (m_object->IsAddressEditable() && std::find(ids.begin(), ids.end(), Metadata::FMD_POSTCODE) == ids.end())
    ids.insert(ids.begin(), Metadata::FMD_POSTCODE);

  for (auto const id : ids)
  {
    auto const info = GetFieldInfo(id);
    if (!info)
      continue;

    QString value;
    if (id == Metadata::FMD_INTERNET)
      value = m_object->GetInternet() == feature::Internet::Wlan ? "yes" : "";
    else if (osm::isSocialContactTag(id))
    {
      // Same as Editor.nativeGetMetadata on Android: a page name, or the full URL of a link.
      auto const v = m_object->GetMetadata(id);
      value = v.find('/') == std::string_view::npos ? FromUtf8View(v)
                                                    : QString::fromStdString(osm::socialContactToURL(id, v));
    }
    else
      value = FromUtf8View(m_object->GetMetadata(id));

    fields.append(QVariantMap{{"id", static_cast<int>(id)},
                              {"kind", info->m_kind},
                              {"label", info->m_localized ? Localized(info->m_label) : info->m_label},
                              {"value", value},
                              {"inputHint", info->m_inputHint}});
  }
  return fields;
}

void PlaceEditor::setField(int id, QString const & value)
{
  auto const metaId = static_cast<Metadata::EType>(id);
  auto v = value.trimmed().toStdString();
  switch (metaId)
  {
  case Metadata::FMD_OPEN_HOURS: m_object->SetOpeningHours(std::move(v)); break;
  case Metadata::FMD_INTERNET:
  {
    // Keeps other internet values when the switch is not changed, like on Android.
    bool const wifi = !v.empty();
    if (wifi != (m_object->GetInternet() == feature::Internet::Wlan))
      m_object->SetInternet(wifi ? feature::Internet::Wlan : feature::Internet::Unknown);
    break;
  }
  default: m_object->SetMetadata(metaId, std::move(v)); break;
  }
}

QString PlaceEditor::fieldError(int id, QString const & value) const
{
  auto const metaId = static_cast<Metadata::EType>(id);
  auto const v = value.trimmed().toStdString();
  if (v.empty())
    return {};
  if (metaId == Metadata::FMD_OPEN_HOURS)
    return osmoh::OpeningHours(v).IsValid() ? QString() : tr("Invalid opening hours");
  if (osm::EditableMapObject::IsValidMetadata(metaId, v))
    return {};
  auto const info = GetFieldInfo(metaId);
  return info && info->m_error ? Localized(info->m_error) : tr("Invalid value");
}

QString PlaceEditor::nameError(QString const & name) const
{
  return osm::EditableMapObject::ValidateName(name.trimmed().toStdString()) ? QString()
                                                                            : Localized("error_enter_correct_name");
}

QString PlaceEditor::houseNumberError(QString const & houseNumber) const
{
  return osm::EditableMapObject::ValidateHouseNumber(houseNumber.trimmed().toStdString())
           ? QString()
           : Localized("error_enter_correct_house_number");
}

QVariantList PlaceEditor::selfServiceValues() const
{
  // Same values as the Android SelfServiceAdapter, with their type names.
  QVariantList values;
  for (char const * value : {"yes", "only", "partially", "no"})
  {
    values.append(QVariantMap{
        {"value", value},
        {"name", QString::fromStdString(platform::GetLocalizedTypeName(std::string("self_service-") + value))}});
  }
  return values;
}

bool PlaceEditor::save()
{
  if (!m_valid)
    return false;
  switch (GetFramework().SaveEditedMapObject(*m_object))
  {
  case osm::Editor::SaveResult::NothingWasChanged:
  case osm::Editor::SaveResult::SavedSuccessfully: return true;
  case osm::Editor::SaveResult::NoFreeSpaceError:
  case osm::Editor::SaveResult::NoUnderlyingMapError:
  case osm::Editor::SaveResult::SavingError: return false;
  }
  return false;
}

bool PlaceEditor::canReset() const
{
  if (!m_valid)
    return false;
  auto const & editor = osm::Editor::Instance();
  auto const & id = m_object->GetID();
  auto const status = editor.GetFeatureStatus(id);
  return (status == FeatureStatus::Modified || status == FeatureStatus::Created) &&
         !editor.IsFeatureUploaded(id.m_mwmId, id.m_index);
}

void PlaceEditor::reset()
{
  if (m_valid)
    GetFramework().RollBackChanges(m_object->GetID());
}
}  // namespace sailfish
