#include <rectangle.h>
#include <string.h>


/* r0       - rectangle to clip
 * r1       - boundary rectangle 
 * our_rect - rectangle, that is the result of applying 
 *          - r1 boundary to r0 rect
 * return value - '-1', rectangle is clipped away
 *                ' 0', rectangle is not clipped
 *                ' 1', rectangle is partially clipped
 */
// static int clip_rect(
//     rect_t *r0, 
//     rect_t *r1, 
//     rect_t *out_i, 
//     intersection_type *out_t
//     )
// {
//   int partial_x, partial_y;
//   rect_t tmp = *r0;
//   partial_x = clip_segment(&tmp->x, &r1->x);
//   if (partial_x < 0)
//     return -1;
// 
//   partial_y = clip_segment(&tmp->y, &r1->y);
//   if (partial_y < 0)
//     return -1;
// 
//   *out_t = tmp;
//   return (partial_x + partial_y) ? 1 : 0;
// }

int clip_segment(segment_t *s0, segment_t *s1) 
{
  /*
   * x------------------x
   * .       .          .
   * .      (op)        .
   * .       .          .
   * .       x-----------------x
   * .       .          .
   * .       =          .
   * .       .          .
   * .       x----------x
   * .       .          .
   */
  if (s0->offset + s0->size > s1->offset 
   || s1->offset + s1->size < s0->offset)
    return -1;

  if (s0->offset > s1->offset) {
    if (s0->offset + s0->size > s1->offset + s1->size)
      s0->size = s1->offset + s1->size - s0->offset;
    return 1;
  }
  if (s0->offset < s1->offset) {
    s0->size -= (s1->offset - s0->offset);
    s0->offset = s1->offset;
    return 1;
  }
  
  return 0;
}



int get_intersection_regions(rect_t *r0, rect_t *r1, intersection_regions_t *rs)
{ 
  int r0x0, r0x1, r0y0, r0y1, r1x0, r1x1, r1y0, r1y1;

  r0x0 = r0->x.offset;
  r0y0 = r0->y.offset;
  r1x0 = r1->x.offset;
  r1y0 = r1->y.offset;
  r0x1 = r0x0 + (int)r0->x.size;
  r0y1 = r0y0 + (int)r0->y.size;
  r1x1 = r1x0 + (int)r1->x.size;
  r1y1 = r1y0 + (int)r1->y.size;

  if (r0x1 <= r1x0 || r1x1 <= r0x0 || r0y1 <= r1y0 || r1y1 <= r0y0)
    // no intersection
    return 0;

  memset(rs, 0, sizeof(*rs));
  RGN(1, 1)->exists = 1;
  RCT(1, 1)->x = r0->x;
  RCT(1, 1)->y = r0->y;
  RCT(1, 0)->x = r0->x;
  RCT(1, 2)->x = r0->x;
  RCT(0, 1)->y = RCT(2, 1)->y = r0->y;

  // Check interection from the left size
  if (r0x0 < r1x0) {
  /*  
   *  x---x---x---x
   *  | x |   |   |
   *  x---x---x---x
   *  | x | x |   |
   *  x---x---x---x
   *  | x |   |   |
   *  x---x---x---x
   */
    RCT(0, 0)->x.offset = RCT(0, 1)->x.offset = RCT(0, 2)->x.offset = r0x0;
    RCT(0, 0)->x.size   = RCT(0, 1)->x.size   = RCT(0, 2)->x.size = r1x0 - r0x0;
    RGN(0, 0)->exists   = RGN(0, 1)->exists   = RGN(0, 2)->exists = 1;

    RCT(1, 1)->x.offset += RCT(0, 0)->x.size;
    RCT(1, 1)->x.size   -= RCT(0, 0)->x.size;
  }

  // Check interection from the right size
  if (r0x1 > r1x1) {
  /*  
   *  x---x---x---x
   *  |   |   | x |
   *  x---x---x---x
   *  |   | x | x |
   *  x---x---x---x
   *  |   |   | x |
   *  x---x---x---x
   */
    RCT(2, 0)->x.offset = RCT(2, 1)->x.offset = RCT(2, 2)->x.offset = r1x1;
    RCT(2, 0)->x.size   = RCT(2, 1)->x.size   = RCT(2, 2)->x.size   = r0x1 - r1x1;
    RGN(2, 0)->exists   = RGN(2, 1)->exists   = RGN(2, 2)->exists = 1;

    RCT(1, 1)->x.size   -= RCT(2, 0)->x.size;
  }

  // Check interection from the top
  if (r0y0 < r1y0) {
  /*  
   *  x---x---x---x   x---x---x---x   x---x---x---x   x---x---x---x
   *  |   | x |   |   | x | x |   |   |   | x | x |   | x | x | x |
   *  x---x---x---x   x---x---x---x   x---x---x---x   x---x---x---x
   *  |   | x |   |   | x | x |   |   |   | x | x |   | x | x | x |
   *  x---x---x---x   x---x---x---x   x---x---x---x   x---x---x---x
   *  |   |   |   |   | x | x |   |   |   |   | x |   | x |   | x |
   *  x---x---x---x   x---x---x---x   x---x---x---x   x---x---x---x
   */
    RCT(0, 0)->y.offset = RCT(1, 0)->y.offset = RCT(2, 0)->y.offset = r0y0;
    RCT(0, 0)->y.size   = RCT(1, 0)->y.size   = RCT(2, 0)->y.size = r1y0 - r0y0;

    RGN(1, 0)->exists = 1;

    RCT(1, 1)->y.offset += RCT(0, 0)->y.size;
    RCT(1, 1)->y.size   -= RCT(0, 0)->y.size;
  }
  else {
    RGN(0, 0)->exists = RGN(1, 0)->exists = RGN(2, 0)->exists = 0;
  }


  // Check interection from the bottom
  if (r0y1 > r1y1) {
  /*  
   *  x---x---x---x   x---x---x---x   x---x---x---x   x---x---x---x
   *  |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
   *  x---x---x---x   x---x---x---x   x---x---x---x   x---x---x---x
   *  |   | x |   |   | x | x |   |   |   | x | x |   | x | x | x |
   *  x---x---x---x   x---x---x---x   x---x---x---x   x---x---x---x
   *  |   | x |   |   | x | x |   |   |   | x | x |   | x | x | x |
   *  x---x---x---x   x---x---x---x   x---x---x---x   x---x---x---x
   */
    RCT(0, 2)->y.offset = RCT(1, 2)->y.offset = RCT(2, 2)->y.offset = r1y1;
    RCT(0, 2)->y.size   = RCT(1, 2)->y.size   = RCT(2, 2)->y.size   = r0y1 - r1y1;

    RCT(1, 1)->y.size  -= RCT(1, 2)->y.size;
    RGN(1, 2)->exists   = 1;
  }
  else {
    RGN(0, 2)->exists = RGN(1, 2)->exists = RGN(2, 2)->exists = 0;
  }

  return 1;
}

