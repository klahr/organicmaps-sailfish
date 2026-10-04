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

void OpeningHoursEditor::Edit(int index, std::function<bool(TimeTable &)> const & change)
{
  if (index < 0 || static_cast<size_t>(index) >= m_timetables->Size())
    return;
  auto tt = m_timetables->Get(static_cast<size_t>(index));
  // Commit is refused when the change would break another schedule, e.g. take its last day.
  if (change(tt))
    tt.Commit();
  emit changed();
}

void OpeningHoursEditor::setDay(int index, int day, bool on)
{
  // A schedule keeps at least one day; a day taken here leaves the other schedules.
  Edit(index, [weekday = static_cast<osmoh::Weekday>(day), on](TimeTable & tt)
  {
    if (!on)
      return tt.RemoveWorkingDay(weekday);
    tt.AddWorkingDay(weekday);
    return true;
  });
}

void OpeningHoursEditor::setAllDay(int index, bool on)
{
  Edit(index, [on](TimeTable & tt)
  {
    tt.SetTwentyFourHours(on);
    return true;
  });
}

void OpeningHoursEditor::setOpeningTime(int index, int open, int close)
{
  Edit(index, [open, close](TimeTable & tt) { return tt.SetOpeningTime(Span(open, close)); });
}

void OpeningHoursEditor::addClosed(int index)
{
  Edit(index, [](TimeTable & tt) { return tt.AddExcludeTime(tt.GetPredefinedExcludeTime()); });
}

void OpeningHoursEditor::setClosed(int index, int closedIndex, int start, int end)
{
  // Rejected when outside the opening time; the view shows the kept value again.
  Edit(index, [closedIndex, start, end](TimeTable & tt)
  { return closedIndex >= 0 && tt.ReplaceExcludeTime(Span(start, end), static_cast<size_t>(closedIndex)); });
}

void OpeningHoursEditor::removeClosed(int index, int closedIndex)
{
  Edit(index, [closedIndex](TimeTable & tt)
  { return closedIndex >= 0 && tt.RemoveExcludeTime(static_cast<size_t>(closedIndex)); });
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
