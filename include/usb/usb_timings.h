#pragma once

/*
 * Signalling and other timing value accrording to USB 2.0 specification
 */

/*
 * Tdrstr - Time duration for reset port on ROOT hub = 50ms. See USB 2.0, chapters 7.1.7.5 and C1
 */
#define TDRSTR_MS 50

/*
 * Tdrstr - Time duration for reset port on hub = 10ms. See USB 2.0, chapters 7.1.7.5 and C1
 */
#define TDRST_MS 10
