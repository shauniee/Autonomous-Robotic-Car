#ifndef ROBOT_NAVIGATION_FIXTURES_H
#define ROBOT_NAVIGATION_FIXTURES_H
#include "../../app_program/robot_navigation.h"
RobotNavigationConfig robot_navigation_fixture_config(void);
bool robot_navigation_fixture_context(void);
bool robot_navigation_fixture_windows(void);
bool robot_navigation_fixture_actions(void);
bool robot_navigation_fixture_reacquire(void);
bool robot_navigation_fixture_stop(void);
void robot_navigation_test(void);
#endif
