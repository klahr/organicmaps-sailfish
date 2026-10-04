#include "sailfish/bookmark_editor.hpp"

#include "sailfish/app_info.hpp"
#include "sailfish/bookmarks_model.hpp"
#include "sailfish/framework_access.hpp"

#include "map/bookmark_manager.hpp"
#include "map/framework.hpp"

#include "kml/type_utils.hpp"
#include "kml/types.hpp"

#include <algorithm>
#include <iterator>

namespace sailfish
{
namespace
{
int PresetIndex(dp::Color color)
{
  auto const & presets = kml::kOrderedPredefinedColors;
  auto const it = std::find_if(presets.begin(), presets.end(), [color](kml::PredefinedColor preset)
  { return kml::ColorFromPredefinedColor(preset).GetRGBA() == color.GetRGBA(); });
  return it != presets.end() ? static_cast<int>(std::distance(presets.begin(), it)) : -1;
}
}  // namespace

BookmarkEditor::BookmarkEditor(QObject * parent) : QObject(parent), m_framework(GetFramework()) {}

void BookmarkEditor::loadBookmark(quint64 id)
{
  auto const * bookmark = m_framework.GetBookmarkManager().GetBookmark(id);
  if (!bookmark)
    return;
  m_isTrack = false;
  m_id = id;
  m_name = QString::fromStdString(bookmark->GetPreferredName());
  m_description = QString::fromStdString(bookmark->GetDescription());
  m_colorIndex = PresetIndex(bookmark->GetColorForRendering());
  LoadCategories(bookmark->GetGroupId());
}

void BookmarkEditor::loadTrack(quint64 id)
{
  auto const * track = m_framework.GetBookmarkManager().GetTrack(id);
  if (!track)
    return;
  m_isTrack = true;
  m_id = id;
  m_name = QString::fromStdString(track->GetName());
  m_description = QString::fromStdString(track->GetDescription());
  m_colorIndex = PresetIndex(track->GetColor(0));
  LoadCategories(track->GetGroupId());
}

bool BookmarkEditor::trackVisible() const
{
  auto const * track = m_isTrack ? m_framework.GetBookmarkManager().GetTrack(m_id) : nullptr;
  return track && track->IsVisible();
}

void BookmarkEditor::setTrackVisible(bool visible)
{
  if (!m_isTrack)
    return;
  m_framework.SetTrackVisibility(m_id, visible);
  emit loaded();
}

int BookmarkEditor::createCategory(QString const & name)
{
  auto const id = m_framework.GetBookmarkManager().CreateBookmarkCategory(name.trimmed().toStdString());
  LoadCategories(m_groupId);
  auto const it = std::find(m_categoryIds.begin(), m_categoryIds.end(), id);
  return it != m_categoryIds.end() ? static_cast<int>(std::distance(m_categoryIds.begin(), it)) : -1;
}

void BookmarkEditor::LoadCategories(uint64_t groupId)
{
  m_groupId = groupId;
  auto const & ids = m_framework.GetBookmarkManager().GetSortedBmGroupIdList();
  m_categoryIds.assign(ids.begin(), ids.end());
  emit loaded();
}

QStringList BookmarkEditor::colors() const
{
  return PresetColors();
}

int BookmarkEditor::categoryIndex() const
{
  auto const it = std::find(m_categoryIds.begin(), m_categoryIds.end(), m_groupId);
  return it != m_categoryIds.end() ? static_cast<int>(std::distance(m_categoryIds.begin(), it)) : -1;
}

QStringList BookmarkEditor::categories() const
{
  QStringList names;
  auto const & manager = m_framework.GetBookmarkManager();
  for (auto const id : m_categoryIds)
    names.append(QString::fromStdString(manager.GetCategoryName(id)));
  return names;
}

void BookmarkEditor::save(QString const & name, QString const & description, int colorIndex, int categoryIndex)
{
  auto & manager = m_framework.GetBookmarkManager();
  auto const newName = name.trimmed().toStdString();
  auto const newGroupId = categoryIndex >= 0 && categoryIndex < static_cast<int>(m_categoryIds.size())
                            ? m_categoryIds[static_cast<size_t>(categoryIndex)]
                            : m_groupId;
  // The pickers of the other platforms store the presets as custom colors too.
  bool const colorChanged = colorIndex >= 0 && colorIndex != m_colorIndex;
  auto const color =
      colorChanged ? kml::ColorFromPredefinedColor(kml::kOrderedPredefinedColors[colorIndex]) : dp::Color();
  {
    auto session = manager.GetEditSession();
    if (m_isTrack)
    {
      auto const * track = manager.GetTrack(m_id);
      if (!track)
        return;
      auto data = track->GetData();
      kml::SetDefaultStr(data.m_name, newName);
      kml::SetDefaultStr(data.m_description, description.toStdString());
      session.UpdateTrack(m_id, data);
      if (colorChanged)
        session.ChangeTrackColor(m_id, color);
      if (newGroupId != m_groupId)
        session.MoveTrack(m_id, m_groupId, newGroupId);
    }
    else
    {
      auto const * bookmark = manager.GetBookmark(m_id);
      if (!bookmark)
        return;
      auto data = bookmark->GetData();
      // Like Android: an unchanged name stays the feature name instead of becoming a custom one.
      if (bookmark->GetPreferredName() != newName)
        kml::SetDefaultStr(data.m_customName, newName);
      kml::SetDefaultStr(data.m_description, description.toStdString());
      if (colorChanged)
        data.m_color = kml::MakeCustomBookmarkColorData(color);
      session.UpdateBookmark(m_id, data);
      if (newGroupId != m_groupId)
        session.MoveBookmark(m_id, m_groupId, newGroupId);
    }
  }
  if (IsSelected())
    m_framework.UpdatePlacePageInfoForCurrentSelection();
}

void BookmarkEditor::remove()
{
  auto & manager = m_framework.GetBookmarkManager();
  bool const selected = IsSelected();
  if (m_isTrack)
    manager.GetEditSession().DeleteTrack(m_id);
  else
    manager.GetEditSession().DeleteBookmark(m_id);
  if (selected)
    m_framework.DeactivateMapSelection();
}

bool BookmarkEditor::IsSelected() const
{
  if (!m_framework.HasPlacePageInfo())
    return false;
  auto const & info = m_framework.GetCurrentPlacePageInfo();
  return (m_isTrack ? info.GetTrackId() : info.GetBookmarkId()) == m_id;
}
}  // namespace sailfish
