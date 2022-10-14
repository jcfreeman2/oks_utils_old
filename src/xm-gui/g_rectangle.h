#ifndef _CONFDB_GUI_G_RECTANGLE_
#define _CONFDB_GUI_G_RECTANGLE_

#include <X11/Intrinsic.h>	// defines 'Dimension' type


	//
	// Describes class Rectangle having:
	//  - (x,y) coordinate
	//  - width
	//  - height
	//

class G_Rectangle {
  friend class G_Object;
  friend class G_Context;

public:
  G_Rectangle ();
  G_Rectangle (Dimension, Dimension, Dimension, Dimension);

  int	      operator==(const G_Rectangle&) const;

  Dimension   get_x() const {return x;}
  Dimension   get_y() const {return y;}
  Dimension   get_width() const {return w;}
  Dimension   get_height() const {return h;}

  void        set(Dimension, Dimension, Dimension, Dimension);
  void        set(const G_Rectangle& r);

  int         is_point_inside(Dimension, Dimension) const;

private:
  Dimension   x;
  Dimension   y;
  Dimension   h;
  Dimension   w;
};


inline
G_Rectangle::G_Rectangle() : x(0), y(0), h(0), w(0) {;}


inline
G_Rectangle::G_Rectangle(Dimension x1, Dimension y1, Dimension w1, Dimension h1) :
 x(x1),
 y(y1),
 h(h1),
 w(w1)
{;}


inline int
G_Rectangle::operator==(const G_Rectangle& r) const
{
  return (
    r.x == x &&
    r.y == y &&
    r.h == h &&
    r.w == w
  );
}


inline void
G_Rectangle::set(Dimension x1, Dimension y1, Dimension w1, Dimension h1)
{
  x=x1;
  y=y1;
  w=w1;
  h=h1;
}


inline void
G_Rectangle::set(const G_Rectangle& r)
{
  x=r.x;
  y=r.y;
  w=r.w;
  h=r.h;
}


inline int
G_Rectangle::is_point_inside(Dimension x1, Dimension y1) const
{
  return (
     ( x <= x1 )       &&
     ( (x + w) >= x1 ) &&
     ( y <= y1 )       &&
     ( (y + h) >= y1 )
  );
}

#endif
