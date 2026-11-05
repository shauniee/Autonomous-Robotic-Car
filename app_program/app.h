/*
 *	app.h
 *	Definitions shared by the whole team.  Changed only by team agreement.
 *
 *	Anything that belongs to ONE subsystem goes in that subsystem's own
 *	header instead (app_program/<name>/<name>.h).
 */

#ifndef APP_H
#define APP_H

#include <tk/tkernel.h>		/* micro T-Kernel API (tk_xxx_yyy) */
#include <tm/tmonitor.h>	/* T-Monitor API: tm_printf() console output */
#include <bsp/libbsp.h>		/* Board helpers: gpio_set_pin/val, pwm_... */
#include "./common/robot_config.h"

/* ---------------------------------------------------------------------
 *  Pins: free for projects are GP0..GP22 and GP26..GP28.  GP0/GP1 are
 *  taken by UART0 when built with "make CRASH_REPORT=1" (see README).
 *  Never use GP23/24/25/29: they are wired to the Pico W radio.
 *  Claim every pin in docs/TEAM.md section 2 (Pins) before using it.
 * --------------------------------------------------------------------- */

/* ---------------------------------------------------------------------
 *  Task priorities: 1 is the highest; the kernel's USB console (3) and
 *  WiFi (4) tasks sit above every application task.  Each subsystem
 *  records its priorities in docs/TEAM.md section 4 and its own header.
 * --------------------------------------------------------------------- */

/* Create and start a task.  prc = TP_PRC1 (core 0), TP_PRC2 (core 1), or
   0 for "any core".  Defined in app_main.c. */
IMPORT ID app_start_task(FP entry, PRI pri, UW prc, const char *name);

/* ---------------------------------------------------------------------
 *  Every subsystem provides these.  usermain() calls ALL the _init()
 *  functions before starting Motion, Mission and I/O, so every
 *  kernel object exists before any task can use it.
 *    <name>_init()   create this subsystem's kernel objects
 *    Task entry points are declared by Motion, Mission and common/io_task.h
 *    <name>_test()   on-target tests, only in test builds (make TEST=1)
 * --------------------------------------------------------------------- */
IMPORT ER comms_init(void);
IMPORT ER motion_init(void);
IMPORT ER line_barcode_init(void);
IMPORT ER terrain_init(void);
IMPORT ER obstacle_init(void);

IMPORT ER motion_start(void);

#if APP_TEST
IMPORT void comms_test(void);
IMPORT void motion_test(void);
IMPORT void terrain_test(void);
#endif

/* Print a readable message if a kernel call failed (negative error code) */
#define CHECK(er, what)							\
	do {								\
		INT check_er_ = (INT)(er);	/* evaluate once */	\
		if (check_er_ < E_OK) {					\
			tm_printf((UB*)"ERROR %s: %d\n", (what), check_er_); \
		}							\
	} while (0)

#endif /* APP_H */
