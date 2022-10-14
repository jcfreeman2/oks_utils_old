#include <unistd.h>
#include <sstream>

#include <oks/relationship.h>
#include <oks/method.h>
#include <oks/class.h>
#include <oks/xm_utils.h>
#include <oks/xm_popup.h>
#include <oks/xm_context.h>

#include "schema_editor_view_dlg.h"
#include "schema_editor_class_dlg.h"
#include "schema_editor_main_dlg.h"
#include "schema_editor.h"

#include <X11/cursorfont.h>
#include <Xm/Xm.h>
#include <Xm/DrawingA.h>
#include <Xm/ToggleB.h>


extern OksSchemaEditorMainDialog * mainDlg;

const Dimension font_dx = 4;
const Dimension font_dy = 3;

const Dimension line_w  = 1;
const Dimension line_w2 = 2;

///////////////////////////////////////////////////////////////////////

std::ostream&
operator<<(std::ostream& s, const Item& i)
{
  i.print(s);
  return s;
}

///////////////////////////////////////////////////////////////////////

Class::Class(short x1, short y1, OksClass * c1) :
  x                  (x1),
  y                  (y1),
  p_oks_class        (c1),
  p_attributes       (ShowDirect),
  p_methods          (ShowDirect),
  p_active_rectangle (0)
{
}

std::string
Class::get_attribute_text(const OksAttribute * a)
{
  std::string s(a->get_name());
  s += ": ";
  if(a->get_is_multi_values()) s += "list of ";
  s += a->get_type();
  if(!a->get_init_value().empty()) {
    s += " = \'";
    s += a->get_init_value();
    s += '\'';
  }

  return s;
}

bool
Class::is_inside(short x1, short y1, unsigned short w1, unsigned short h1) const
{
  if(x > (x1 + w1)) return false;
  if(x1 > (x + w))  return false;
  if(y > (y1 + h1)) return false;
  if(y1 > (y + h))  return false;
  
  return true;
}

bool
Class::does_select(short x1, short y1) const
{
  return (
    (x1 >= x) &&
    (x1 <= (x + w)) &&
    (y1 >= y) &&
    (y1 <= (y + h))
  );
}

void
Class::get_rect(short & x1, short & y1, unsigned short & w1, unsigned short & h1) const
{
  x1 = x;
  y1 = y;
  w1 = w;
  h1 = h;
}

void
Class::process_select_event(XButtonEvent * event)
{
  static Rectangle * old_active_rectangle = 0;
  static Time old_time = 0;

  old_active_rectangle = p_active_rectangle;
  p_active_rectangle = 0;
  
  std::vector<Rectangle>::iterator i = p_items.begin();

  while(i != p_items.end()) {
    if(
     (event->x >= (*i).x) &&
     (event->x <= ((*i).x + (*i).w)) &&
     (event->y >= (*i).y) &&
     (event->y <= ((*i).y + (*i).h))
    ) {
      if((*i).info == 0) {
        break;
      }
      else {
        p_active_rectangle = &(*i);
	break;
      }
    }
    ++i;
  }

  if(old_active_rectangle == p_active_rectangle) {

      // read double-click time interval set in the X server

    static const Time double_click_time_interval = static_cast<Time>(mainDlg->get_multi_click_time());


      // create an attribute, a method or a class dialog

    if((event->time - old_time) <= double_click_time_interval) {

      if(p_active_rectangle == 0) {
        mainDlg->create_class_dlg(p_oks_class);
      }
      else {

          // check list of attributes

        const std::list<OksAttribute *> * attrs = p_oks_class->direct_attributes();
        if(attrs) {
          for(std::list<OksAttribute *>::const_iterator j = attrs->begin(); j != attrs->end(); ++j) {
            if((void *)(*j) == (*i).info) {
	      mainDlg->create_attribute_dlg(*j, p_oks_class);
	      break;
	    }
	  }
        }

          // check list of methods

        const std::list<OksMethod *> * mets = p_oks_class->direct_methods();
        if(mets) {
          for(std::list<OksMethod *>::const_iterator j = mets->begin(); j != mets->end(); ++j) {
            if((void *)(*j) == (*i).info) {
	      mainDlg->create_method_dlg(*j, p_oks_class);
	      break;
	    }
	  }
        }
      }
    }
  }

  old_time = event->time;
}

void
Class::process_deselect_event()
{
  p_active_rectangle = 0;
}

void
Class::draw(OksSchemaEditorViewDialog * parent, DrawType /*draw_type*/)
{
  OksGC & gc = parent->get_gc();

  Dimension class_name_width = gc.get_string_width(p_oks_class->get_name().c_str());
  Dimension font_height = gc.get_font_height();

  w = class_name_width;

  unsigned short row_number = 1;

  const std::list<OksAttribute *> * attributes = (
    p_attributes == ShowAll ? p_oks_class->all_attributes() :
    p_attributes == ShowDirect ? p_oks_class->direct_attributes() :
    0
  );

  const std::list<OksMethod *> * methods = (
    p_methods == ShowAll ? p_oks_class->all_methods() :
    p_methods == ShowDirect ? p_oks_class->direct_methods() :
    0
  );

  if(attributes) {
    row_number += attributes->size();
    for(std::list<OksAttribute *>::const_iterator i = attributes->begin(); i != attributes->end(); ++i) {
      Dimension aw = gc.get_string_width(get_attribute_text(*i).c_str());
      if(aw > w) w = aw;
    }
  }

  if(methods) {
    row_number += methods->size();
    for(std::list<OksMethod *>::const_iterator i = methods->begin(); i != methods->end(); ++i) {
      Dimension aw = gc.get_string_width((*i)->get_name().c_str());
      if(aw > w) w = aw;
    }
  }

    // reset vector of rectangles if required

  if(p_items.size() != row_number) {
    p_items.clear();
    p_items.resize(row_number);
    p_active_rectangle = 0;
  }

  unsigned int pos = 0;

  w += 2 * font_dx + line_w2;
  h = row_number * (font_height + font_dy) + 3 * (font_dy + line_w2);

  gc.clean_rectangle(x - line_w, y - line_w, w + line_w2, h + line_w2);

  if(is_selected == false) {
    gc.draw_rectangle(x, y, w, h, 0, line_w2);
  }
  else {
    gc.draw_stippled_rectangle(x, y, w, h, "dark blue");
    if(p_active_rectangle != 0)
      gc.draw_rectangle(
        p_active_rectangle->x + line_w,
	p_active_rectangle->y + line_w,
	p_active_rectangle->w - 2 * line_w,
	p_active_rectangle->h - 2 * line_w,
	gc.get_top_shadow_color(), 0, true
      );
  }

  Dimension x1 = x + (w - class_name_width) / 2 + line_w;
  Dimension y1 = y + font_dy + line_w2;

  gc.draw_string(x1, y1, p_oks_class->get_name().c_str());
  p_items[pos++].set(x, y, w, font_height + 2 * font_dy);

  x1 = x;
  y1 += font_height + font_dy;

  gc.draw_line(x1, y1, x1 + w, y1, 0, line_w2);

  y1 += line_w;
  x1 += font_dx;

  if(attributes) {
    for(std::list<OksAttribute *>::const_iterator i = attributes->begin(); i != attributes->end(); ++i) {
      p_items[pos++].set(x, y1, w, font_height + 2 * font_dy, (void *)(*i));
      y1 += font_dy;
      gc.draw_string(x1, y1, get_attribute_text(*i).c_str());
      y1 += font_height;
    }
  }
  
  y1 += font_dy;

  gc.draw_line(x1 - font_dx, y1, x1 + w - font_dx, y1, 0, line_w2);

  y1 += line_w2;

  if(methods) {
    for(std::list<OksMethod *>::const_iterator i = methods->begin(); i != methods->end(); ++i) {
      p_items[pos++].set(x, y1, w, font_height + 2 * font_dy, (void *)(*i));
      y1 += font_dy;
      gc.draw_string(x1, y1, (*i)->get_name().c_str());
      y1 += font_height;
    }
  }
}

void
Class::save(OksViewSchema & view_schema) const
{
  OksObject * obj = new OksObject(view_schema.class_c, p_oks_class->get_name().c_str());

    // set x
  {
    OksData d(x);
    obj->SetAttributeValue("x", &d);
  }

    // set y
  {
    OksData d(y);
    obj->SetAttributeValue("y", &d);
  }
}

void
Class::print(std::ostream& s) const
{
  s << "class \'" << get_oks_class()->get_name() << "\' (" << (void *)this << ')';
}

void
Class::make_selected(OksSchemaEditorViewDialog * p, bool value)
{
  if(value != is_selected) {
    XtSetSensitive(p->get_remove_class_button(), value);
  }

  is_selected = value;
}

///////////////////////////////////////////////////////////////////////////////////

void
Handle::set_points(Point * t1, Point * t2)
{
  p_type = ((t1->get_y() == t2->get_y()) ? Horizontal : Vertical);

  p1 = t1;
  p2 = t2;

  const Class * c = (
    (r->get_points().front() == p1) ? r->get_class_from() :
    (r->get_points().back()  == p2) ? r->get_class_to() :
    0
  );

  min_x = min_y = 0;
  max_x = max_y = 32767;

  if(c) {
    if(p_type == Horizontal) {
      min_y = c->get_y();
      max_y = min_y + c->get_height();
    }
    else {
      min_x = c->get_x();
      max_x = min_x + c->get_width();
    }
  }
}

///////////////////////////////////////////////////////////////////////////////////


Link::Link(const class Class * cf, const class Class * ct, OksRelationship * r1) :
  p_link_type        (r1 ? Link::Relationship : Link::Generalization),
  p_oks_relationship (r1),
  p_from             (cf),
  p_to               (ct),
  p_active_handle    (0),
  rx1                (0),
  rx2                (0),
  ry1                (0),
  ry2                (0),
  p_active_point     (0),
  p_is_reshaped      (false)

