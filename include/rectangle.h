#pragma once

typedef struct segment {
  int offset;
  unsigned size;
} segment_t __attribute__ ((aligned(8)));

typedef struct rect {
  segment_t x;
  segment_t y;
} rect_t;

int clip_segment(segment_t *s0, segment_t *s1);

/* get_intersection_regions is an api for getting intersection regions by applying
 * boundary rectangle r1 on the subject rectangle r0.
 *
 * intersection_regions_t is in essence a 3 x 3 matrix, with elements composed
 * of rectangle objects plus the mark that this rect exists in intersection
 * 
 *  x-------x-------x-------x
 *  |       |       |       |
 *  | [0,0] | [0,1] | [0,2] |
 *  |       |       |       |
 *  x-------x-------x-------x
 *  |       |       |       |
 *  | [1,0] | [1,1] | [1,2] |
 *  |       |       |       |
 *  x-------x-------x-------x
 *  |       |       |       |
 *  | [2,0] | [2,1] | [2,2] |
 *  |       |       |       |
 *  x-------x-------x-------x
 *
 *  in this 3 x 3 matrix, cell with coordinates x = 1, y = 1 is an intersection
 *  of r0 and r1 and all the cells around it are possible regions that are 
 *  outside of the intersection. For example if r1 completely contains r0, then
 *  only cell 1:1 exists and there are not outside regions, if r0 is much bigger
 *  than r1, like below:
 *    ------------------------
 *    |                      |
 *    |                      |
 *    |    ------            |
 *    |    |    |            |
 *    |    ------            |
 *    |                      |
 *    |                      |
 *    ------------------------, then
 * there would exist 8 outside regions, like:
 *    ------------------------
 *    |r0  |r1  |r2          |
 *    |    |    |            |
 *    |----------------------|
 *    |r3  |r4  |r5          |
 *    |----------------------|
 *    |r6  |r7  |r8          |
 *    |    |    |            |
 *    ------------------------
 * where r0, r1, r2, r3, r5-r8 regions are outter rectangles,
 * that exist after applying boundary
 *
 *
 * In case of some partial intersection like:
 *    ------------------------
 *    |                      |
 *    -----------            |
 *    |         |            |
 *    |         |            |
 *    -----------            |
 *    |                      |
 *    |                      |
 *    ------------------------, 
 * there would exist 8 outside regions, like:
 *    ------------------------
 *  r0|r1       |r2          |
 *    -----------------------|
 *  r3|r4       |r5          |
 *    |         |            |
 *    -----------------------|
 *  r6|r7       |r8          |
 *    |         |            |
 *    ------------------------, 
 * you can see, that after applying boundary, there are no left-side outter
 * rectangles, because r0 and r1 did not overlap from left-side
 */

typedef struct intersection_region {
  rect_t r;
  char exists;
} intersection_region_t;

typedef struct intersection_regions {
  intersection_region_t rgs[9];
} intersection_regions_t;

#define RGN(x, y) (&(rs->rgs[y * 3 + x]))
#define RCT(x, y) (&(rs->rgs[y * 3 + x].r))

int get_intersection_regions(rect_t *r0, rect_t *r1, intersection_regions_t *rs);


