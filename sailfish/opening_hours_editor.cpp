#include "sailfish/opening_hours_editor.hpp"

#include "editor/opening_hours_ui.hpp"
#include "editor/ui2oh.hpp"

#include <QVariantMap>

#include <sstream>

namespace sailfish
{
namespace
{
using editor::ui::TimeTable;
using editor::ui::TimeTableSet;
using osmoh::HourMinutes;

int Minutes(osmoh::Time const & time)
{
  return static_cast<int>(time.GetHourMinutes().GetDurationCount());
}

osmoh::Timespan Span(int start, int end)
{
  return {HourMinutes::TMinutes(start), HourMinutes::TMinutes(end)};
}
}  // namespace

OpeningHoursEditor::OpeningHoursEditor(QObject * parent)
  : QObject(parent)
  , m_timetables(std::make_unique<TimeTableSet>())
{}

OpeningHoursEditor::~OpeningHoursEditor() = default;

QString OpeningHoursEditor::value() const
{
  if (!m_simple)
    return m_text;
  std::ostringstream rule;
  rule << editor::MakeOpeningHours(*m_timetables).GetRule();
  return QString::fromStdString(rule.str());
}

void OpeningHoursEditor::setValue(QString const & value)
{
  auto const text = value.trimmed();
  TimeTableSet timetables;
  // Empty starts from the default schedule, like on Android.
  m_simple = text.isEmpty() || editor::MakeTimeTableSet(osmoh::OpeningHours(text.toStdString()), timetables);
  *m_timetables = std::move(timetables);
  m_text = text;
  emit changed();
}

QVariantList OpeningHoursEditor::timetables() const
{
  QVariantList result;
  for (auto const & tt : *m_timetables)
  {
    QVariantList days;
    for (auto const day : tt.GetOpeningDays())
      days.append(static_cast<int>(day));
    QVariantList closed;
    for (auto const & span : tt.GetExcludeTime())
      closed.append(QVariantMap{{"start", Minutes(span.GetStart())}, {"end", Minutes(span.GetEnd())}});
    auto const & opening = tt.GetOpeningTime();
    result.append(QVariantMap{{"days", days},
                              {"allDay", tt.IsTwentyFourHours()},
                              {"open", Minutes(opening.GetStart())},
                              {"close", Minutes(opening.GetEnd())},
                              {"closed", closed},
                              {"canAddClosed", !tt.IsTwentyFourHours() && tt.CanAddExcludeTime()}});
  }
  return result;
}

bool OpeningHoursEditor::canAddTimetable() const
{
  return !m_timetables->GetUnhandledDays().empty();
}

void OpeningHoursEditor::setDay(int index, int day, bool on)
{
  if (index < 0 || static_cast<size_t>(index) >= m_timetables->Size())
    return;
  auto tt = m_timetables->Get(static_cast<size_t>(index));
  auto const weekday = static_cast<osmoh::Weekday>(day);
  // A schedule keeps at least one day; a day taken here leaves the other schedules.
  if (on)
    tt.AddWorkingDay(weekday);
  else if (!tt.RemoveWorkingDay(weekday))
    return;
  // Another schedule losing its last day keeps it.
  tt.Commit();
  emit changed();
}

void OpeningHoursEditor::setAllDay(int index, bool on)
{
  if (index < 0 || static_cast<size_t>(index) >= m_timetables->Size())
    return;
  auto tt = m_timetables->Get(static_cast<size_t>(index));
  tt.SetTwentyFourHours(on);
  tt.Commit();
  emit changed();
}

void OpeningHoursEditor::setOpeningTime(int index, int open, int close)
{
  if (index < 0 || static_cast<size_t>(index) >= m_timetables->Size())
    return;
  auto tt = m_timetables->Get(static_cast<size_t>(index));
  if (tt.SetOpeningTime(Span(open, close)))
    tt.Commit();
  emit changed();
}

void OpeningHoursEditor::addClosed(int index)
{
  if (index < 0 || static_cast<size_t>(index) >= m_timetables->Size())
    return;
  auto tt = m_timetables->Get(static_cast<size_t>(index));
  if (tt.AddExcludeTime(tt.GetPredefinedExcludeTime()))
    tt.Commit();
  emit changed();
}

void OpeningHoursEditor::setClosed(int index, int closedIndex, int start, int end)
{
  if (index < 0 || static_cast<size_t>(index) >= m_timetables->Size() || closedIndex < 0)
    return;
  auto tt = m_timetables->Get(static_cast<size_t>(index));
  // Rejected when outside the opening time; the view shows the kept value again.
  if (tt.ReplaceExcludeTime(Span(start, end), static_cast<size_t>(closedIndex)))
    tt.Commit();
  emit changed();
}

void OpeningHoursEditor::removeClosed(int index, int closedIndex)
{
  if (index < 0 || static_cast<size_t>(index) >= m_timetables->Size() || closedIndex < 0)
    return;
  auto tt = m_timetables->Get(static_cast<size_t>(index));
  if (tt.RemoveExcludeTime(static_cast<size_t>(closedIndex)))
    tt.Commit();
  emit changed();
}

void OpeningHoursEditor::addTimetable()
{
  if (m_timetables->Append(m_timetables->GetComplementTimeTable()))
    emit changed();
}

bool OpeningHoursEditor::isValid(QString const & value) const
{
  auto const text = value.trimmed();
  return text.isEmpty() || osmoh::OpeningHours(text.toStdString()).IsValid();
}

void OpeningHoursEditor::removeTimetable(int index)
{
  if (index >= 0 && m_timetables->Remove(static_cast<size_t>(index)))
    emit changed();
}
}  // namespace sailfish
