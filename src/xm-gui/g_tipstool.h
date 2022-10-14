#ifndef _CONFDB_GUI_G_TIPSTOOL_
#define _CONFDB_GUI_G_TIPSTOOL_

#include <string>
#include "g_rectangle.h"

class G_Object;

class G_TipsTool {
  friend class G_Object;

public:
  G_TipsTool(G_Object *, Dimension, Dimension, Dimension, Dimension);
  G_TipsTool(G_Object *, const G_Rectangle&);
  virtual ~G_TipsTool() {;}

  int operator==(const G_TipsTool&) const;

  int test_pointer(Dimension x, Dimension y) const
    {return box.is_point_inside(x, y);}

  virtual const char *get_text() const = 0;

protected:
  G_Rectangle	box;
  G_Object	*obj;
};


inline
G_TipsTool::G_TipsTool(G_Object *o, Dimension x, Dimension y, Dimension w, Dimension h) :
obj (o)
{
  box.set(x, y, w, h);
}

inline
G_TipsTool::G_TipsTool(G_Object *o, const G_Rectangle& r) :
obj (o)
{
  box.set(r);
}

inline int
G_TipsTool::operator==(const G_TipsTool& tt) const
{
  return (tt.box == box);
}


class G_SimpleTipsTool : public G_TipsTool {
public:
  G_SimpleTipsTool(G_Object *o, Dimension x, Dimension y, Dimension w, Dimension h, const std::string& t) :
    G_TipsTool (o, x, y, w, h), text (t) {;}

  G_SimpleTipsTool(G_Object *o, const G_Rectangle& r, const std::string& t) :
    G_TipsTool (o, r), text (t) {;}

  virtual ~G_SimpleTipsTool() {;}

  virtual const char *get_text() const {return text.c_str();}

private:
  const std::string text;
};


class G_RelationshipButtonTipsTool : public G_TipsTool {
public:
  G_RelationshipButtonTipsTool(G_Object *o, const G_Rectangle& r) :
    G_TipsTool (o, r) {;}

  virtual ~G_RelationshipButtonTipsTool() {;}

  virtual const char *get_text() const;
};


class G_ObjectTipsTool : public G_TipsTool {
public:
  G_ObjectTipsTool(G_Object *o, Dimension x, Dimension y, Dimension w, Dimension h) :
    G_TipsTool (o, x, y, w, h) {;}

  virtual ~G_ObjectTipsTool() {;}

  virtual const char *get_text() const;
};


#endif