{
  unsigned short srel_dhb = 24;  // self-relationship delta-height bottom
  unsigned short srel_dht = 24;  // self-relationship delta-height top

  if(p_from != p_to) {
    short x1 = p_from->get_width() * 3 / 4;
    short y1 = p_from->get_height() * 3 / 4;
    short x4 = p_to->get_width() / 4;
    short y4 = p_to->get_height() / 4;

    short x2 = x1;
    short x3 = x4;
    short y2 = ( p_from->get_y() + p_to->get_y() ) / 2;
    short y3 = y2;

    p_segments.push_back(new Point(x1, y1));
    p_segments.push_back(new Point(x2, y2));
    p_segments.push_back(new Point(x3, y3));
    p_segments.push_back(new Point(x4, y4));
  }
  else {

      // found ordering number of relationship

    unsigned int count = 0;
    const std::list<OksRelationship *> * rels = cf->get_oks_class()->direct_relationships();
    std::list<OksRelationship *>::const_iterator j;
    for(j = rels->begin(); j != rels->end(); ++j) {
      OksRelationship * r = *j;
      if(r->get_class_type() == cf->get_oks_class()) count ++;
      if(r == p_oks_relationship) break;
    }

    short points[10];

    if(srel_dht > (p_from->get_height() / 3)) {
      srel_dht = p_from->get_height() / 3;
    }

    if((count % 4) == 1) {
      points[8] = p_from->get_width();
      points[6] = points[4] = (p_from->get_width() * 4) / 3;
      points[9] = points[7] = p_from->get_height() - srel_dht;
      points[5] = points[3] = p_from->get_height() + srel_dhb;
      points[2] = points[0] = (p_from->get_width() * 2) / 3;
      points[1] = p_from->get_height();
    }
    else if((count % 4) == 2) {
      points[8] = 0;
      points[6] = points[4] = - (p_from->get_width() / 3);
      points[9] = points[7] = p_from->get_height() - srel_dht;
      points[5] = points[3] = p_from->get_height() + srel_dhb;
      points[2] = points[0] = p_from->get_width() / 3;
      points[1] = p_from->get_height();
    }
    else if((count % 4) == 3) {
      points[2] = points[0] = p_from->get_width() / 3;
      points[5] = points[3] = - srel_dhb;
      points[6] = points[4] = - (p_from->get_width() / 3);
      points[7] = points[9] = srel_dht;
      points[8] = points[1] = 0;
    }
    else {
      points[1] = 0;
      points[0] = points[2] = (p_from->get_width() * 2) / 3;
      points[3] = points[5] = - srel_dhb;
      points[4] = points[6] = (p_from->get_width() * 4) / 3;
      points[7] = points[9] = srel_dht;
      points[8] = p_from->get_width();
    }

    if(count > 4) {
      std::cout << "WARNING: to many self-links; count = " << count << std::endl;
    }

    p_segments.push_back(new Point(points[0], points[1]));
    p_segments.push_back(new Point(points[2], points[3]));
    p_segments.push_back(new Point(points[4], points[5]));
    p_segments.push_back(new Point(points[6], points[7]));
    p_segments.push_back(new Point(points[8], points[9]));
  }

  set_multiplicity_text();
}


void
Link::set_multiplicity_text()
{
  if(p_link_type == Link::Relationship) {

    p_multiplicity_text = (

      p_oks_relationship->get_low_cardinality_constraint() == OksRelationship::Zero &&
      p_oks_relationship->get_high_cardinality_constraint() == OksRelationship::One ? "0..1" :

      p_oks_relationship->get_low_cardinality_constraint() == OksRelationship::Zero &&
      p_oks_relationship->get_high_cardinality_constraint() == OksRelationship::Many ? "0..N" :

      p_oks_relationship->get_low_cardinality_constraint() == OksRelationship::One &&
      p_oks_relationship->get_high_cardinality_constraint() == OksRelationship::One ? "1..1" :

      p_oks_relationship->get_low_cardinality_constraint() == OksRelationship::One &&
      p_oks_relationship->get_high_cardinality_constraint() == OksRelationship::Many ? "1..N" :

      "???"

    );

  }
}


  /**
   *  The 'save' method puts link description into OKS configuration file.
   */

void
Link::save(OksViewSchema & view_schema) const
{
  OksObject * obj = 0;

  std::string id(p_from->get_oks_class()->get_name());
  id += "-2-";
  id += p_to->get_oks_class()->get_name();

  if(p_link_type == Relationship) {
    id += "-via-";
    id += p_oks_relationship->get_name();

    obj = new OksObject(view_schema.alink_c, id.c_str());
 
    OksData d(p_oks_relationship->get_name());
    obj->SetAttributeValue("Name", &d);
  }
  else {
    obj = new OksObject(view_schema.glink_c, id.c_str());
  }

  obj->SetRelationshipValue("From", "Class", p_from->get_oks_class()->get_name());
  obj->SetRelationshipValue("To", "Class", p_to->get_oks_class()->get_name());

  unsigned short count = 0;
  for(std::list<Point *>::const_iterator i = p_segments.begin(); i != p_segments.end(); ++i) {
    std::ostringstream s;
    s << ++count << ' ' << id << std::ends;

    std::string s2 = s.str();
    OksObject * p = new OksObject(view_schema.point_c, s2.c_str());

    obj->AddRelationshipValue("Points", p);

        // set x
    {
      OksData d((*i)->get_x());
      p->SetAttributeValue("x", &d);
    }

      // set y
    {
      OksData d((*i)->get_y());
      p->SetAttributeValue("y", &d);
    }
  }
}

void
Link::print(std::ostream& s) const
{
  s << (p_link_type == Generalization ? "generalization link" : "relationship")
    << " from \'" << p_from->get_oks_class()->get_name()
    << "\' to \'" << p_to->get_oks_class()->get_name() << "\' (" << (void *)this << ')';
  
  if(p_link_type == Relationship && p_oks_relationship) {
    s << " name \'" << p_oks_relationship->get_name() << '\'';
  }
}


  /**
   *  The 'does_select' returns true if a rectangle's line
   *  is close to the point given via parameter.
   *
   *  The closeness is defined by the 'dx' constant
   *  and it is measured in the pixels.
   */

bool
Link::does_select(short x, short y) const
{
  const unsigned short dx = 2;

  std::list<Point *>::const_iterator i = p_segments.begin();
  std::list<Point *>::const_iterator i2 = p_segments.begin(); ++i2;

  while(i2 != p_segments.end()) {
    short x1 = get_x(*i);
    short y1 = get_y(*i);
    short x2 = get_x(*i2);
    short y2 = get_y(*i2);

    if(x1 == x2) {
      if(
        (x > (x1 - dx)) &&
	(x < (x1 + dx)) &&
        (
	  ( (y1 <= y2) && (y >= y1) && (y <=y2) ) ||
	  ( (y2 <= y1) && (y >= y2) && (y <=y1) )
	)
      ) {
        return true;
      }
    }
    else {
      if(
        (y > (y1 - dx)) &&
	(y < (y1 + dx)) &&
        (
	  ( (x1 <= x2) && (x >= x1) && (x <=x2) ) ||
	  ( (x2 <= x1) && (x >= x2) && (x <=x1) )
	)
      ) {
        return true;
      }
    }

    ++i; ++i2;
  }

  return false;
}


  /**
   *  The 'does_reshape' returns true if the point (x,y)
   *  is inside one of the reshape handels.
   */

bool
Link::does_reshape(short x, short y)
{
  const unsigned short dx = 3;

  p_active_point = 0;

  Point * points[2] = {&p_point_from, &p_point_to};

  for(int i = 0; i < 2; ++i) {
    Point * p = points[i];

    short x1 = p->get_x() - dx;
    short x2 = p->get_x() + dx;
    short y1 = p->get_y() - dx;
    short y2 = p->get_y() + dx;

    if(x >= x1 && x <= x2 && y >= y1 && y <= y2) {
      p_active_point = (p == &p_point_from) ? &p_point_to : &p_point_from ;
      return true;
    }
  }

  return false;
}


  /**
   *  The 'is_inside' returns true if the relationship-picture-rectangle
   *  has intersection with rectangle given via parameter
   */

bool
Link::is_inside(short x1, short y1, unsigned short w1, unsigned short h1) const
{
  if(rx1 > (x1 + w1)) return false;
  if(rx2 < x1)  return false;
  if(ry1 > (y1 + h1)) return false;
  if(ry2 < y1)  return false;
  
  return true;
}


  /**
   *  The 'adjust_rect' is used to calculate min/max boundaries
   *  of the relationship picture
   */

void
Link::adjust_rect(short x1, short y1,short  x2, short y2)
{
  if(x1 < rx1) rx1 = x1;
  if(x2 < rx1) rx1 = x2;

  if(y1 < ry1) ry1 = y1;
  if(y2 < ry1) ry1 = y2;

  if(x1 > rx2) rx2 = x1;
  if(x2 > rx2) rx2 = x2;

  if(y1 > ry2) ry2 = y1;
  if(y2 > ry2) ry2 = y2;
}


  /**
   *  The 'get_rect' returns minimal rectangle containing relationship picture
   */

void
Link::get_rect(short & rx, short & ry, unsigned short & rw, unsigned short & rh) const
{
  rx = rx1;
  ry = ry1;
  
  rw = rx2 - rx1;
  rh = ry2 - ry1;
}


void
Link::shift(short, short dy)
{
  if(p_from != p_to) {
    Point * i_front = p_segments.front();
    Point * i_back = p_segments.back();

    for(std::list<Point *>::iterator i = p_segments.begin(); i != p_segments.end(); ++i) {
      if(*i != i_front && *i != i_back) {
        short ny = (*i)->get_y() + dy;
        (*i)->set_y(ny);
      }
    }
  }
}


void
Link::process_select_event(XButtonEvent * event)
{
  static Link * old_link = 0;
  static Time old_time = 0;

  p_active_handle = get_handle(event->x, event->y);
  
  if(old_link == this) {

          // read double-click time interval set in the X server

    static const Time double_click_time_interval = static_cast<Time>(mainDlg->get_multi_click_time());


      // create an attribute or a method dialog

    if((event->time - old_time) <= double_click_time_interval) {
      if(p_oks_relationship)
        mainDlg->create_relationship_dlg(p_oks_relationship, p_from->get_oks_class());
      else {
        OksSchemaEditorClassDialog * cdlg = mainDlg->create_class_dlg(p_to->get_oks_class());
	cdlg->select_super_class(p_from->get_oks_class()->get_name());
      }
    }
  }
  
  old_link = this;
  old_time = event->time;
}

void
Link::process_deselect_event()
{
  p_active_handle = 0;
  p_active_point = 0;
  p_is_reshaped = false;
}

short
Link::get_x(const Point * p) const
{
  if(p_from == p_to) return (p->get_x() + p_from->get_x());

  std::list<Point *>::const_iterator i = p_segments.begin();
  if(p == *i || p == *(++i)) return (p->get_x() + p_from->get_x());

  return (p->get_x() + p_to->get_x());  
}

