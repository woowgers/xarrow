#include "arrow.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>


extern bool draw_needed;


struct arrow
{
  int length;
  XPoint center, end, last_end;
};


struct arrow * arrow_create(int length, short center_x, short center_y)
{
  struct arrow * self;

  assert(length > 0);
  self = malloc(sizeof(struct arrow));
  assert(self != NULL);
  self->length = length;
  self->center = (XPoint){center_x, center_y};

  return self;
}

void arrow_free(struct arrow * self)
{
  free(self);
}

void arrow_set_center(struct arrow * self, int x, int y)
{
  self->center = (XPoint){x, y};
}

void arrow_set_length(struct arrow * self, int length)
{
  self->length = length;
}

void arrow_set_target(struct arrow * self, int x, int y)
{
  const double EPS = 0.01;
  int dx = x - self->center.x, dy = y - self->center.y;
  double length = hypot(dx, dy), coeff = length / self->length;

  if (coeff > EPS)
    self->last_end = self->end = (XPoint){.x = self->center.x + dx / coeff, .y = self->center.y + dy / coeff};
  else
    self->end = self->last_end;
}

int arrow_get_length(const struct arrow * self)
{
  return self->length;
}

XPoint arrow_get_target(const struct arrow * self)
{
  return self->end;
}

XPoint arrow_get_center(const struct arrow * self)
{
  return self->center;
}

void arrow_draw(struct arrow * self, Display * d, Drawable drawable, GC gc)
{
  const double delta_angle = M_PI / 30, length_coeff = 0.5;
  XPoint triangle_points[3];
  int dx = self->end.x - self->center.x, dy = self->end.y - self->center.y;
  double angle, angle_left, angle_right;

  angle = atan((double)dy / dx);
  if (dx < 0)
    angle += M_PI;
  angle_left = angle - delta_angle;
  angle_right = angle + delta_angle;

  triangle_points[0] = self->end;
  triangle_points[1] = (XPoint){.x = self->end.x - length_coeff * self->length * cos(angle_left),
                                .y = self->end.y - length_coeff * self->length * sin(angle_left)},
  triangle_points[2] = (XPoint){.x = self->end.x - length_coeff * self->length * cos(angle_right),
                                .y = self->end.y - length_coeff * self->length * sin(angle_right)};

  XDrawLine(d, drawable, gc, self->center.x, self->center.y, self->end.x, self->end.y);
  XFillPolygon(d, drawable, gc, triangle_points, 3, Convex, CoordModeOrigin);
}
