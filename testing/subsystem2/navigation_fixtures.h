#ifndef NAVIGATION_FIXTURES_H
#define NAVIGATION_FIXTURES_H
#include "../../app_program/subsystem2/navigation.h"
S2NavigationConfig navigation_fixture_config(void);
bool navigation_fixture_quadrature(void);
bool navigation_fixture_actions(void);
bool navigation_fixture_deadlines(void);
bool navigation_fixture_stall(void);
void subsystem2_navigation_test(void);
#endif