short
Link::get_y(const Point * p) const
{
  return (
    p->get_y() + (
      (p_from == p_to) ? p_from->get_y() :
      (p == p_segments.front()) ? p_from->get_y() :
      (p == p_segments.back()) ? p_to->get_y() :
      0
    )
  );
}


Handle *
Link::get_handle(short x, short y) const
{
  const Dimension dx = 3;

  for(std::list<Handle *>::const_iterator i = p_handles.begin(); i != p_handles.end(); ++i) {
    Handle * h = *i;
    short x1 = h->get_x();
    short y1 = h->get_y();

    if(x >= (x1 - dx) && x <= (x1 + dx) && y >= (y1 - dx) && y <= (y1 + dx)) return h;
  }

  return 0;
}

void
Link::move_handle(short x, short y)
{
  if(p_active_handle == 0) {
    return;
  }

  Point * p1 = p_active_handle->get_point1();
  Point * p2 = p_active_handle->get_point2();

  if(x < p_active_handle->get_min_x()) x = p_active_handle->get_min_x();
  if(x > p_active_handle->get_max_x()) x = p_active_handle->get_max_x();
  if(y < p_active_handle->get_min_y()) y = p_active_handle->get_min_y();
  if(y > p_active_handle->get_max_y()) y = p_active_handle->get_max_y();

  if(x == p_active_handle->get_x() && y == p_active_handle->get_y()) {
    return;
  }

  p_active_handle->set_x(x);
  p_active_handle->set_y(y);

  if(p_to == p_from) {
    x -= p_to->get_x();
    y -= p_to->get_y();
  }
  else if(p_segments.front() == p1) {
    x -= p_from->get_x();
    y -= p_from->get_y();
  }
  else if(p_segments.back() == p2) {
    x -= p_to->get_x();
    y -= p_to->get_y();
  }

  if(p_active_handle->get_type() == Handle::Vertical) {
    p1->set_x(x);
    p2->set_x(x);
  }
  else {
    p1->set_y(y);
    p2->set_y(y);
  }
}

void
Link::draw(OksSchemaEditorViewDialog * parent, DrawType draw_type)
{
  OksGC& gc = parent->get_gc();

    // constants below are used to draw aggregation sign

  const short ah = parent->get_aggregation_sign_size();
  const short ah2 = ah / 2;
  const short ah3 = ah / 3;
  
  
    // constants below are used to draw generalization sign

  const short gh = parent->get_generalization_sign_size();
  const short gh2 = gh / 2;


    // constants below are used to draw handle

  const Dimension dx = 3;
  const Dimension dx2 = dx * 2 + line_w;
  const Dimension dxw = dx + line_w;


    // remove old reshape boxes if any

  if(draw_type == Item::Deshape) {
    gc.clean_rectangle(p_point_from.get_x() - dx, p_point_from.get_y() - dx, dx2, dx2);
    gc.clean_rectangle(p_point_to.get_x() - dx, p_point_to.get_y() - dx, dx2, dx2);
  }


  Point * points_ptrs[5];

    // check first and last points:
    // they should be INSIDE class which may not be true
    // if font been changed

  {
    unsigned short j = 0;

    for(std::list<Point *>::iterator i = p_segments.begin(); i != p_segments.end(); ++i) {
      points_ptrs[j++] = *i;
    }

    if(points_ptrs[0]->get_x() > p_from->get_width()) {
      if(points_ptrs[0]->get_x() == points_ptrs[1]->get_x()) {
        points_ptrs[0]->set_x(p_from->get_width() - 1);
        points_ptrs[1]->set_x(p_from->get_width() - 1);
      }
      else {
        points_ptrs[0]->set_x(p_from->get_width() - 1);
      }
    }

    if(points_ptrs[0]->get_y() > p_from->get_height()) {
      if(points_ptrs[0]->get_y() == points_ptrs[1]->get_y()) {
        points_ptrs[0]->set_y(p_from->get_height() - 1);
        points_ptrs[1]->set_y(p_from->get_height() - 1);
      }
      else {
        points_ptrs[0]->set_y(p_from->get_height() - 1);
      }
    }

    if(points_ptrs[j-1]->get_x() > p_to->get_width()) {
      if(points_ptrs[j-1]->get_x() == points_ptrs[j-2]->get_x()) {
        points_ptrs[j-1]->set_x(p_to->get_width() - 1);
        points_ptrs[j-2]->set_x(p_to->get_width() - 1);
      }
      else {
        points_ptrs[j-1]->set_x(p_to->get_width() - 1);
      }
    }

    if(points_ptrs[j-1]->get_y() > p_to->get_height()) {
      if(points_ptrs[j-1]->get_y() == points_ptrs[j-2]->get_y()) {
        points_ptrs[j-1]->set_y(p_to->get_height() - 1);
        points_ptrs[j-2]->set_y(p_to->get_height() - 1);
      }
      else {
        points_ptrs[j-1]->set_y(p_to->get_height() - 1);
      }
    }
  }


    // check relationships which starts and ends at the same class
    // at least two points must be OUTSIDE class which may not be true
    // if font been changed or link was wrongly reshaped

  if(p_from == p_to && is_selected == false) {
    bool found_outside_point = false;

    for(unsigned short j = 1; j < 5; ++j) {
      if(p_to->does_select(p_to->get_x() + points_ptrs[j]->get_x(), p_to->get_y() + points_ptrs[j]->get_y()) == false) {
        found_outside_point = true;
	break;
      }
    }

    if(found_outside_point == false) {
      std::cerr << "WARNING: link \"" << *this << "\" is inside class\n"
                   "*** Reshaping the link to make it visible... ***\n";

      if(points_ptrs[1]->get_x() == points_ptrs[2]->get_x()) {
        int new_x = p_to->get_width() +
	            gc.get_string_width(get_oks_relationship()->get_name().c_str()) +
		    2 * gc.get_params().get_font_x_margin();

        points_ptrs[1]->set_x(new_x);
        points_ptrs[2]->set_x(new_x);
      }
      else {
        int new_y = p_to->get_height() +
	            gc.get_font_height() +
		    2 * gc.get_params().get_font_y_margin();

        points_ptrs[1]->set_y(new_y);
        points_ptrs[2]->set_y(new_y);
      }
    }
  }


    // fill points array

  short points[10];
  unsigned short num = 0;

  {
    std::list<Point *>::iterator i = p_segments.begin();
    for(; i != p_segments.end(); ++i) {
      points_ptrs[num / 2] = *i;
      points[num++] = get_x(*i);
      points[num++] = get_y(*i);
      
        // check if there are points inside class boxes

      if(num == 4) {
        if(p_from->does_select(points[2], points[3])) {
          points_ptrs[0] = points_ptrs[1];
	  points[0] = points[2];
	  points[1] = points[3];
	  num = 2;
	}
      }

      if(p_to != p_from || num > 2) {
        if(p_to->does_select(points[num - 2], points[num - 1])) {
	  break;
	}
      }
    }
  }

  if(points[1] == points[3]) {
    points[0] = (points[0] < points[2]) ? p_from->get_x() + p_from->get_width() + line_w : p_from->get_x() - line_w;
  }
  else {
    points[1] = (points[1] < points[3]) ? p_from->get_y() + p_from->get_height() + line_w : p_from->get_y() - line_w;
  }

  if(points[num - 3] == points[num - 1]) {
    points[num - 2] = (points[num - 2] < points[num - 4]) ? p_to->get_x() + p_to->get_width() + line_w : p_to->get_x() - line_w;
  }
  else {
    points[num - 1] = (points[num - 1] < points[num - 3]) ? p_to->get_y() + p_to->get_height() + line_w : p_to->get_y() - line_w;
  }


    // find place for relationship name and multiplicity text

  if(p_link_type == Link::Relationship) {
    int name_width = gc.get_string_width(p_oks_relationship->get_name().c_str());
    
    short x1, y1, x2, y2;

    if(num > 6) {
      x1 = points[2];
      y1 = points[3];
      x2 = points[4];
      y2 = points[5];
    }
    else {
      x1 = points[0];
      y1 = points[1];
      x2 = points[2];
      y2 = points[3];
    }

    if(y1 == y2) {
      if(x1 < x2) {
        p_name.set_y(y1 - gc.get_font_height() - font_dy);
        if((x2 - x1) >= (name_width + 2 * font_dx)) {
          p_name.set_x(x1 + (x2 - x1 - name_width) / 2 + line_w);
        }
        else {
          p_name.set_x(x2 + font_dx);
        }
      }
      else {
        p_name.set_y(y1 + font_dy);
        if((x1 - x2) >= (name_width + 2 * font_dx)) {
          p_name.set_x(x2 + (x1 - x2 - name_width) / 2 + line_w);
        }
        else {
          p_name.set_x(x2 - name_width - font_dx);
        }
      }
    }
    else {
      p_name.set_y((y1 + y2 - gc.get_font_height())/2);

      if(y1 < y2) {
        p_name.set_x(x1 + font_dx);
      }
      else {
        p_name.set_x(x1 - name_width - font_dx);
      }
    }

    if(points[num - 4] == points[num - 2]) {
      if(points[num - 3] <= points[num - 1]) {
        p_multiplicity.set_x(points[num - 2] - gc.get_string_width(get_multiplicity_text()) - font_dx);
        p_multiplicity.set_y(points[num - 1] - font_dy - gc.get_font_height());
      }
      else {
        p_multiplicity.set_x(points[num - 2] + line_w + font_dx);
        p_multiplicity.set_y(points[num - 1] + font_dy);
      }
    }
    else {
      if(points[num - 4] >= points[num - 2]) {
        p_multiplicity.set_x(points[num - 2] + line_w + font_dx);
        p_multiplicity.set_y(points[num - 1] - font_dy - gc.get_font_height());
      }
      else {
        p_multiplicity.set_x(points[num - 2] - gc.get_string_width(get_multiplicity_text()) - font_dx);
        p_multiplicity.set_y(points[num - 1] + font_dy);
      }
    }
  }

    // clean rectangle min-max values
  
  rx1 = rx2 = points[0];
  ry1 = ry2 = points[1];


  if(draw_type == Item::Select || draw_type == Item::Deselect || is_selected) {
    if(draw_type != Item::Deselect) {

        // remember points of active handle

      Point * ah1 = p_active_handle ? p_active_handle->get_point1() : 0;
      Point * ah2 = p_active_handle ? p_active_handle->get_point2() : 0;

      std::list<Handle *>::iterator i = p_handles.begin();

      short x1 = points[0];
      short y1 = points[1];
      for(unsigned short j = 1; j < num / 2; ++j) {
        short x2 = points[j * 2];
        short y2 = points[j * 2 + 1];

        short x = (x1 + x2) / 2;
        short y = (y1 + y2) / 2;

        if(i != p_handles.end()) {

	    // clean existing rectangle first

          (*i)->set_x(x);
	  (*i)->set_y(y);
	  (*i)->set_points(points_ptrs[j - 1], points_ptrs[j]);
	  ++i;
        }
	else {
          p_handles.push_back(new Handle(x, y, this, points_ptrs[j - 1], points_ptrs[j]));
	}

	x1 = x2;
	y1 = y2;
      }

      while(i != p_handles.end()) {
        Handle * h = *i;
        if(num <= 4 && h == p_active_handle) {
          ++i;
        }
	else {
          i = p_handles.erase(i);
          delete h;
	}
      }

      p_active_handle = 0;
      for(std::list<Handle *>::iterator i = p_handles.begin(); i != p_handles.end(); ++i) {
        Handle * h = *i;

	if(ah1 != 0 && h->get_point1() == ah1 && h->get_point2() == ah2) {
	  p_active_handle = h;
	  break;
	}
      }
    }

      // draw or remove handle boxes

    for(std::list<Handle *>::iterator i = p_handles.begin(); i != p_handles.end(); ++i) {
      Handle * h = *i;
      Dimension x = h->get_x();
      Dimension y = h->get_y();
      if(draw_type == Item::Select || (is_selected && draw_type == Item::Normal)) {
        if(gc.get_fc() == 0) {
          gc.draw_rectangle(x - dx, y - dx, dx2, dx2, "green", 0, true);
          adjust_rect(x - dx, y - dx, x + dx + line_w, y + dx + line_w);
        }
      }
      else {
        gc.clean_rectangle(x - dx, y - dx, dx2 + line_w, dx2 + line_w);
      }
    }

    if(draw_type == Item::Select) {
      return;
    }
    else if(draw_type == Item::Deselect){
      (const_cast<class Class *>(p_from))->draw(parent);
      (const_cast<class Class *>(p_to))->draw(parent);

      while(!p_handles.empty()) {
        Handle * h = p_handles.front();
	p_handles.pop_front();
	delete h;
      }
    }
  }


  short x1 = points[0];
  short y1 = points[1];

  unsigned short num2 = num / 2;
  unsigned short num21 = num2 - 1;

  for(unsigned short j = 1; j < num2; ++j) {
    short x2 = points[j * 2];
    short y2 = points[j * 2 + 1];

    if(j == 1) {
      if(p_link_type == Link::Relationship) {
        if(p_oks_relationship->get_is_composite()) {
          if(y1 == y2) {
            short ahs = (x1 < x2) ? ah : -ah;
            short ah2s = (x1 < x2) ? ah2 : -ah2;
            gc.draw_line(x1, y1, x1 + ah2s, y1 + ah3);
            gc.draw_line(x1, y1, x1 + ah2s, y1 - ah3);
            gc.draw_line(x1 + ah2s, y1 + ah3, x1 + ahs, y1);
            gc.draw_line(x1 + ah2s, y1 - ah3, x1 + ahs, y1);
            adjust_rect(x1, y1 - ah3, x1 + ahs, y1 + ah3);
	    x1 += ahs;
          }
          else {
            short ahs = (y1 < y2) ? ah : -ah;
            short ah2s = (y1 < y2) ? ah2 : -ah2;
            gc.draw_line(x1, y1, x1 - ah3, y1 + ah2s);
            gc.draw_line(x1, y1, x1 + ah3, y1 + ah2s);
            gc.draw_line(x1 - ah3, y1 + ah2s, x1, y1 + ahs);
            gc.draw_line(x1 + ah3, y1 + ah2s, x1, y1 + ahs);
            adjust_rect(x1 - ah3, y1, x1 + ah3, y1 + ahs);
            y1 += ahs;
          }
        }
      }
      else {
        if(y1 == y2) {
          short ghs = (x1 < x2) ? gh : -gh;
          gc.draw_line(x1, y1, x1 + ghs, y1 + gh2);
          gc.draw_line(x1, y1, x1 + ghs, y1 - gh2);
          gc.draw_line(x1 + ghs, y1 - gh2, x1 + ghs, y1 + gh2);
          adjust_rect(x1, y1 - gh2, x1 + ghs, y1 + gh2);
	  x1 += ghs;
        }
        else {
          short ghs = (y1 < y2) ? gh : -gh;
          gc.draw_line(x1, y1, x1 - gh2, y1 + ghs);
          gc.draw_line(x1, y1, x1 + gh2, y1 + ghs);
          gc.draw_line(x1 - gh2, y1 + ghs, x1 + gh2, y1 + ghs);
          adjust_rect(x1 - gh2, y1, x1 + gh2, y1 + ghs);
          y1 += ghs;
        }
      }
    }

    gc.draw_line(x1, y1, x2, y2);
    adjust_rect(x1, y1, x2 + line_w, y2 + line_w);

      // fill reshape points

    if(j == 1 || j == num21) {
      short nx1 = x1;
      short nx2 = x2;
      short ny1 = y1;
      short ny2 = y2;

      if(x1 == x2) {
        if(y1 < y2) { ny1 += dxw; ny2 -= dxw; }
	else        { ny1 -= dxw; ny2 += dxw; }
      }
      else {
        if(x1 < x2) { nx1 += dxw; nx2 -= dxw; }
	else        { nx1 -= dxw; nx2 += dxw; }
      }

      if(j == 1) {
        p_point_from.set_x(nx1);
        p_point_from.set_y(ny1);
      }

      if(j == num21) {
        p_point_to.set_x(nx2);
        p_point_to.set_y(ny2);
      }
    }

    x1 = x2;
    y1 = y2;
  }

  if(p_is_reshaped) {
    gc.draw_rectangle(p_point_from.get_x() - dx, p_point_from.get_y() - dx, dx2, dx2, "red", 0, true);
    adjust_rect(p_point_from.get_x() - dx, p_point_from.get_y() - dx, p_point_from.get_x() + dx + line_w, p_point_from.get_y() + dx + line_w);

    gc.draw_rectangle(p_point_to.get_x() - dx, p_point_to.get_y() - dx, dx2, dx2, "red", 0, true);
    adjust_rect(p_point_to.get_x() - dx, p_point_to.get_y() - dx, p_point_to.get_x() + dx + line_w, p_point_to.get_y() + dx + line_w);
  }

  if(p_link_type == Link::Relationship) {
    gc.draw_string(p_name.get_x(), p_name.get_y(), p_oks_relationship->get_name().c_str());
    gc.draw_string(p_multiplicity.get_x(), p_multiplicity.get_y(), get_multiplicity_text());
    adjust_rect(
      p_name.get_x(), p_name.get_y(),
      p_name.get_x() + gc.get_string_width(p_oks_relationship->get_name().c_str()),
      p_name.get_y() + gc.get_font_height() + font_dx
    );
    adjust_rect(
      p_multiplicity.get_x(), p_multiplicity.get_y(),
      p_multiplicity.get_x() + gc.get_string_width(get_multiplicity_text()),
      p_multiplicity.get_y() + gc.get_font_height() + font_dy
    );
  }
}

