#include <rectangle.h>
#include <stdlib.h>
#include <stdio.h>

#define ASSERT_VALUE_EQ(v1, v2, text) \
  if ((v1) != (v2)) { printf(text"\n"); exit(1); }

void test_rectangle(rect_t *r0, rect_t *r1, intersection_regions_t *r_exp, int retval_exp)
{
  intersection_regions_t r;
  intersection_region_t *rt0, *rt1;
  int retval;
  retval = get_intersection_regions(r0, r1, &r);
  ASSERT_VALUE_EQ(retval, retval_exp, "get_intersection_regions return value mismatch");
  if (!retval)
    return;
  int x, y;
  for (y = 0; y < 3; ++y) {
    for (x = 0; x < 3; ++x) {
      rt0 = &r.rgs[y * 3 + x];
      rt1 = &(r_exp->rgs[y * 3 + x]);
      ASSERT_VALUE_EQ(rt0->exists, rt1->exists, "get_intersection_regions exist mismatch");
      if (rt0->exists) {
        ASSERT_VALUE_EQ(rt0->r.x.offset, rt1->r.x.offset, "get_intersection_regions r.x.offset mismatch");
        ASSERT_VALUE_EQ(rt0->r.y.offset, rt1->r.y.offset, "get_intersection_regions r.y.offset mismatch");
        ASSERT_VALUE_EQ(rt0->r.x.size, rt1->r.x.size, "get_intersection_regions r.x.size mismatch");
        ASSERT_VALUE_EQ(rt0->r.y.size, rt1->r.y.size, "get_intersection_regions r.x.size mismatch");
      }
    }
  }
}

void set_iregion(intersection_region_t *r, int x, int y, unsigned int sx, unsigned int sy, char exists)
{
  r->r.x.offset = x;
  r->r.y.offset = y;
  r->r.x.size   = sx;
  r->r.y.size   = sy;
  r->exists = exists;
}

void fill_rect(rect_t *r, int x, int y, unsigned int sx, unsigned int sy) 
{ 
  r->x.offset = x; 
  r->y.offset = y; 
  r->x.size = sx;
  r->y.size = sy;
}

int main() 
{
  rect_t r0;
  rect_t r1;

  intersection_regions_t r;
  intersection_regions_t *rs = &r;

  // perfect match
  memset(rs, 0, sizeof(*rs));
  fill_rect(&r0, 0, 0, 20, 20);  
  fill_rect(&r1, 0, 0, 20, 20);  
  set_iregion(RGN(1, 1), 0, 0, 20, 20, 1);
  test_rectangle(&r0, &r1, &r, 1);

  // clip left
  memset(rs, 0, sizeof(*rs));
  fill_rect(&r0, -1, 0, 20, 20);  
  fill_rect(&r1, 0, 0, 20, 20);  
  set_iregion(RGN(0, 1), -1,  0,  1, 20,  1);
  set_iregion(RGN(1, 1),  0,  0, 19, 20,  1);
  test_rectangle(&r0, &r1, &r, 1);

  // clip top 
  memset(rs, 0, sizeof(*rs));
  fill_rect(&r0, 0, -1, 20, 20);  
  fill_rect(&r1, 0, 0, 20, 20);  
  set_iregion(RGN(1, 0),  0,  -1, 20, 1,  1);
  set_iregion(RGN(1, 1),  0,  0, 20, 19,  1);
  test_rectangle(&r0, &r1, &r, 1);

  // clip right 
  memset(rs, 0, sizeof(*rs));
  fill_rect(&r0, 19, 0, 20, 20);  
  fill_rect(&r1, 0, 0, 20, 20);  
  set_iregion(RGN(1, 1), 19,  0,  1, 20,  1);
  set_iregion(RGN(2, 1), 20,  0, 19, 20,  1);
  test_rectangle(&r0, &r1, &r, 1);

  // clip bottom
  memset(rs, 0, sizeof(*rs));
  fill_rect(&r0, 0, 19, 20, 20);  
  fill_rect(&r1, 0, 0, 20, 20);  
  set_iregion(RGN(1, 1),  0,  19, 20, 1,  1);
  set_iregion(RGN(1, 2),  0,  20, 20, 19,  1);
  test_rectangle(&r0, &r1, &r, 1);

  // out bounds
  memset(rs, 0, sizeof(*rs));
  fill_rect(&r0, -20, 0, 20, 20);  
  fill_rect(&r1, 0, 0, 20, 20);  
  test_rectangle(&r0, &r1, &r, 0);

  memset(rs, 0, sizeof(*rs));
  fill_rect(&r0, 20, 0, 20, 20);  
  fill_rect(&r1, 0, 0, 20, 20);  
  test_rectangle(&r0, &r1, &r, 0);

  memset(rs, 0, sizeof(*rs));
  fill_rect(&r0, 0, 20, 20, 20);  
  fill_rect(&r1, 0, 0, 20, 20);  
  test_rectangle(&r0, &r1, &r, 0);

  memset(rs, 0, sizeof(*rs));
  fill_rect(&r0, 0, -20, 20, 20);  
  fill_rect(&r1, 0, 0, 20, 20);  
  test_rectangle(&r0, &r1, &r, 0);
}
