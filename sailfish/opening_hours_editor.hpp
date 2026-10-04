#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

#include <memory>

// Nested namespaces spelled out for the Qt 5.6 moc.
namespace editor
{
namespace ui
{
class TimeTableSet;
}  // namespace ui
}  // namespace editor

namespace sailfish
{
// Opening hours as schedules of days, like the Android simple timetable editor. The core keeps the schedules
// valid: a day belongs to one schedule, and non-business hours lie within the opening time. Values that can't
// be shown as schedules are edited as text.
class OpeningHoursEditor : public QObject
{
  Q_OBJECT
  // The opening_hours value; setting it reads the schedules from it.
  Q_PROPERTY(QString value READ value WRITE setValue NOTIFY changed)
  // The value can be shown as schedules.
  Q_PROPERTY(bool simple READ simple NOTIFY changed)
  // Schedules as {days, allDay, open, close, closed, canAddClosed}: days are osmoh::Weekday values (Sunday is 1),
  // times are minutes since midnight and closed lists the non-business hours as {start, end}.
  Q_PROPERTY(QVariantList timetables READ timetables NOTIFY changed)
  // Some days are in no schedule yet.
  Q_PROPERTY(bool canAddTimetable READ canAddTimetable NOTIFY changed)

public:
  explicit OpeningHoursEditor(QObject * parent = nullptr);
  ~OpeningHoursEditor() override;

  QString value() const;
  void setValue(QString const & value);
  bool simple() const { return m_simple; }
  QVariantList timetables() const;
  bool canAddTimetable() const;

  Q_INVOKABLE void setDay(int index, int day, bool on);
  Q_INVOKABLE void setAllDay(int index, bool on);
  Q_INVOKABLE void setOpeningTime(int index, int open, int close);
  Q_INVOKABLE void addClosed(int index);
  Q_INVOKABLE void setClosed(int index, int closedIndex, int start, int end);
  Q_INVOKABLE void removeClosed(int index, int closedIndex);
  // A schedule for the days left, like "Add Schedule" on Android.
  Q_INVOKABLE void addTimetable();
  Q_INVOKABLE void removeTimetable(int index);
  // For text: empty, which removes the opening hours, or valid.
  Q_INVOKABLE bool isValid(QString const & value) const;

signals:
  void changed();

private:
  std::unique_ptr<editor::ui::TimeTableSet> m_timetables;
  bool m_simple = true;
  // The value when it isn't simple.
  QString m_text;
};
}  // namespace sailfish