///////////////////////////////////////////////////////////////////////////////////


void
OksSchemaEditorViewDialog::toolCB(Widget w, XtPointer c_data, XtPointer)
{
  OksSchemaEditorViewDialog * dlg = (OksSchemaEditorViewDialog *)c_data;
  long old_tool = dlg->p_tool;

  dlg->p_tool = (long)OksXm::get_user_data(w);

  switch(dlg->p_tool) {
    case idPSelect:
      break;

    case idPMove:
      dlg->deselect();
      if(old_tool != idPMove) {
        dlg->gc.define_cursor(XC_fleur);
      }
      break;

    case idPLink2:
    case idPGLink:
      dlg->deselect();
      break;
  }

  if(old_tool == idPMove && dlg->p_tool != idPMove) {
    dlg->gc.undefine_cursor();
  }

  dlg->set_tool_state(w);
}

void
OksSchemaEditorViewDialog::menuCB(Widget w, XtPointer c_data, XtPointer)
{
  OksSchemaEditorViewDialog * dlg = (OksSchemaEditorViewDialog *)c_data;

  switch((long)OksXm::get_user_data(w)) {
    case idPNewClass:
      try
        {
          std::string s = OksXm::ask_name(dlg->get_form_widget(), "Input name of new class;\nthen choose place where to put it", "New OKS Class");
          OksClass * c = new OksClass(s, dlg->parent());
          dlg->p_looking4place = true;
          dlg->p_add_class_name = c->get_name();
          dlg->gc.define_cursor(XC_crosshair);
        }
      catch (const std::exception & ex)
        {
          std::cerr << "ERROR: " << ex.what() << std::endl;
        }
      break;
    
    case idPAddClass: {
      std::list<OksClass *> exclude_list;
      const OksClass::Map & oks_classes = dlg->parent()->classes();

      for(OksClass::Map::const_iterator i = oks_classes.begin(); i != oks_classes.end(); ++i) {
        for(std::list<Item *>::const_iterator j = dlg->p_items.begin(); j != dlg->p_items.end(); ++j) {
          if((*j)->get_type() == Item::Class) {
            const Class *c = static_cast<const Class *>(*j);
	    OksClass * oks_class = c->get_oks_class();
	    if(oks_class == i->second) {
	      exclude_list.push_back(oks_class);
	      break;
	    }
          }
        }
      }

      dlg->parent()->create_select_class_dlg("Add Class", &exclude_list, putCB, (XtPointer)dlg);

      break; }
      
    case idPRemoveClass:
      dlg->remove_selected_class();
      break;

    case idPSaveAs: {
      std::string s = OksXm::ask_file(
        dlg->get_form_widget(),
	"Input name of data file",
	"Selection of file to store view",
	0, "*.view.xml"
      );

      if(s.length()) {
        if(dlg->save(s) == 0) {
	  dlg->p_file = s;
	  XtSetSensitive(dlg->get_widget(idPSave), true);
	  XtSetSensitive(dlg->get_widget(idPRevertToSaved), true);
	}
      }
      break; }
    
    case idPSave:
      dlg->save(dlg->p_file);
      break;
    
    case idPRevertToSaved:
      while(!dlg->p_items.empty()) {
        Item * item = dlg->p_items.front();
        dlg->p_items.pop_front();
        delete item;
      }
      dlg->read_data();
      dlg->p_selected_item = 0;
      dlg->draw(0);
      break;

    case idPRename: {
      std::string s = OksXm::ask_name(
        dlg->get_form_widget(),
	"Input name of \'View\' dialog",
	"Rename View",
	dlg->p_name.c_str()
      );

      if(s.length() && dlg->p_name != s) {
        dlg->p_name = s;
        dlg->update_title();
        mainDlg->update_menu_bar();
      }

      break; }

    case idPParameters:
      dlg->gc.create_parameters_dialog(dlg->parent(), dlg->p_name, refresh, (XtPointer)dlg);
      break;

    case idPPrint:
      dlg->gc.create_print_dialog(dlg->parent(), dlg->p_name, refresh, (XtPointer)dlg, dlg->parent()->schema_files());
      break;
  }
}

