#include "sailfish/place_editor.hpp"

#include "sailfish/app_info.hpp"
#include "sailfish/framework_access.hpp"

#include "map/framework.hpp"

#include "editor/new_feature_categories.hpp"
#include "editor/osm_editor.hpp"

#include "indexer/classificator.hpp"
#include "indexer/editable_map_object.hpp"
#include "indexer/validate_and_format_contacts.hpp"

#include "opening_hours/opening_hours.hpp"

#include "platform/localization.hpp"
#include "platform/preferred_languages.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "base/assert.hpp"
#include "base/stl_helpers.hpp"

#include "geometry/mercator.hpp"

#include <QVariantMap>

#include <algorithm>
#include <optional>

namespace sailfish
{
namespace
{
using feature::Metadata;

// Shared like on Android: the search index and the recent categories are built once.
osm::NewFeatureCategories & GetFeatureCategories()
{
  static osm::NewFeatureCategories categories = []
  {
    auto c = GetFramework().GetEditorCategories();
    c.AddLanguage(languages::GetCurrentNorm());
    c.AddLanguage("en");
    return c;
  }();
  return categories;
}

QString FromUtf8View(std::string_view s)
{
  return QString::fromUtf8(s.data(), static_cast<int>(s.size()));
}

struct FieldInfo
{
  PlaceEditor::Kind m_kind;
  PlaceEditor::Section m_section;
  // An icon in icons/editor, from the Android editor.
  char const * m_icon;
  // A UI string key, or a brand name when not localized.
  char const * m_label;
  bool m_localized;
  // Sailfish.Silica input method hint: "url", "email", "phone" or "".
  char const * m_inputHint;
  // Shown for an invalid value.
  char const * m_error;
};

// The fields of the Android editor in its order, see fragment_editor.xml.
// TODO: Cuisine needs a picker of the supported values.
Metadata::EType constexpr kFieldOrder[] = {Metadata::FMD_POSTCODE,
                                           Metadata::FMD_OPEN_HOURS,
                                           Metadata::FMD_OPERATOR,
                                           Metadata::FMD_WEBSITE,
                                           Metadata::FMD_WEBSITE_MENU,
                                           Metadata::FMD_PHONE_NUMBER,
                                           Metadata::FMD_EMAIL,
                                           Metadata::FMD_INTERNET,
                                           Metadata::FMD_SELF_SERVICE,
                                           Metadata::FMD_CONTACT_FACEBOOK,
                                           Metadata::FMD_CONTACT_INSTAGRAM,
                                           Metadata::FMD_CONTACT_TWITTER,
                                           Metadata::FMD_CONTACT_VK,
                                           Metadata::FMD_CONTACT_LINE};

FieldInfo GetFieldInfo(Metadata::EType id)
{
  using K = PlaceEditor::Kind;
  using S = PlaceEditor::Section;
  switch (id)
  {
  case Metadata::FMD_POSTCODE:
    return {K::Text, S::Address, "ic_address", "editor_zip_code", true, "", "error_enter_correct_zip_code"};
  case Metadata::FMD_OPEN_HOURS:
    return {K::Text, S::Details, "ic_operating_hours", "editor_time_title", true, "", nullptr};
  case Metadata::FMD_OPERATOR: return {K::Text, S::Details, "ic_operator", "editor_operator", true, "", nullptr};
  case Metadata::FMD_WEBSITE:
    return {K::Text, S::Details, "ic_website", "website", true, "url", "error_enter_correct_web"};
  case Metadata::FMD_WEBSITE_MENU:
    return {K::Text, S::Details, "ic_website_menu", "website_menu", true, "url", "error_enter_correct_web"};
  case Metadata::FMD_PHONE_NUMBER:
    return {K::Text, S::Details, "ic_phone", "phone", true, "phone", "error_enter_correct_phone"};
  case Metadata::FMD_EMAIL:
    return {K::Text, S::Details, "ic_email", "email", true, "email", "error_enter_correct_email"};
  case Metadata::FMD_INTERNET: return {K::Wifi, S::Details, "ic_wifi", "category_wifi", true, "", nullptr};
  case Metadata::FMD_SELF_SERVICE:
    return {K::SelfService, S::Details, "ic_self_service", "self_service", true, "", nullptr};
  case Metadata::FMD_CONTACT_FACEBOOK:
    return {K::Text, S::SocialMedia, "ic_facebook", "Facebook", false, "url", "error_enter_correct_facebook_page"};
  case Metadata::FMD_CONTACT_INSTAGRAM:
    return {K::Text, S::SocialMedia, "ic_instagram", "Instagram", false, "url", "error_enter_correct_instagram_page"};
  case Metadata::FMD_CONTACT_TWITTER:
    return {K::Text, S::SocialMedia, "ic_twitterx", "X (Twitter)", false, "url", "error_enter_correct_twitter_page"};
  case Metadata::FMD_CONTACT_VK:
    return {K::Text, S::SocialMedia, "ic_vk", "VK", false, "url", "error_enter_correct_vk_page"};
  case Metadata::FMD_CONTACT_LINE:
    return {K::Text, S::SocialMedia, "ic_line", "LINE", false, "url", "error_enter_correct_line_page"};
  default: UNREACHABLE();
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

bool PlaceEditor::create(QString const & type, double lat, double lon)
{
  auto const typeName = type.toStdString();
  m_valid = GetFramework().CreateMapObject(mercator::FromLatLon(lat, lon),
                                           classif().GetTypeByReadableObjectName(typeName), *m_object);
  if (m_valid)
    GetFeatureCategories().AddToRecentCategories(typeName);
  emit changed();
  return m_valid;
}

QVariantList PlaceEditor::categories(QString const & query) const
{
  auto const & categories = GetFeatureCategories();
  auto const toList = [](osm::NewFeatureCategories::TypeNames const & types, bool recent, bool sort)
  {
    std::vector<std::pair<QString, std::string>> named;
    named.reserve(types.size());
    for (auto const & type : types)
      named.emplace_back(QString::fromStdString(platform::GetLocalizedTypeName(type)), type);
    if (sort)
    {
      std::sort(named.begin(), named.end(),
                [](auto const & a, auto const & b) { return QString::localeAwareCompare(a.first, b.first) < 0; });
    }
    QVariantList list;
    for (auto const & [name, type] : named)
      list.append(QVariantMap{{"type", QString::fromStdString(type)}, {"name", name}, {"recent", recent}});
    return list;
  };

  auto const q = query.trimmed().toStdString();
  if (!q.empty())
    return toList(categories.Search(q), false /* recent */, true /* sort */);
  return toList(categories.GetRecentCategories(), true /* recent */, false /* sort */) +
         toList(categories.GetAllCreatableTypeNames(), false /* recent */, true /* sort */);
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

QVariantList PlaceEditor::localizedNames() const
{
  QVariantList names;
  auto const source = m_object->GetNamesDataSource();
  for (auto const & name : source.names)
  {
    if (name.m_code == StringUtf8Multilang::kDefaultCode)
      continue;
    names.append(QVariantMap{{"code", static_cast<int>(name.m_code)},
                             {"language", FromUtf8View(name.m_langName)},
                             {"value", QString::fromStdString(name.m_name)}});
  }
  return names;
}

void PlaceEditor::setLocalizedName(int code, QString const & name)
{
  m_object->SetName(name.trimmed().toStdString(), static_cast<int8_t>(code));
}

QVariantList PlaceEditor::otherLanguages() const
{
  auto const & names = m_object->GetNameMultilang();
  QVariantList languages;
  for (auto const & lang : StringUtf8Multilang::GetSupportedLanguages(false /* includeServiceLangs */))
  {
    auto const code = StringUtf8Multilang::GetLangIndex(lang.m_code);
    if (code == StringUtf8Multilang::kDefaultCode || names.Has(code))
      continue;
    languages.append(QVariantMap{{"code", static_cast<int>(code)}, {"language", FromUtf8View(lang.m_name)}});
  }
  return languages;
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

  auto const editable = m_object->GetEditableProperties();
  for (auto const id : kFieldOrder)
  {
    // Android shows the postcode with an editable address.
    bool const shown = id == Metadata::FMD_POSTCODE ? m_object->IsAddressEditable() : base::IsExist(editable, id);
    if (!shown)
      continue;
    auto const info = GetFieldInfo(id);

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
                              {"kind", info.m_kind},
                              {"section", info.m_section},
                              {"icon", QStringLiteral("../../icons/editor/%1.svg").arg(info.m_icon)},
                              {"label", info.m_localized ? Localized(info.m_label) : info.m_label},
                              {"value", value},
                              {"inputHint", info.m_inputHint}});
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
  return info.m_error ? Localized(info.m_error) : tr("Invalid value");
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

void PlaceEditor::createNote(QString const & note)
{
  auto const text = note.trimmed().toStdString();
  if (m_valid && !text.empty())
    GetFramework().CreateNote(*m_object, osm::Editor::NoteProblemType::General, text);
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
