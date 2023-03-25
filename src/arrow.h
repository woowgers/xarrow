#ifndef ARROW_H
#define ARROW_H


#include <X11/Xlib.h>


struct arrow;


struct arrow * arrow_create(int, short, short);
void arrow_free(struct arrow *);
void arrow_set_center(struct arrow *, int, int);
void arrow_set_length(struct arrow *, int);
void arrow_set_target(struct arrow *, int, int);
int arrow_get_length(const struct arrow *);
XPoint arrow_get_target(const struct arrow *);
void arrow_draw(struct arrow *, Display *, Drawable, GC);


#endif /* ARROW_H */