void 
OksSchemaEditorViewDialog::update_title() const
{
  std::string title("View \'");
  title += p_name;
  title += '\'';
  setTitle(title.c_str());
}

static void
set_label_pixmap(Widget w, const char ** ps)
{
  if(ps) {
    Pixmap pxm = OksXm::create_color_pixmap(mainDlg->get_form_widget(), ps);
    if(pxm) {
      XtVaSetValues(w, XmNlabelPixmap, pxm, XmNlabelType, XmPIXMAP, NULL);
    }
  }
}


void
OksSchemaEditorViewDialog::add_tool_button(const char * s, const char ** ps, long id, bool is_first)
{
  Widget w = add_radio_button(id, s);
  p_tool_widgets.push_back(w);
  OksXm::set_user_data(w, (XtPointer)id);
  XtAddCallback(w, XmNvalueChangedCallback, toolCB, (XtPointer)this); 
  set_label_pixmap(w, ps);

  if(is_first == false) attach_previous(id, 0xFFFF, false);
  else XmToggleButtonSetState(w, true, false);

  p_label_tips.push_back(OksXm::show_label_as_tips(w));
}

void
OksSchemaEditorViewDialog::add_menu_button(const char * s, const char ** ps, long id, Dimension dx, bool is_first)
{
  Widget w = add_push_button(id, s);
  OksXm::set_user_data(w, (XtPointer)id);
  XtAddCallback(w, XmNactivateCallback, menuCB, (XtPointer)this);
  set_label_pixmap(w, ps);

  if(is_first == false) attach_previous(id, dx, false);

  p_label_tips.push_back(OksXm::show_label_as_tips(w));
}

void
OksSchemaEditorViewDialog::set_tool_state(Widget w) const
{
  for(std::list<Widget>::const_iterator i = p_tool_widgets.begin(); i != p_tool_widgets.end(); ++i) {
    XmToggleButtonSetState(*i, (*i == w), false);
  }
}


#include "select.xpm"
#include "move.xpm"
#include "link2.xpm"
#include "glink.xpm"
#include "new_class.xpm"
#include "add_class.xpm"
#include "remove.xpm"
#include "save.xpm"
#include "save_as.xpm"
#include "revert.xpm"
#include "rename.xpm"
#include "params.xpm"
#include "print.xpm"

void
OksSchemaEditorViewDialog::init()
{
  p_name           = "Unknown";
  p_tool           = idPSelect;
  p_selected_item  = 0;

  p_looking4place  = false;
  p_add_class_name = "";

  setIcon((OksDialog *)parent());

  add_tool_button("Select", select_xpm, idPSelect, true);
  add_tool_button("Move Picture", move_xpm, idPMove);
  add_tool_button("Set Relationships", link2_xpm, idPLink2);
  add_tool_button("Set Inheritance", glink_xpm, idPGLink);

  add_menu_button("Create New Class", new_class_xpm, idPNewClass);
  add_menu_button("Add Existing Class", add_class_xpm, idPAddClass, 0);
  add_menu_button("Remove Class From View", remove_xpm, idPRemoveClass, 0);
  add_menu_button("Save", save_xpm, idPSave);
  add_menu_button("Save As ...", save_as_xpm, idPSaveAs, 0);
  add_menu_button("Revert To Saved", revert_xpm, idPRevertToSaved, 0);
  add_menu_button("Set Name", rename_xpm, idPRename);
  add_menu_button("Print", print_xpm, idPPrint);
  add_menu_button("Parameters", params_xpm, idPParameters);

  add_form(idDrawingForm, "Schema");
  attach_right(idDrawingForm);

  p_drawing_area = XmCreateDrawingArea(
    get_form(idDrawingForm)->get_form_widget(),
    (char *)"View",
    0,
    0
  );

  XtVaSetValues(p_drawing_area,
    XmNtopAttachment, XmATTACH_FORM,
    XmNtopOffset, 0,
    XmNbottomAttachment, XmATTACH_FORM,
    XmNbottomOffset, 0,
    XmNleftAttachment, XmATTACH_FORM,
    XmNleftOffset, 0,
    XmNrightAttachment, XmATTACH_FORM,
    XmNrightOffset, 0,
    XmNmarginHeight, 0,
    XmNmarginWidth, 0,
    XtNwidth, 400,
    XtNheight, 200,
    XmNresizePolicy, XmRESIZE_NONE,
    NULL
  );

  XtManageChild(p_drawing_area);


    // save the pointer to graphic window

  OksXm::set_user_data(p_drawing_area, (XtPointer)this);

     // add mouse wheel actions on the drawing area

  OksXm::add_mouse_wheel_action(p_drawing_area); // mouse wheel actions on the drawing area

  XtAddCallback(p_drawing_area, XmNexposeCallback, drawCB, (XtPointer)this);
  XtAddCallback(p_drawing_area, XmNinputCallback,  inputCB, (XtPointer)this);

    // process motion events

  XtAddEventHandler(p_drawing_area, PointerMotionMask, False, motionAC, (XtPointer)this);
  XtAddEventHandler(XtParent(XtParent(p_drawing_area)), PointerMotionMask, False, motionAC, (XtPointer)this);

    // process button press events

  XtAddEventHandler(XtParent(XtParent(p_drawing_area)), ButtonPressMask, False, buttonAC, (XtPointer)this);

  add_separator();

  XtAddCallback(addCloseButton(), XmNactivateCallback, closeCB, (XtPointer)this);
  OksXm::set_close_cb(XtParent(get_form_widget()), closeCB, (void *)this);

  attach_bottom(idDrawingForm);

  show();

  gc.init(p_drawing_area);

  setCascadePos();


    // set default parameters

  gc.get_params() = OksGC::Parameters::def_params();
}

OksSchemaEditorViewDialog::OksSchemaEditorViewDialog(const std::string & file, OksSchemaEditorMainDialog * p) :
  OksSchemaEditorDialog	(p),
  p_file                (file)
{
  init();
  read_data();
  draw(0);
}

OksSchemaEditorViewDialog::OksSchemaEditorViewDialog(OksSchemaEditorMainDialog * p) :
  OksSchemaEditorDialog	(p)
{
  init();

  XtSetSensitive(get_widget(idPSave), false);
  XtSetSensitive(get_widget(idPRevertToSaved), false);
  XtSetSensitive(get_widget(idPRemoveClass), false);

  update_title();
}


OksSchemaEditorViewDialog::~OksSchemaEditorViewDialog()
{
  parent()->remove_dlg(this);

  while(!p_label_tips.empty()) {
    OksXm::TipsInfo * p = p_label_tips.front();
    p_label_tips.pop_front();
    delete p;
  }
}


void
OksSchemaEditorViewDialog::read_data()
{
  const char * fname = "OksSchemaEditorViewDialog::read_data()";

  try {

    OksKernel kernel(false, false, false, false);
    kernel.set_silence_mode(true);

    std::string dummu_file = OksKernel::get_tmp_file("/tmp/dummu-view.schema.xml");

    kernel.new_schema(dummu_file);

    OksViewSchema schema_view;
    define_schema(schema_view, &kernel);

    kernel.load_data(p_file);


      // read name

    const OksObject::Map * objs = schema_view.view_c->objects();

    if(objs && objs->size() != 0) {
      OksObject * obj = (*(objs->begin())).second;
      p_name = obj->GetId();

      gc.get_params().set_drawing_area_x_margin(obj->GetAttributeValue("drawing area x margin")->data.U16_INT);
      gc.get_params().set_drawing_area_y_margin(obj->GetAttributeValue("drawing area y margin")->data.U16_INT);
      gc.get_params().set_font_x_margin(obj->GetAttributeValue("font x margin")->data.U16_INT);
      gc.get_params().set_font_y_margin(obj->GetAttributeValue("font y margin")->data.U16_INT);
      gc.get_params().set_graphical_parameter("Aggregation sign size", obj->GetAttributeValue("aggregation sign size")->data.U16_INT);
      gc.get_params().set_graphical_parameter("Generalization sign size", obj->GetAttributeValue("generalization sign size")->data.U16_INT);
      gc.get_params().set_font_name(*obj->GetAttributeValue("font name")->data.STRING);

      gc.reset();
    }

    update_title();


      // create classes

    objs = schema_view.class_c->objects();

    if(objs) for(OksObject::Map::const_iterator i = objs->begin(); i != objs->end(); ++i) {
      OksObject * obj = (*i).second;

      short x(obj->GetAttributeValue("x")->data.S16_INT);
      short y(obj->GetAttributeValue("y")->data.S16_INT);

      if(OksClass * oks_class = parent()->find_class(obj->GetId())) {
        add(new Class(x, y, oks_class));
      }
      else {
        Oks::error_msg(fname) << "Class \'" << obj->GetId() << "\' stored in view is not defined\n";
      }
    }

      // put links on place

    OksClass * classes[] = {
      schema_view.glink_c,
      schema_view.alink_c
    };

    for(unsigned short j = 0; j < sizeof(classes) / sizeof(OksClass *); ++j) {
      objs = classes[j]->objects();

      if(objs) for(OksObject::Map::const_iterator i = objs->begin(); i != objs->end(); ++i) {
        OksObject * obj = (*i).second;


          // process links

        const std::string * from_name = 0;
        const std::string * to_name   = 0;
        const std::string * rel_name  = 0;

        OksData * d(obj->GetRelationshipValue("From"));

        if(d->type == OksData::object_type && d->data.OBJECT) from_name = &d->data.OBJECT->GetId();
        if(from_name == 0 || from_name->empty()) {
          Oks::error_msg(fname) << "Failed read \'From\' relationship of object " << obj << std::endl;
          continue;
        }

        d = obj->GetRelationshipValue("To");
        if(d->type == OksData::object_type && d->data.OBJECT) to_name = &d->data.OBJECT->GetId();
        if(to_name == 0 || to_name->empty()) {
          Oks::error_msg(fname) << "Failed read \'To\' relationship of object " << obj << std::endl;
          continue;
        }

        if(obj->GetClass() == schema_view.alink_c) {
    	  rel_name = obj->GetAttributeValue("Name")->data.STRING;
        }
 
          // find link

        for(std::list<Item *>::const_iterator i = p_items.begin(); i != p_items.end(); ++i) {
          if((*i)->get_type() == Item::Relationship) {
  	    const Link * l = static_cast<const Link *>(*i);

	    if(
	     (l->get_class_from()->get_oks_class()->get_name() == *from_name) &&
	     (l->get_class_to()->get_oks_class()->get_name() == *to_name) &&
	     (
	       (rel_name == 0) ||
	       (l->get_oks_relationship() && l->get_oks_relationship()->get_name() == *rel_name)
	     )
	    ) {
	      d = obj->GetRelationshipValue("Points");
	    
	      if(d->data.LIST) {
	        OksData::List::const_iterator i2 = d->data.LIST->begin();
	        std::list<Point *>::const_iterator j2 = l->get_points().begin();
	        
	        while(i2 != d->data.LIST->end()) {
	          if((*i2)->type == OksData::object_type) {
		    OksObject * p_obj = (*i2)->data.OBJECT;
		    Point * p = *j2;

                    p->set_x(p_obj->GetAttributeValue("x")->data.S16_INT);
                    p->set_y(p_obj->GetAttributeValue("y")->data.S16_INT);

		    ++i2; ++j2;
		  }
	        }
	      }

	      break;
	    }
	  }
        }
      }
    }

    if(unlink(dummu_file.c_str()) != 0) {
      Oks::error_msg(fname) << "Failed to unlink helper schema file \"" << dummu_file << "\"\n";
    }

  }
  catch (oks::exception & ex) {
    Oks::error_msg(fname) << "Failed to read data: " << ex.what() << std::endl;
  }
}

