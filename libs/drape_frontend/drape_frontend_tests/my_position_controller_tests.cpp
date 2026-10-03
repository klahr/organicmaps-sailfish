#include "testing/testing.hpp"

#include "drape_frontend/drape_frontend_tests/visual_params_fixture.hpp"
#include "drape_frontend/my_position_controller.hpp"

#include "geometry/screenbase.hpp"

namespace my_position_controller_tests
{
using df::test_support::VisualParamsFixture;

UNIT_CLASS_TEST(VisualParamsFixture, StopFollowModesAndFirstFix)
{
  ScreenBase screen;
  screen.OnSize(0, 0, 800, 600);
  screen.SetFromRect(m2::AnyRectD(m2::RectD(-100, -100, 100, 100)));
  location::GpsInfo fix;
  fix.m_latitude = 47;
  fix.m_longitude = 8;
  fix.m_horizontalAccuracy = 5;
  fix.m_timestamp = 1;

  for (auto mode : {location::Follow, location::FollowAndRotate, location::NotFollow})
  {
    df::MyPositionController controller(
        {mode, 0.0 /* timeInBackground */, {}, false /* isRoutingActive */, false /* isAutozoomEnabled */, {}},
        nullptr);
    controller.OnUpdateScreen(screen);
    controller.OnLocationUpdate(fix, false /* isNavigable */, screen);
    TEST_EQUAL(controller.GetCurrentMode(), mode, ());
    controller.StopLocationFollow();
    TEST_EQUAL(controller.GetCurrentMode(), location::NotFollow, ());
  }

  df::MyPositionController pending({location::Follow,
                                    0.0 /* timeInBackground */,
                                    {},
                                    false /* isRoutingActive */,
                                    false /* isAutozoomEnabled */,
                                    {}},
                                   nullptr);
  pending.OnUpdateScreen(screen);
  TEST_EQUAL(pending.GetCurrentMode(), location::PendingPosition, ());
  pending.StopLocationFollow();
  pending.OnLocationUpdate(fix, false /* isNavigable */, screen);
  TEST_EQUAL(pending.GetCurrentMode(), location::NotFollow, ());
}

UNIT_CLASS_TEST(VisualParamsFixture, LastKnownPositionKeepsPendingMode)
{
  ScreenBase screen;
  screen.OnSize(0, 0, 800, 600);
  screen.SetFromRect(m2::AnyRectD(m2::RectD(-100, -100, 100, 100)));
  location::GpsInfo fix;
  fix.m_latitude = 47;
  fix.m_longitude = 8;
  fix.m_horizontalAccuracy = 5;
  fix.m_timestamp = 1;
  location::GpsInfo lastKnown = fix;
  lastKnown.m_isObsolete = true;

  df::MyPositionController controller({location::PendingPosition,
                                       0.0 /* timeInBackground */,
                                       {},
                                       false /* isRoutingActive */,
                                       false /* isAutozoomEnabled */,
                                       {}},
                                      nullptr);
  controller.OnUpdateScreen(screen);
  controller.OnLocationUpdate(lastKnown, false /* isNavigable */, screen);
  TEST_EQUAL(controller.GetCurrentMode(), location::PendingPosition, ());
  TEST(controller.IsWaitingForLocation(), ());

  fix.m_timestamp = 2;
  controller.OnLocationUpdate(fix, false /* isNavigable */, screen);
  TEST_EQUAL(controller.GetCurrentMode(), location::Follow, ());

  // Showing a place on the map before the first fix keeps the map there.
  df::MyPositionController scrolled({location::PendingPosition,
                                     0.0 /* timeInBackground */,
                                     {},
                                     false /* isRoutingActive */,
                                     false /* isAutozoomEnabled */,
                                     {}},
                                    nullptr);
  scrolled.OnUpdateScreen(screen);
  scrolled.OnLocationUpdate(lastKnown, false /* isNavigable */, screen);
  scrolled.StopLocationFollow();
  scrolled.OnLocationUpdate(fix, false /* isNavigable */, screen);
  TEST_EQUAL(scrolled.GetCurrentMode(), location::NotFollow, ());
}
}  // namespace my_position_controller_tests
