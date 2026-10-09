#include "robot_navigation.h"
#include "subsystem2/subsystem2.h"
#include "subsystem3/subsystem3.h"
#if defined(__has_include)
#if __has_include("navigation_config.h")
#include "navigation_config.h"
#define HAVE_NAVIGATION_CONFIG 1
#endif
#endif
#ifndef HAVE_NAVIGATION_CONFIG
static const S2NavigationConfig robot_motion_configuration = {0};
static const S3BarcodeConfig robot_barcode_configuration = {0};
static const RobotNavigationConfig robot_route_configuration = {0};
#endif

bool robot_navigation_load(RobotNavigationConfig *config,
    S2NavigationConfig *motion, S3BarcodeConfig *barcode)
{
    *config = robot_route_configuration;
    *motion = robot_motion_configuration;
    *barcode = robot_barcode_configuration;
    if (!robot_navigation_config_valid(config, motion) ||
        !s3_barcode_config_valid(barcode) ||
        (barcode->left_polarity != motion->left_polarity) ||
        (barcode->right_polarity != motion->right_polarity))
    {
        return false;
    }
    return s2_navigation_configure(motion) && s3_barcode_configure(barcode);
}