void 
OksSchemaEditorViewDialog::update_class(const OksClass * oks_class, bool remove_from_view)
{
  const char * vf_out_str = "VERBOSE [OksSchemaEditorViewDialog::update_class()]:\n";

  Class * c = 0; // pointer to view class
  std::set<OksRelationship *, std::less<OksRelationship *> > shown_rels;
  std::set<std::string, std::less<std::string> > shown_inhr;

  for(std::list<Item *>::iterator i = p_items.begin(); i != p_items.end();) {
    if((*i)->get_type() == Item::Class) {
      Class * cc = static_cast<Class *>(*i);
      if(cc->get_oks_class() == oks_class) {
        c = cc;
	if(remove_from_view) {
	  if(p_selected_item == *i) p_selected_item = 0;
	  i = p_items.erase(i);
	  delete c;
	  continue;
	}
      }
    }
    else {
      Link * l = static_cast<Link *>(*i);
      if(
       (l->get_class_from()->get_oks_class() == oks_class) ||
       (l->get_class_to()->get_oks_class() == oks_class)
      ) {

          // remove link if oks class was removed

        if(remove_from_view) {
	  if(p_selected_item == *i) p_selected_item = 0;
          i = p_items.erase(i);
	  delete l;
	  continue;
        }


          // make set of shown relationships
          // consider case when the relationship was removed or changed

	if(l->get_class_from()->get_oks_class() == oks_class) {
	  OksRelationship * r = l->get_oks_relationship();

	  if(r) {
	    const std::list<OksRelationship *> * rels = oks_class->direct_relationships();
	    if(rels) {
	      for(std::list<OksRelationship *>::const_iterator j = rels->begin(); j != rels->end(); ++j) {
	        if((*j == r) && (r->get_class_type() == l->get_class_to()->get_oks_class())) {
		  shown_rels.insert(r);
		  l->set_multiplicity_text(); // could be changed ?
		  r = 0; // indicates that the relationship was found
		  break;
		}
	      }
	    }

            if(r) {
	      if(p_selected_item == *i) p_selected_item = 0;
              i = p_items.erase(i);
	      delete l;
	      continue;
            }
	  }
	}

          // consider case when the generalization was removed

	if(l->get_class_to()->get_oks_class() == oks_class) {
	  OksRelationship * r = l->get_oks_relationship();
	  if(!r) {
	    if(oks_class->has_direct_super_class(l->get_class_from()->get_oks_class()->get_name()) == false) {
	      if(parent()->is_verbouse_out()) {
	        std::cout << vf_out_str << " remove generalization link from \'"
			  << l->get_class_from()->get_oks_class()->get_name()
			  << "\' to \'" << oks_class->get_name() << "\'\n";
              }
              if(p_selected_item == *i) p_selected_item = 0;
	      i = p_items.erase(i);
	      delete l;
	      continue;
	    }
	    else {
              shown_inhr.insert(l->get_class_from()->get_oks_class()->get_name());
	    }
	  }
	}
      }
    }
    ++i;
  }

  if(c == 0) return;


    // consider the case when new relationship appeared

  if(remove_from_view == false) {
    const std::list<OksRelationship *> * rels = oks_class->direct_relationships();

    if(rels) {
      for(std::list<OksRelationship *>::const_iterator j = rels->begin(); j != rels->end(); ++j) {
        if(shown_rels.find(*j) == shown_rels.end()) {
          for(std::list<Item *>::iterator i = p_items.begin(); i != p_items.end(); ++i) {
            if((*i)->get_type() == Item::Class) {
              Class * cc = static_cast<Class *>(*i);
              if((*j)->get_class_type() == cc->get_oks_class()) {
	        if(parent()->is_verbouse_out()) {
	          std::cout << vf_out_str << " add new Link(\'" << c->get_oks_class()->get_name()
		            << "\', \'" << cc->get_oks_class()->get_name()
			    << "\', \'" << (*j)->get_name() <<"\')\n";
		}
                p_items.push_back(new Link(c, cc, *j));
	        break;
	      }
	    }
          }
        }
      }
    }
  }


    // consider the case when new generalization link appeared

  if(remove_from_view == false) {
    const std::list<std::string *> * links = oks_class->direct_super_classes();

    if(parent()->is_verbouse_out()) {
      std::cout << vf_out_str << " superclasses of \'" << oks_class->get_name() << "\' are:\n";
      if(links) for(std::list<std::string *>::const_iterator j = links->begin(); j != links->end(); ++j) {
        std::cout << "  - \'" << *(*j) << "\'\n";
      }

      std::cout << " shown links are:\n";
      for(std::set<std::string>::const_iterator j2 = shown_inhr.begin(); j2 != shown_inhr.end(); ++j2) {
        std::cout << "  - \'" << *j2 << "\'\n";
      }
    }

    if(links) {
      for(std::list<std::string *>::const_iterator j = links->begin(); j != links->end(); ++j) {
        if(shown_inhr.find(*(*j)) == shown_inhr.end()) {
          for(std::list<Item *>::iterator i = p_items.begin(); i != p_items.end(); ++i) {
            if((*i)->get_type() == Item::Class) {
              Class * cc = static_cast<Class *>(*i);
              if(*(*j) == cc->get_oks_class()->get_name()) {
                if(parent()->is_verbouse_out()) {
                  std::cout << vf_out_str << " add new Link(\'" << cc->get_oks_class()->get_name()
		            << "\', \'" << c->get_oks_class()->get_name() << "\')\n";
                }
                p_items.push_back(new Link(cc, c, 0));
	        break;
	      }
	    }
          }
	}
      }
    }
  }

  draw(0);
}

void
OksSchemaEditorViewDialog::add(Class *c)
{
  p_items.push_back(c);

  std::list<Link *> created;

  c->draw(this);

  for(std::list<Item *>::const_iterator i = p_items.begin(); i != p_items.end(); ++i) {
    if((*i)->get_type() == Item::Class) {
      const Class * cc = static_cast<const Class *>(*i);
      OksClass * oks_class = cc->get_oks_class();

      {
          // find new relationships pointing from existing classes to c 

        const std::list<OksRelationship *> * rels = oks_class->direct_relationships();
        std::list<OksRelationship *>::const_iterator j;
        if(rels) for(j = rels->begin(); j != rels->end(); ++j) {
          if((*j)->get_class_type() == c->get_oks_class()) {
	    created.push_back(new Link(cc, c, *j));
	  }
        }

          // find new relationships from c to existing classes (except self-references)

        rels = c->get_oks_class()->direct_relationships();
        if(rels) for(j = rels->begin(); j != rels->end(); ++j) {
          if((c != cc) && ((*j)->get_class_type() == oks_class)) {
	    created.push_back(new Link(c, cc, *j));
	  }
        }
      }

      {
          // find superclasses having given one as superclass

        const std::list<std::string *> * direct_classes = oks_class->direct_super_classes();
        std::list<std::string *>::const_iterator j;
        if(direct_classes) for(j = direct_classes->begin(); j != direct_classes->end(); ++j) {
          if(*(*j) == c->get_oks_class()->get_name()) {
	    created.push_back(new Link(c, cc));
	    break; // the same supreclass is the only one
	  }
        }

          // find direct subclasses of given one

        direct_classes = c->get_oks_class()->direct_super_classes();
        if(direct_classes) for(j = direct_classes->begin(); j != direct_classes->end(); ++j) {
          if(*(*j) == cc->get_oks_class()->get_name()) {
	    created.push_back(new Link(cc, c));
	    break; // the same supreclass is the only one
	  }
        }
      }
    }
  }

  while(!created.empty()) {
    Link * r = created.front();
    created.pop_front();
    p_items.push_front(r);
  }
}

void
OksSchemaEditorViewDialog::remove_selected_class()
{
  if(p_selected_item && p_selected_item->get_type() == Item::Class) {
    Class * c = static_cast<Class *>(p_selected_item);
    update_class(c->get_oks_class(), true);
    XtSetSensitive(get_widget(idPRemoveClass), false);
  }
}

void
OksSchemaEditorViewDialog::draw(XExposeEvent * exposeEvent)
{
    // the constants below are used when draw links

  p_aggregation_sign_size = gc.get_params().get_u16_parameter("Aggregation sign size");
  p_generalization_sign_size = gc.get_params().get_u16_parameter("Generalization sign size");


  short x, y;
  unsigned short w2 = 0, h2 = 0;
  Dimension width, height;

  if(exposeEvent) {
    x = exposeEvent->x;
    y = exposeEvent->y;
    width = exposeEvent->width;
    height = exposeEvent->height;
  }
  else {
    x = y = 0;
    XtVaGetValues(p_drawing_area, XtNwidth, &width, XtNheight, &height, NULL);
  }

  gc.clean_rectangle(x, y, width, height);

  for(std::list<Item *>::const_iterator i = p_items.begin(); i != p_items.end(); ++i) {
    if(exposeEvent == 0 || (*i)->is_inside(x, y, width, height)) {
      (*i)->draw(this);
    }

    short rx, ry;
    unsigned short rw, rh;
    (*i)->get_rect(rx, ry, rw, rh);
    short mx = rx + rw;
    short my = ry + rh;
    if(w2 < mx) w2 = mx;
    if(h2 < my) h2 = my;
  }

  h2 += gc.get_params().get_drawing_area_y_margin();
  w2 += gc.get_params().get_drawing_area_x_margin();


    // reset size of window when new and current sizes are different

  {
    Arg	args[2];
    unsigned short n = 0;

    if(w2 != width)  { XtSetArg(args[n], XtNwidth, w2); n++; }
    if(h2 != height) { XtSetArg(args[n], XtNheight, h2); n++; }
    if(n)            { XtSetValues(p_drawing_area, args, n); }
  }
}

Item *
OksSchemaEditorViewDialog::get_item(short x, short y)
{
    // process lines first

  for(std::list<Item *>::const_iterator i = p_items.begin(); i != p_items.end(); ++i) {
    if((*i)->does_select(x, y)) {
      return *i;
    }
  }

  return 0;
}

void
OksSchemaEditorViewDialog::deselect()
{
  if(p_selected_item) {
    p_selected_item->make_selected(this, false);
    p_selected_item->draw(this, Item::Deselect);
    p_selected_item->process_deselect_event();
    p_selected_item = 0;
  }
}

void
OksSchemaEditorViewDialog::deshape()
{
  if(p_selected_item) {
    p_selected_item->process_deselect_event();
    p_selected_item->draw(this, Item::Deshape);
    p_selected_item = 0;
  }
}

void
OksSchemaEditorViewDialog::define_schema(OksViewSchema & view_schema, OksKernel * kernel)
{
  try
    {
      view_schema.view_c = new OksClass("View", kernel);
      view_schema.view_c->add(new OksAttribute("drawing area x margin", OksAttribute::u16_int_type, false, "", "6", "", false));
      view_schema.view_c->add(new OksAttribute("drawing area y margin", OksAttribute::u16_int_type, false, "", "4", "", false));
      view_schema.view_c->add(new OksAttribute("font x margin", OksAttribute::u16_int_type, false, "", "3", "", false));
      view_schema.view_c->add(new OksAttribute("font y margin", OksAttribute::u16_int_type, false, "", "2", "", false));
      view_schema.view_c->add(new OksAttribute("font name", OksAttribute::string_type, false, "", "", "", false));
      view_schema.view_c->add(new OksAttribute("aggregation sign size", OksAttribute::u16_int_type, false, "", "18", "", false));
      view_schema.view_c->add(new OksAttribute("generalization sign size", OksAttribute::u16_int_type, false, "", "16", "", false));

      view_schema.class_c = new OksClass("Class", kernel);
      view_schema.class_c->add(new OksAttribute("x", OksAttribute::s16_int_type, false, "", "", "", false));
      view_schema.class_c->add(new OksAttribute("y", OksAttribute::s16_int_type, false, "", "", "", false));

      view_schema.point_c = new OksClass("Point", kernel);
      view_schema.point_c->add(new OksAttribute("x", OksAttribute::s16_int_type, false, "", "", "", false));
      view_schema.point_c->add(new OksAttribute("y", OksAttribute::s16_int_type, false, "", "", "", false));

      view_schema.glink_c = new OksClass("GLink", kernel);
      view_schema.glink_c->add(new OksRelationship("From", "Class", OksRelationship::One, OksRelationship::One, false, false, false, ""));
      view_schema.glink_c->add(new OksRelationship("To", "Class", OksRelationship::One, OksRelationship::One, false, false, false, ""));
      view_schema.glink_c->add(new OksRelationship("Points", "Point", OksRelationship::One, OksRelationship::Many, false, false, false, ""));

      view_schema.alink_c = new OksClass("ALink", kernel);
      view_schema.alink_c->add_super_class("GLink");
      view_schema.glink_c->add(new OksAttribute("Name", OksAttribute::string_type, false, "", "", "", false));
    }
  catch (std::exception& ex)
    {
      Oks::error_msg("OksSchemaEditorViewDialog::define_schema()") << ex.what() << std::endl;
    }
}


int
OksSchemaEditorViewDialog::save_schema(const std::string & s)
{
  try {
    OksKernel kernel(false, false, false, false);

    OksFile * file_h = kernel.new_schema(s);

    OksViewSchema schema_view;
    define_schema(schema_view, &kernel);

    kernel.save_schema(file_h);
  }
  catch (oks::exception & ex) {
    std::cerr << "ERROR: cannot save schema file \'" << s << "\': " << ex.what() << std::endl;
    return 1;
  }

  return 0;
}


int
OksSchemaEditorViewDialog::save(const std::string & file) const
{
  if(file.empty()) {
    Oks::error_msg("OksSchemaEditorViewDialog::Save()") << "File name is empty\n";
    return 1;
  }

  int status = 0;

  try {

      // define the schema

    OksKernel kernel(false, false, false, false);
    kernel.set_silence_mode(true);

    std::string dummu_file = OksKernel::get_tmp_file("/tmp/dummu-view.schema.xml");

    kernel.new_schema(dummu_file);

    OksFile * file_h = kernel.new_data(file);

    OksViewSchema schema_view;
    define_schema(schema_view, &kernel);


      // save dialog name and graphic context parameters

    {
      OksObject * obj = new OksObject(schema_view.view_c, p_name.c_str());

      {
        OksData d(gc.get_params().get_drawing_area_x_margin());
        obj->SetAttributeValue("drawing area x margin", &d);
      }

      {
        OksData d(gc.get_params().get_drawing_area_y_margin());
        obj->SetAttributeValue("drawing area y margin", &d);
      }

      {
        OksData d(gc.get_params().get_font_x_margin());
        obj->SetAttributeValue("font x margin", &d);
      }

      {
        OksData d(gc.get_params().get_font_y_margin());
        obj->SetAttributeValue("font y margin", &d);
      }

      {
        OksData d(gc.get_params().get_u16_parameter("Aggregation sign size"));
        obj->SetAttributeValue("aggregation sign size", &d);
      }

      {
        OksData d(gc.get_params().get_u16_parameter("Generalization sign size"));
        obj->SetAttributeValue("generalization sign size", &d);
      }

      {
        OksData d(gc.get_params().get_font_name());
        obj->SetAttributeValue("font name", &d);
      }
    }


    for(std::list<Item *>::const_iterator i = p_items.begin(); i != p_items.end(); ++i) {
      (*i)->save(schema_view);
    }

    kernel.save_data(file_h, true);

    std::cout << "Save view layout to \"" << file << "\"\n";

    if(unlink(dummu_file.c_str()) != 0) {
      Oks::error_msg("OksSchemaEditorViewDialog::Save()") << "Failed to unlink helper schema file \"" << dummu_file << "\"\n";
    }
  }
  catch (oks::exception & ex) {
    Oks::error_msg("OksSchemaEditorViewDialog::Save()") << "Caught oks exception: " << ex.what() << std::endl;
    status = 1;
  }

  return status;
}


void
OksSchemaEditorViewDialog::input(XButtonEvent * event)
{
  if(p_looking4place == false) {
    Item * item = get_item(event->x, event->y);

    if(event->button == Button1) {
      if(p_tool == idPMove && (event->state & Button1Mask) == 0) {
        p_selected_item_x = event->x;
        p_selected_item_y = event->y;
      }
      else if(p_tool == idPLink2 || p_tool == idPGLink) {
        if(item) {
	  if(item->get_type() == Item::Relationship) {
	    Link * link = static_cast<Link *>(item);
	    if(
	      (event->state & Button1Mask) == 0 &&
	      (
	       (link->get_oks_relationship() != 0 && p_tool == idPLink2) ||
	       (link->get_oks_relationship() == 0 && p_tool == idPGLink)
	      )
	    ) {
	      if(p_selected_item) {
	        if(p_selected_item == item) {
	          if(link->does_reshape(event->x, event->y)) {
                    p_selected_item_x = link->get_reshape_point()->get_x();
                    p_selected_item_y = link->get_reshape_point()->get_y();
	          }
	        }
		else {
		  deshape();
		  p_selected_item = item;
		  link->set_reshaped(true);
		  link->draw(this, Item::Normal);
		}
	      }
	      else {
	        p_selected_item = item;
		link->set_reshaped(true);
		link->draw(this, Item::Normal);
	      }
	    }
	    else if(item != p_selected_item) {
	      gc.draw_selection_line(0, 0, 0, 0, true); // hide line
	      deshape();
	    }
	  }
          else if(item->get_type() == Item::Class) {
	    if((event->state & Button1Mask) == 0) {
	      if(p_selected_item && item != p_selected_item) {
	        deshape();
	      }

	      p_selected_item = item;
              p_selected_item_x = event->x;
              p_selected_item_y = event->y;
	    }
	    else {
	      gc.draw_selection_line(0, 0, 0, 0, true); // hide line

	      if(p_selected_item) {
	        OksClass * c2 = (static_cast<Class *>(item))->get_oks_class();

	        if(p_selected_item->get_type() == Item::Class) {
	          OksClass * c1 = (static_cast<Class *>(p_selected_item))->get_oks_class();

		  if(c1->get_file()->is_read_only()) {
		    std::cerr << "ERROR: class \'" << c1->get_name() << "\' is read-only\n";
		  }
		  else {
                    if(p_tool == idPLink2) {
                      std::string text("Input name of new relationship from\n\'");
	              text += c1->get_name();
	              text += "\' class to \'";
	              text += c2->get_name();
	              text += "\' class";

                      std::string s = OksXm::ask_name(
                        get_form_widget(),
                        text.c_str(),
                        "New OKS Relationship"
                      );

                      if(s.length()) {
	                OksRelationship * r = new OksRelationship(s);
                        try {
		          r->set_type(c2->get_name());
                          c1->add(r);
                        }
                        catch(oks::exception& ex) {
                          Oks::error_msg("OksSchemaEditorViewDialog::input()") << ex.what() << std::endl;
                          delete r;
                        }
	              }
	            }
	            else {
                      try {
	                c1->add_super_class(c2->get_name());
                      }
                      catch(oks::exception& ex) {
                        Oks::error_msg("OksSchemaEditorViewDialog::input()") << ex.what() << std::endl;
                      }
	            }
		  }
	        }
		else {
		  Link * link = static_cast<Link *>(p_selected_item);

                  if(link->get_reshape_point() == link->get_reshape_point_from()) {
		    if(link->get_class_to()->get_oks_class() != c2) {
		      if(p_tool == idPLink2) {
		        OksClass * c1 = link->get_class_from()->get_oks_class();
		        if(c1->get_file()->is_read_only()) {
			  std::cerr << "ERROR: class \'" << c1->get_name() << "\' is read-only\n";
			}
			else {
		          OksRelationship * r = link->get_oks_relationship();
                          try {
			    r->set_type(c2->get_name());
                          }
                          catch(oks::exception& ex) {
                            Oks::error_msg("OksSchemaEditorViewDialog::input()") << ex.what() << std::endl;
                          }
		        }
		      }
		      else {
		        OksClass * c1 = link->get_class_to()->get_oks_class();
		        const std::string & bc_name = link->get_class_from()->get_oks_class()->get_name();

			if(c2->get_file()->is_read_only()) {
			  std::cerr << "ERROR: class \'" << c2->get_name() << "\' is read-only\n";
			}
			else if(c1->get_file()->is_read_only()) {
			  std::cerr << "ERROR: class \'" << c1->get_name() << "\' is read-only\n";
			}
                        else {
                          try {
                            c2->add_super_class(bc_name);
                            c1->remove_super_class(bc_name);
                          }
                          catch(oks::exception& ex) {
                            Oks::error_msg("OksSchemaEditorViewDialog::input()") << ex.what() << std::endl;
                          }
                        }
		      }
		    }
		  }
		  else {
		    if(link->get_class_from()->get_oks_class() != c2) {
		      if(p_tool == idPLink2) {
		        OksRelationship * r = link->get_oks_relationship();
			OksClass * c1 = link->get_class_from()->get_oks_class();

			if(c2->get_file()->is_read_only()) {
			  std::cerr << "ERROR: class \'" << c2->get_name() << "\' is read-only\n";
			}
			else if(c1->get_file()->is_read_only()) {
			  std::cerr << "ERROR: class \'" << c1->get_name() << "\' is read-only\n";
			}
			else if(c2->find_direct_relationship(r->get_name())) {
			  std::cerr << "ERROR: class \'" << c2->get_name()
			            << "\' already has direct relationship with name \'"
				    << r->get_name() << "\'\n";
			}
			else {
                          try {
			    c1->remove(r, false);
			    c2->add(r);
			  }
                          catch(oks::exception& ex) {
                            Oks::error_msg("OksSchemaEditorViewDialog::input()") << ex.what() << std::endl;
                          }
			}
		      }
		      else {
		        OksClass * dc = link->get_class_to()->get_oks_class(); // derived class

			if(dc->get_file()->is_read_only()) {
			  std::cerr << "ERROR: class \'" << dc->get_name() << "\' is read-only\n";
			}
			else {
                          try {
                            dc->add_super_class(c2->get_name());
                            dc->remove_super_class(link->get_class_from()->get_oks_class()->get_name());
                          }
                          catch(oks::exception& ex) {
                            Oks::error_msg("OksSchemaEditorViewDialog::input()") << ex.what() << std::endl;
                          }
			}
		      }
		    }
		  }
		}
	      }

              deshape();

              if(p_selected_item) {
	      	p_selected_item->process_deselect_event();
	        p_selected_item->draw(this, Item::Deshape);
	        p_selected_item = 0;
	      }
	    }
	  }
        }
	else {
	  gc.draw_selection_line(0, 0, 0, 0, true); // hide line
	  deshape();
	}
      }
      else if(p_tool == idPSelect && (event->state & Button1Mask) == 0) {
        if((item == 0 && p_selected_item) || (p_selected_item != 0 && item != p_selected_item)) {
          deselect();
        }

        if(item) {
          p_selected_item_x = event->x;
          p_selected_item_y = event->y;
          p_selected_item = item;
          p_selected_item->make_selected(this, true);
          p_selected_item->process_select_event(event);
          p_selected_item->draw(this, Item::Select);
        }
      }
    }
  }
  else {
    gc.undefine_cursor();
    p_looking4place = false;

      // create new class in the view

    Class * new_class = new Class(
      event->x,
      event->y,
      parent()->find_class(p_add_class_name)
    );

    if(new_class) {
      deselect();
      new_class->make_selected(this, true);
      add(new_class);
      new_class->draw(this);
      p_selected_item = new_class;
      draw(0);
    }
  }
}

void
OksSchemaEditorViewDialog::move(XMotionEvent * event)
{
  if(p_tool == idPMove && event->state != 0) {
    if(event->x != p_selected_item_x || event->y != p_selected_item_y) {
      short dx = event->x - p_selected_item_x;
      short dy = event->y - p_selected_item_y;

      for(std::list<Item *>::const_iterator i = p_items.begin(); i != p_items.end(); ++i) {
        (*i)->shift(dx, dy);
      }

      p_selected_item_x = event->x;
      p_selected_item_y = event->y;
      
      draw(0);
    }
  }
  else if(p_tool == idPLink2 || p_tool == idPGLink) {
    if(p_selected_item) {
      if(p_selected_item->get_type() == Item::Relationship) {
        if(static_cast<Link *>(p_selected_item)->get_reshape_point() == 0) return;
      }
      gc.draw_selection_line(p_selected_item_x, p_selected_item_y, event->x, event->y, false);
    }
  }
  else if(p_tool == idPSelect) {

    if(p_selected_item && p_selected_item->get_type() == Item::Relationship && event->state != 0) {
      (static_cast<Link *>(p_selected_item))->move_handle(event->x, event->y);
      draw(0);
    }

    if(p_selected_item && p_selected_item->get_type() == Item::Class && event->state != 0) {
      Class * selected_class = static_cast<Class *>(p_selected_item);
      if((p_selected_item_x != event->x) || (p_selected_item_y != event->y)) {
        short dx = event->x - p_selected_item_x;
        short dy = event->y - p_selected_item_y;

        short new_x = selected_class->get_x() + dx;
        short new_y = selected_class->get_y() + dy;

        p_selected_item_x = event->x;
        p_selected_item_y = event->y;

        if(new_x < gc.get_params().get_drawing_area_x_margin()) new_x = gc.get_params().get_drawing_area_x_margin();
        if(new_y < gc.get_params().get_drawing_area_y_margin()) new_y = gc.get_params().get_drawing_area_y_margin();

        if(selected_class->get_x() != new_x || selected_class->get_y() != new_y) {
      
            // re-calculate displacement

	  dx = new_x - selected_class->get_x();
	  dy = new_y - selected_class->get_y();

            // calculare rectangle to be updated

          XExposeEvent e;

	  {
	    short xx1, yy1; unsigned short ww1, hh1;
            p_selected_item->get_rect(xx1, yy1, ww1, hh1);
            e.x = xx1; e.y = yy1; e.width = ww1; e.height = hh1;
          }

          e.width += line_w;
          e.height += line_w;

          if(dx > 0) {
            e.width += dx;
          }
          else {
            e.x += dx;
	    e.width -= dx;
          }

          if(dy > 0) {
            e.height += dy;
          }
          else {
            e.y += dy;
	    e.height -= dy;
          }

            // set new position of class

          selected_class->set_x(new_x);
          selected_class->set_y(new_y);


	    // look to links and adjust if necessary

          for(std::list<Item *>::const_iterator i = p_items.begin(); i != p_items.end(); ++i) {
            if((*i)->get_type() == Item::Relationship) {
              const Link *r = static_cast<const Link *>(*i);
	      if(r->get_class_from() == selected_class || r->get_class_to() == selected_class) {
                short rx, ry;
	        unsigned short rw, rh;
	        r->get_rect(rx, ry, rw, rh);

	        if(rx < e.x) {
	          e.width += e.x - rx;
		  e.x = rx;
	        }

	        if((rx + rw) > (e.x + e.width)) {
	          e.width = rx + rw - e.x + line_w;
	        }

	        if(ry < e.y) {
	          e.height += e.y - ry;
		  e.y = ry;
	        }

	        if((ry + rh) > (e.y + e.height)) {
	          e.height = ry + rh - e.y + line_w;
	        }
	      }
            }
	  }


          if(dx > 0) {
	    e.x -= dx;
            e.width += dx;
          }
          else {
	    e.width -= dx;
          }

           if(dy > 0) {
	  e.y -= dy;
            e.height += dy;
          }
          else {
	    e.height -= dy;
          }

          e.width += line_w;
          e.height += line_w;

          if(e.x < 0) e.x = 0;
          if(e.y < 0) e.y = 0;

          draw(&e);
        }
      }
    }
  }
}

void
OksSchemaEditorViewDialog::putCB(Widget w, XtPointer data, XtPointer)
{
  OksSchemaEditorViewDialog * dlg = (OksSchemaEditorViewDialog *) data;
  OksXm::AutoCstring value(OksXm::List::get_selected_value(w));

  dlg->p_add_class_name = value.get();

  OksXm::List::delete_row(w, dlg->p_add_class_name.c_str());

  for (const auto& j : dlg->p_items)
    if (j->get_type() == Item::Class && static_cast<const Class *>(j)->get_oks_class()->get_name() == dlg->p_add_class_name)
      return;

  dlg->p_looking4place = true;
  dlg->gc.define_cursor(XC_crosshair);
}

void
OksSchemaEditorViewDialog::drawCB(Widget, XtPointer c_data, XtPointer call_d)
{
  ((OksSchemaEditorViewDialog *)c_data)->draw(
    (XExposeEvent *)((XmDrawingAreaCallbackStruct *)call_d)->event
  );
}

void OksSchemaEditorViewDialog::buttonAC(Widget, XtPointer c_data, XEvent * event, Boolean *)
{
  ((OksSchemaEditorViewDialog *)c_data)->input((XButtonEvent *)event);
}

void OksSchemaEditorViewDialog::inputCB(Widget, XtPointer c_data, XtPointer callData)
{
  ((OksSchemaEditorViewDialog *)c_data)->input(((XButtonEvent *)(((XmDrawingAreaCallbackStruct *)callData)->event)));
}

void OksSchemaEditorViewDialog::motionAC(Widget, XtPointer c_data, XEvent * event, Boolean *)
{
  ((OksSchemaEditorViewDialog *)c_data)->move((XMotionEvent *)event);
}
