#include <ers/ers.h>

#include <iostream>
#include <sstream>

#include <oks/kernel.h>
#include <oks/class.h>
#include <oks/object.h>
#include <oks/relationship.h>
#include <oks/xm_popup.h>
#include <oks/xm_utils.h>

#include "g_class.h"
#include "g_context.h"
#include "g_object.h"
#include "data_editor.h"
#include "data_editor_exceptions.h"


const unsigned short line_w1 = 1;
const unsigned short line_w2 = 2;

const unsigned short s_object_dx = 4;
const unsigned short s_object_dy = 3;


G_Kid::Propagate
G_Kid::get_propagate_type(const OksClass *oks_class, const OksRelationship * oks_rel)
{
    // check that there are no loops
    // (at least of first level for non-composite links)

  if(!oks_rel->get_is_composite()) {
    const OksClass *r_type = oks_rel->get_class_type();

      // build list of all relationships

    const std::list<OksRelationship *> * rels =  r_type->all_relationships();

    if(rels) {
      for(std::list<OksRelationship *>::const_iterator i = rels->begin(); i != rels->end(); ++i) {
        if((*i)->get_is_composite()) {
	  OksClass * rel_class = (*i)->get_class_type();

          if(rel_class == oks_class) {
            return G_Kid::stop_propagate;
	  }
          else {
            if(const OksClass::FList * sc = oks_class->all_super_classes()) {
              for(OksClass::FList::const_iterator iss = sc->begin(); iss != sc->end(); ++iss) {
                if(rel_class == *iss) {
                  return G_Kid::stop_propagate;
	        }
	      }
            }
          }
	}
      }
    }
  }

  return G_Kid::use_propagate;
}


G_Object::G_Object(OksObject *o, G_Context *gc_, const G_Window *w_, const G_Class *c_, unsigned short x_, unsigned short y_, G_Kid::Propagate p) :
  obj			(o),
  gc			(gc_),
  p_window		(w_),
  g_class		(c_),
  propagate		(p),
  valid			(true),
  shown			(false),
  used			(false),
  dnd_state		(none),
  hiden			(false),
  complex_state		(icon),
  composite_state	(single),
  x			(x_),
  y			(y_),
  w			(0),
  h			(0),
  cw			(0),
  ch			(0),
  iw			(0),
  dx			(s_object_dx),
  dy			(s_object_dy),
  number_of_children	(0),
  number_of_copies	(1),
  parent		(0)
{

  ERS_DEBUG(3, "id: \'" << get_id() << "\', g-class: \'" << g_class->get_name() << "\', " <<
               (p == G_Kid::stop_propagate ? "stop" : "use" ) << " propagate");

    // temporal container used to draw table for object without defined class

  std::list<std::string> r_list;

  if(p == G_Kid::use_propagate && g_class->get_class() == 0) {
    const std::list<OksRelationship *> * c_list = obj->GetClass()->all_relationships();
    if(c_list)
      for(std::list<OksRelationship *>::const_iterator j = c_list->begin(); j != c_list->end(); ++j)
        r_list.push_back((*j)->get_name());
  }


    // initialize reference

  std::list<std::string> & shown_rels = r_list;
  if(g_class->get_class() != 0) shown_rels = g_class->p_shown_relationships;


  if(p == G_Kid::use_propagate && !shown_rels.empty()) {
    std::list<std::string>::const_iterator i = shown_rels.begin();

    for(;i != shown_rels.end(); ++i) {
      OksRelationship * oks_rel = obj->GetClass()->find_relationship(*i);

      if(oks_rel) {
        G_Kid::Propagate kid_propagate = G_Kid::get_propagate_type(obj->GetClass(), oks_rel);

        if(oks_rel->get_high_cardinality_constraint() == OksRelationship::One) {
          ERS_DEBUG(4, " - append child \'" << oks_rel->get_name() << "\' (" <<
	               ( kid_propagate == G_Kid::stop_propagate ? "stop" : "use" ) << " propagate)" );

          children.push_back(new G_Child(oks_rel->get_name().c_str(), kid_propagate));
	}
	else {
	  ERS_DEBUG(4, " - append children \'" << oks_rel->get_name() << "\' (" <<
	               ( kid_propagate == G_Kid::stop_propagate ? "stop" : "use" ) << " propagate)" );

          children.push_back(new G_Children(oks_rel->get_name().c_str(), kid_propagate));
	}
      }
    }
  }
}


G_Object::~G_Object()
{
  clean();
  unlink_children();
}


void
G_Object::clean()
{
  while(!attributes.empty()) {
    G_Rectangle * r = attributes.back();
    attributes.pop_back();
    delete r;
  }

  while(!tips.empty()) {
    G_TipsTool * t = tips.front();
    tips.pop_front();
    delete t;
  }
}


void
G_Object::unlink_children()
{
  while(!children.empty()) {
    G_Kid *c = children.front();
    children.pop_front();

    if(c->get_cardinality() == G_Kid::simple) {
      G_Child *child = static_cast<G_Child *>(c);

      if(child->object)
        child->object->parent = 0;
    }
    else {
      G_Children *child2 = static_cast<G_Children *>(c);

      while(!child2->objects.empty()) {
        child2->objects.front()->parent = 0;
	child2->objects.pop_front();
      }
    }
  }
}


  //
  // The method draw graphical object if parameter is true,
  // otherwise only new sizes are calculated
  //

void
G_Object::draw(bool redraw)
{
    // clean attribute's rectangles and tipstool

  clean();


    // draw object

  if(complex_state == icon) draw_icon(redraw);
  else draw_table(redraw);

  set_valid();

  shown = true;			// allow callbacks

  cw = w;			// set children width
  ch = h;			// set children height

  if(get_composite_state() != single && number_of_children) draw_children(redraw);
}


void
G_Object::draw_relationship_pixmap(Dimension &child_x, Dimension ly, const G_Kid * kid, Dimension &a_dx, G_Rectangle& return_value, bool redraw)
{
  const OksRelationship * r = (kid ? obj->GetClass()->find_relationship(kid->rel_name) : 0);

    // chosse pixmap and it's properties

  Pixmap pixmap = (kid ? (kid->get_cardinality() == G_Kid::simple ? gc->one_pxm : gc->many_pxm) : gc->next_pxm);
  Dimension pixmap_w = (kid ? (kid->get_cardinality() == G_Kid::simple ? gc->one_pxm_w : gc->many_pxm_w) : gc->next_pxm_w);
  Dimension pixmap_h = (kid ? (kid->get_cardinality() == G_Kid::simple ? gc->one_pxm_h : gc->many_pxm_h) : gc->next_pxm_h);

  Dimension lx = x + (iw / 2) + dx;		// middle of parent object


      // draw vertical line

  if(redraw)
    gc->draw_shadowed_line(lx, y + h, lx, ly + (pixmap_h + 1)/2);


    // create rectangle to keep info about space occupaied by the pixmap(s) and relationship name

  return_value.set(lx, ly, pixmap_w, pixmap_h);


      // draw pixmap

  lx += 2 * G_Context::shadow_thickness;
  
  if(redraw)
    gc->draw_pixmap(pixmap, pixmap_w, pixmap_h, lx, ly);


    // draw aggregation pixmap if required

  a_dx = 0;

  if(r && r->get_is_composite()) {
    a_dx = gc->aggregation_pxm_w;

    if(redraw)
      gc->draw_pixmap(
	gc->aggregation_pxm, a_dx, gc->aggregation_pxm_h,
	lx + pixmap_w, ly + (pixmap_h / 2) - (gc->aggregation_pxm_h / 2)
    );
  }

  const char * pxm_text = (kid ? kid->rel_name : " ");

      // draw relationship name

  if(redraw)
    gc->draw_string(
      lx + 2 * line_w1 + gc->get_params().get_font_x_margin(),
      ly + pixmap_h + gc->get_params().get_font_y_margin() + 1,
      pxm_text
    );

  return_value.h += 1 + 2*gc->get_params().get_font_y_margin() + gc->get_font_height();


    // create tooltips describing relationship

  {
    static std::string previous_tips_tool_text;
    std::string tips_tool_text = (r ? "" : "Continuation of\n");

    if(r) {
      tips_tool_text += make_description(r);
      previous_tips_tool_text = tips_tool_text;
    }
    else {
      tips_tool_text += previous_tips_tool_text;
    }

    tips.push_back(
      new G_SimpleTipsTool(
        this,
        lx, ly, pixmap_w + a_dx, pixmap_h,
        tips_tool_text
      )
    );
  }

    // correct placement of first child in the horisontal line

  return_value.w = 
    gc->get_string_width(pxm_text) +
    2 * line_w1 +
    2 * gc->get_params().get_font_x_margin();

  child_x += return_value.w;
}


Dimension
G_Object::get_icon_width_dx(const G_Object *kid_obj) const
{
  if((kid_obj->get_complex_state() == icon) && (is_one_child_per_row() == false)) {
    Dimension kid_icon_w = (
      (kid_obj->used && kid_obj->g_class->get_used_pixmap())
        ? kid_obj->g_class->get_used_pixmap_width()
        : kid_obj->g_class->get_pixmap_width()
    );

    if(kid_obj->iw > kid_icon_w) {
      return ( ( kid_obj->iw - kid_icon_w ) / 2 );
    }
  }

  return 0;
}


void
G_Object::draw_kid(G_Object *kid_obj, Dimension& child_x, Dimension child_y, bool redraw)
{
  kid_obj->set_x(child_x);

  kid_obj->set_y(child_y);
  kid_obj->draw(redraw);

  child_x += kid_obj->cw;

  if((child_x - x) > cw) cw = child_x - x;
  if((kid_obj->ch + (kid_obj->y - y)) > ch) ch = kid_obj->ch + (kid_obj->y - y);
}

void
G_Object::draw_children(bool redraw)
{
  std::list<G_Kid *>::iterator i = children.begin();

  Dimension child_y = y + h + gc->get_ch_obj_dy();          // child row y coordinate

  for(; i != children.end(); ++i) {
    G_Kid *c = *i;

    Dimension child_x = x + (iw / 2);                       // child row x coordinate

    Dimension a_dx = 0;                                     // possible offset because of aggregation sign

    G_Rectangle rel_rect;                                   // remember space occupied by relationship pixmap and it's text

    if(c->get_cardinality() == G_Kid::simple) {
      G_Child *child = static_cast<G_Child *>(c);

      if(!child->object) continue;

      draw_relationship_pixmap(child_x, child_y, c, a_dx, rel_rect, redraw);
      Dimension child_lx = child_x;			    // remember left position of child
      draw_kid(child->object, child_x, child_y, redraw);    // draw child and change child_x to new value

      if(redraw)
        gc->draw_shadowed_line(
	  x + (iw / 2) + dx + gc->one_pxm_w + a_dx + 2 * G_Context::shadow_thickness,
	  child_y + gc->one_pxm_h / 2,
	  child_lx + get_icon_width_dx(child->object),
	  child_y + gc->one_pxm_h / 2
	);
    }
    else {
      G_Children *child2 = static_cast<G_Children *>(c);

      if(child2->objects.empty()) continue;

      draw_relationship_pixmap(child_x, child_y, c, a_dx, rel_rect, redraw);
      const Dimension lx4new_line(x + (iw / 2) + rel_rect.get_width() + 2 * dx);    // x coordinate of child vertical line
      const Dimension cx4new_line(lx4new_line + 10);                                // x coordinate of new child
      Dimension start_at_y = 0;
      Dimension stop_at_y = 0;

      Dimension prev_x = x + (iw / 2) + dx + gc->many_pxm_w + a_dx + 2 * G_Context::shadow_thickness;
      child_x = cx4new_line;

      for(std::list<G_Object *>::iterator j = child2->objects.begin(); j != child2->objects.end(); ++j) {

	  // wrap children chain when it is too long or when one-child-per-line option is choosen

        if(((child_x - x) > gc->get_ch_max_width()) || (is_one_child_per_row() == true)) {
	  if(j != child2->objects.begin()) {
	    child_y = y + ch + gc->get_ch_obj_dy();
	    prev_x = lx4new_line;
	    child_x = cx4new_line;
	  }
	}

	Dimension child_lx = child_x;			   // remember left position of child

        draw_kid(*j, child_x, child_y, redraw); // draw child and change child_x to new value

        Dimension icon_w_dx = get_icon_width_dx(*j);

        if(redraw) {
	  stop_at_y = child_y + gc->many_pxm_h / 2;
          gc->draw_shadowed_line(
	    prev_x,
	    stop_at_y,
	    child_lx + icon_w_dx,
	    stop_at_y
	  );

	  if(start_at_y == 0) start_at_y = stop_at_y;
	}


          // if there is num_of_copies field (child is icon), take it into account

	if(((*j)->num_of_copies_rect.get_y() + (*j)->num_of_copies_rect.get_height()) >= stop_at_y) {
	  prev_x = (*j)->num_of_copies_rect.get_x() + (*j)->num_of_copies_rect.get_width() + gc->get_ch_obj_dx();
	}
	else {
          prev_x = (*j)->x + (*j)->iw - icon_w_dx + gc->get_ch_obj_dx();
	}
	  
	  
        child_x += gc->get_ch_obj_dx();
      }

      if(redraw && (start_at_y != stop_at_y)) {
        gc->draw_shadowed_line(lx4new_line, start_at_y, lx4new_line, stop_at_y);
      }
    }

    child_y = y + ch + gc->get_ch_obj_dy();

      // correct position for next child and children height, if the relationship name is put below child

    if( (rel_rect.h + rel_rect.y) > child_y) {
      child_y = rel_rect.h + rel_rect.y;
      ch = child_y - y;
    }
  }
}

	//
        // Process selection rectangle
	//


void
G_Object::draw_selection(Dimension x1, Dimension y1, Dimension w1, Dimension h1, bool redraw) const
{
    // calculate selection box placement

  x1 -= line_w1 * 2;
  y1 -= line_w1 * 2;
  w1 += line_w1 * 4;
  h1 += line_w1 * 4;


    // draw selected rectangle if required
    // or remove possible selection rectangle

  if(dnd_state == src)
    gc->draw_stippled_rectangle(x1, y1, w1, h1, "magenta");
  else if(dnd_state == dest_valid)
    gc->draw_stippled_rectangle(x1, y1, w1, h1, "green");
  else if(dnd_state == dest_invalid)
    gc->draw_stippled_rectangle(x1, y1, w1, h1, "red");
  else if(shown && redraw)
    gc->clean_rectangle(x1, y1, w1, h1);
}

	//
	// Draw an object as icon
	//

void
G_Object::draw_icon(bool redraw)
{
    // no table buttons defined (hide from tooltips handler)

  sys_btn.set(0, 0, 0, 0);
  class_btn.set(0, 0, 0, 0);
  id_btn.set(0, 0, 0, 0);
  icon_btn.set(0, 0, 0, 0);
  child_btn.set(0, 0, 0, 0);
  num_of_copies_rect.set(0, 0, 0, 0);


    // destroy edit text field if it is for the object

  if(this == gc->edit_g_obj) gc->destroy_edit_widget();


    // choose pixmap and parameters

  Pixmap    icon_pxm;
  Dimension icon_w;
  Dimension icon_h;

  if(used && g_class->get_used_pixmap()) {
    icon_pxm = g_class->get_used_pixmap();
    icon_w = g_class->get_used_pixmap_width();
    icon_h = g_class->get_used_pixmap_height();
  }
  else {
    icon_pxm = g_class->get_pixmap();
    icon_w = g_class->get_pixmap_width();
    icon_h = g_class->get_pixmap_height();
  }


    // draw icon if there is valid pixmap

  if(icon_pxm) {
    bool put_text_under_icon = (parent == 0 || is_one_child_per_row() == false);
  
    Dimension object_id_width = gc->get_icon_width();
    if(object_id_width < icon_w) object_id_width = icon_w;


      // try to wrap text (if required) and get sizes of OBJECT-ID string

    std::string object_id;
    Dimension object_id_height;

    if(put_text_under_icon) {
      object_id = gc->wrap_text(get_icon_text().c_str(), object_id_width, object_id_height);
    }
    else {
      object_id = get_icon_text();
      object_id_width = gc->get_string_width(object_id.c_str());
      object_id_height = gc->get_font_height();
    }


    Dimension object_num_width = 0;
    Dimension object_num_height = 0;
    std::string object_num;

    Dimension g_width = icon_w;	// icon + number of copies (if non 1)
    if(number_of_copies > 1) {
      object_num = gc->wrap_text(number_of_copies_to_string().c_str(), object_num_width, object_num_height).c_str();
      object_num_width += 2 * gc->get_params().get_font_x_margin();
      g_width += object_num_width + gc->get_params().get_font_x_margin();
    }


    if(put_text_under_icon) {
        // detect width of icon & text
      w = ( (object_id_width > g_width) ? object_id_width : g_width );
      if(w < gc->get_icon_width()) w = gc->get_icon_width();
      iw = w;
    }
    else {
      w = g_width + object_id_width + 2 * gc->get_params().get_font_x_margin();
      iw = icon_w;
    }


      // calculate icon placement

    Dimension icon_y = y + dy;
    Dimension icon_x = x + dx;

    if(put_text_under_icon) {
      icon_x += ((w - icon_w) / 2);
    }
    else {
      // FIXME ????
    }

      // process selection rectangle

    draw_selection(icon_x, icon_y, icon_w, icon_h, redraw);


      // draw class icon

    if(redraw)
      gc->draw_pixmap(icon_pxm, icon_w, icon_h, icon_x, icon_y);

    icon_x += icon_w + gc->get_params().get_font_x_margin();


    if(redraw) {
      if(object_num_width) {
        gc->draw_shadowed_rectangle(
          icon_x,
          icon_y,
          object_num_width,
          object_num_height
        );

	num_of_copies_rect.set(icon_x, icon_y, object_num_width, object_num_height);

        gc->draw_text(
          object_num.c_str(),
          icon_x + line_w1,
          icon_y,
          object_num_width
        );
      }


	// draw object identity

      if(put_text_under_icon) {
        gc->draw_text(
          object_id.c_str(),
          x + dx,
          y + icon_h + dy + gc->get_params().get_font_y_margin(),
          w
        );
      }
      else {
        gc->draw_text(
          object_id.c_str(),
          x + dx + g_width + gc->get_params().get_font_x_margin(),
          y + (icon_h - object_id_height) / 2,
          object_id_width
        );
      }
    }

      // adjust width to the rightest coordinate of
      // number of copies rectangle, if required

    Dimension num_of_copies_w = icon_x + object_num_width - x;
    if(w < num_of_copies_w) w = num_of_copies_w;


      // set object's width and height

    w += 2 * dx;
    if(put_text_under_icon) {
      h = icon_h + object_id_height + 2 * dy + 2 * gc->get_params().get_font_y_margin();
    }
    else {
      const Dimension id_h = object_id_height + 2 * gc->get_params().get_font_y_margin();
      if(id_h > icon_h) h = id_h; else h = icon_h;
      h += 2 * gc->get_ch_obj_dy();
    }

    tips.push_back(new G_ObjectTipsTool(this, x, y, w, h));
  }
  else {
    ers::error(OksDataEditor::InternalProblem(ERS_HERE, "cannot draw object's icon because there is no valid pixmap"));
  }
}


	//
	// Draw an object as table
	//

void
G_Object::draw_table(bool redraw)
{
    // often used font parameters

  const short font_dx = gc->get_params().get_font_x_margin();
  const short font_dy = gc->get_params().get_font_y_margin();
  const short font_h  = gc->get_font_height();
  
  
    // x, y are always used with dx, dy

  const short x_dx = x + dx;
  const short y_dy = y + dy;


    // detect title height and width

  const char *class_name = obj->GetClass()->get_name().c_str();
  const char *obj_id = get_id().c_str();


    // clean num_of_copies_rect
  
  num_of_copies_rect.set(0, 0, 0, 0);


  Dimension class_name_width = gc->get_string_width(class_name);

  Dimension title_height = font_h + 2 * font_dy + 2 * line_w1;
  Dimension title_width =
    class_name_width +
    gc->get_string_width(obj_id) +
    4 * font_dx + 3 * title_height + 4 * line_w1;


    // detect attribute table height and width

  const size_t num_of_attributes = give_number_of_attributes();
  
  std::vector<std::string> values;
  values.reserve(num_of_attributes);

  size_t i = 0;

  Dimension max_attr_width = 0;
  Dimension max_value_width = 0;


    // temporal container used to draw table for object without defined class

  std::list<std::string> a_list;

  if(g_class->get_class() == 0) {
    const std::list<OksAttribute *> * c_list = obj->GetClass()->all_attributes();
    if(c_list)
      for(std::list<OksAttribute *>::const_iterator j = c_list->begin(); j != c_list->end(); ++j)
        a_list.push_back((*j)->get_name());
  }

    // initialize reference

  std::list<std::string> & shown_attrs  = a_list;
  if(g_class->get_class() != 0) shown_attrs = g_class->p_shown_attributes;

  std::list<std::string>::const_iterator ai = shown_attrs.begin();

  for(; ai != shown_attrs.end(); ++ai) {
    const std::string& attribute_name = *ai;

    if(!has_attribute(attribute_name)) continue;

    values.push_back(get_attribute_value(attribute_name));


    Dimension width = 0;
    
      // set maximum width of attribute names column
    
    width = gc->get_string_width(attribute_name.c_str());
    if(width > max_attr_width) max_attr_width = width;

      // set maximum width of attribute values column
    
    width = gc->get_string_width(values[i].c_str());
    if(width > max_value_width) max_value_width = width;
    
    i++;
  }

  Dimension rect_height = num_of_attributes * (
    font_h +
    2 * font_dy +
    2 * G_Context::shadow_thickness +		// for attributes value (to be editable)
    2 * G_Context::highlight_thickness		// for attributes value (to be editable)
  );

  Dimension rect_width = (
    max_value_width + max_attr_width +
    4 * font_dx +
    2 * line_w1 +
    2 * G_Context::shadow_thickness +		// for attributes value (to be editable)
    2 * G_Context::highlight_thickness		// for attributes value (to be editable)
  );


    // set title's and attribute table's width

  if(rect_width < title_width)
    rect_width = title_width;
  else
    title_width = rect_width;


      // process selection rectangle

  w = rect_width + 2 * line_w1 + 2 * line_w2;
  h = title_height + rect_height + 5 * line_w1 + 2 * line_w2;

  draw_selection(x_dx, y_dy, w, h, redraw);


    // draw table title

  {
    Dimension x1 = x_dx + line_w1 + line_w2;  // left position of an item
    Dimension x2 = 0;                         // width of the object_id rectangle
    Dimension y1 = y_dy + line_w1 + line_w2;  // title top
    Dimension y2 = y1 + font_dy + 1;          // title font base_line
    
    	// draw system button
    sys_btn.set(x1, y1, title_height, title_height);
    tips.push_back(new G_SimpleTipsTool(this, sys_btn, "Action Button"));
    if(redraw) {
      draw_table_system_btn();
    }
    x1 += title_height + line_w1;

	// draw class name button & class name text
    x2 = class_name_width + 2 * font_dx;
    class_btn.set(x1, y1, x2, title_height);
    tips.push_back(new G_SimpleTipsTool(this, class_btn, "Class Name"));
    if(redraw) {
      gc->draw_shadowed_rectangle(x1, y1, x2, title_height, G_Context::shadow_out);
      gc->draw_string(x1 + font_dx, y2, class_name);
    }
    x1 += class_name_width + 2 * font_dx + line_w1;

	// draw object id button & object id text
    x2 = x_dx + line_w2 + title_width - x1 - 2 * title_height - line_w1;
    id_btn.set(x1, y1, x2, title_height);
    tips.push_back(new G_SimpleTipsTool(this, id_btn, "Object Identity"));
    if(redraw) {
      gc->draw_shadowed_rectangle(x1, y1, x2, title_height, G_Context::shadow_out);
      gc->draw_string(x1 + font_dx, y2, obj_id);
    }
    x1 += x2 + line_w1;

    	// draw icon button
    icon_btn.set(x1, y1, title_height, title_height);
    tips.push_back(new G_SimpleTipsTool(this, icon_btn, "Minimize Table"));
    if(redraw) {
      draw_table_minimize_btn();
    }
    x1 += title_height + line_w1;

    	// draw children button
    child_btn.set(x1, y1, title_height, title_height);
    tips.push_back(new G_RelationshipButtonTipsTool(this, child_btn));
    if(redraw) {
      draw_children_btn();
      gc->draw_rectangle(
        x_dx + line_w2, y1 - line_w1,
        title_width + 2 * line_w1, title_height + 2 * line_w1
      );
    }
  }


    // draw table

  {
    Dimension x1 = x_dx + line_w1 + line_w2;	  		// left position of attribute name rectangle
    Dimension y1 = y_dy + 4 * line_w1 + line_w2 + title_height;	// title top
    Dimension x2 = x1 + max_attr_width + 2 * font_dx + line_w1;			// left position of attribute value rectangle

    if(redraw) {
      gc->draw_shadowed_rectangle(x1, y1, x2 - x1, rect_height, G_Context::shadow_out);

      gc->draw_shadowed_rectangle(
        x2 + line_w1, y1,
        rect_width - 2 * line_w1 - max_attr_width - 2 * font_dx,
        rect_height,
        G_Context::shadow_out
      );
    }

    for(ai = shown_attrs.begin(), i=0; ai != shown_attrs.end(); ++ai) {
      const std::string& attribute_name = *ai;

      if(!has_attribute(attribute_name)) continue;

      const char * attribute_value = values[i].c_str();
      Dimension y2 = (
        y_dy + title_height + 4 * line_w1 + line_w2 +
	1 + font_dy + G_Context::shadow_thickness + G_Context::highlight_thickness +
	(i++) * (font_h + 2 * (font_dy + G_Context::shadow_thickness + G_Context::highlight_thickness)));

      Dimension row_h = font_h + 2 * (font_dy + G_Context::shadow_thickness + G_Context::highlight_thickness);

      attributes.push_back(
        new G_Rectangle(
          x2 + 2 * line_w1,
	  y2 - font_dy - G_Context::shadow_thickness - G_Context::highlight_thickness,
          rect_width - 3 * line_w1 - max_attr_width - 2 * font_dx,
	  row_h
	)
      );

      tips.push_back(
        new G_SimpleTipsTool(
          this,
          x_dx, y2, max_attr_width, row_h,
          make_description(obj->GetClass()->find_attribute(attribute_name))
        )
      );

      if(redraw) {

        gc->draw_string(
          x_dx + font_dx + line_w2 + 2 * line_w1,
	  y2,
          attribute_name.c_str()
	);

        gc->draw_string(
          x_dx + max_attr_width + 3 * font_dx + line_w2 + 4 * line_w1 + G_Context::shadow_thickness + G_Context::highlight_thickness,
	  y2,
          attribute_value,
	  (G_Object::is_url(attribute_value) ? "dark blue" : 0)
	);
      }
    }
  }


    // draw outline rectangle

  if(redraw)
    gc->draw_shadowed_rectangle(
      x_dx, y_dy, w, h,
      G_Context::shadow_out, line_w2
    );


    // set size

  iw = w;

  w += 2 * dx;
  h += 2 * dy;
}


G_Object *
G_Object::find_g_object(const OksObject *o, G_Set * objects, bool silent)
{
  G_Object dummy(o);
  G_Set::iterator i = objects->find(&dummy);

  if(i != objects->end()) return *i;

  if(silent == false) {
    std::ostringstream text;
    text << "cannot find graphical object corresponding OKS object " << o;
    ers::error(OksDataEditor::InternalProblem(ERS_HERE, text.str().c_str()));
  }

  return 0;
}


void
G_Object::set_children(std::list<G_Object *> *objects, G_Set * objs_set)
{
    // remember objects which are processing now to avoid infinite recursion
    // (may have place for objects of unknown graphical classes)

  static std::set<OksObject *, std::less<OksObject *> > r_objs;

  if(children.empty()) return;

  r_objs.insert(get_oks_object());

  number_of_children = 0;
  
  std::list<G_Kid *>::iterator i = children.begin();

  for(; i != children.end(); ++i) {
    G_Kid *c = *i;

    if(c->get_cardinality() == G_Kid::simple) {
      G_Child *child = static_cast<G_Child *>(c);

      OksData *d(obj->GetRelationshipValue(c->rel_name));

      if(d->type == OksData::object_type) {
        if(d->data.OBJECT) {
	  child->object = find_g_object(d->data.OBJECT, objs_set);

	  G_Kid::Propagate kp = (
	    (
	      r_objs.find(d->data.OBJECT) == r_objs.end() ||
	      c->propagate == G_Kid::stop_propagate ||
	      child->object == parent
	    )
	      ? G_Kid::stop_propagate
	      : G_Kid::use_propagate
	  );

	  if(c->propagate == G_Kid::stop_propagate || child->object->parent || child->object == parent) {
	    G_Object *g_obj = child->object->make_copy(kp);

	    objects->push_back(g_obj);
	    objs_set->insert(g_obj);

	    g_obj->set_used(child->object->is_used());
	    child->object = g_obj;
	  }

	  child->object->parent = this;

	  number_of_children++;
	}
      }
      else {
        std::ostringstream text;
        text << "object \'" << *d << "\' is not loaded and will not be shown (referenced by " << obj << " via \"" << c->rel_name << "\" relationship)";
        ers::error(OksDataEditor::Problem(ERS_HERE, text.str().c_str()));
      }
    }
    else {
      G_Children *child2 = static_cast<G_Children *>(c);

      OksData *d(obj->GetRelationshipValue(c->rel_name));

      if(d->data.LIST && !d->data.LIST->empty()) {
        OksData::List * dlist = d->data.LIST;
	for(OksData::List::iterator j = dlist->begin(); j != dlist->end(); ++j) {
          if((*j)->type == OksData::object_type) {
            if((*j)->data.OBJECT) {
	      G_Object *g_obj = find_g_object((*j)->data.OBJECT, objs_set);


	      if(c->propagate == G_Kid::stop_propagate || g_obj->parent || g_obj == parent) {
	        bool g_obj_is_used = g_obj->is_used();

	        G_Kid::Propagate kp = (
	          (
	            r_objs.find((*j)->data.OBJECT) == r_objs.end() ||
	            c->propagate == G_Kid::stop_propagate ||
		    g_obj == parent
	          )
	            ? G_Kid::stop_propagate
	            : G_Kid::use_propagate
	        );

	        g_obj = g_obj->make_copy(kp);

	        objects->push_back(g_obj);
	        objs_set->insert(g_obj);
		g_obj->set_used(g_obj_is_used);
	      }

              g_obj->parent = this;

              child2->objects.push_back(g_obj);

              number_of_children++;
	    }
          }
          else {
            std::ostringstream text;
            text << "object \'" << *(*j) << "\' is not loaded and will not be shown (referenced by " << obj << " via \"" << c->rel_name << "\" relationship)";
            ers::error(OksDataEditor::Problem(ERS_HERE, text.str().c_str()));
          }
	}
      }
    }
  }

  r_objs.erase(get_oks_object());
}


void
G_Object::reset_children(G_Set * objs_set)
{
  ERS_DEBUG(4, "object: " << obj );

  if(children.empty()) return;

  number_of_children = 0;
  
  for(std::list<G_Kid *>::iterator i = children.begin(); i != children.end(); ++i) {
    G_Kid *c = *i;

    if(c->get_cardinality() == G_Kid::simple) {
      G_Object *test_obj = static_cast<G_Child *>(c)->object;
      
      if(test_obj) {
        if(find_g_object(test_obj->get_oks_object(), objs_set, true) == 0) {
          try {
            OksData *d(obj->GetRelationshipValue(c->rel_name));
	  
            if(d->type == OksData::object_type && d->data.OBJECT) {
	      ERS_DEBUG(4, "call SetRelationshipValue(\"" << c->rel_name << "\", (null)) for object " << obj->GetId() );
	      d->data.OBJECT = 0;
              obj->SetRelationshipValue(c->rel_name, d);
	      (static_cast<G_Child *>(c))->object = 0;
	      continue;
	    }
          }
          catch(oks::exception& ex) {
            report_exception("Reset Single-Value Child", ex);
          }
        }
        else {
          number_of_children++;
        }
      }
    }
    else {
      G_Children *child2 = static_cast<G_Children *>(c);

      try {
        OksData *d(obj->GetRelationshipValue(c->rel_name));

        if(d->data.LIST && !d->data.LIST->empty()) {
          for(std::list<G_Object *>::iterator i2 = child2->objects.begin(); i2 != child2->objects.end();) {
            G_Object *test_obj = *i2;
 
            if(find_g_object(test_obj->get_oks_object(), objs_set, true) == 0) {
	      i2 = child2->objects.erase(i2);

	      OksData::List * dlist = d->data.LIST;
	      for(OksData::List::iterator j = dlist->begin(); j != dlist->end(); ++j) {
	        OksData * obj_d = *j;

	        if(obj_d->type == OksData::object_type && obj_d->data.OBJECT == test_obj->get_oks_object()) {
                  j = dlist->erase(j);
                  delete obj_d;
	          ERS_DEBUG(4, "call SetRelationshipValue(\"" << c->rel_name << "\", {" << *d << "}) for object " << obj->GetId() );
                  obj->SetRelationshipValue(c->rel_name, d);
	        }
	      }
	    }
	    else {
	      number_of_children++;
	      ++i2;
	    }
	  }
        }
      }
      catch(oks::exception& ex) {
        report_exception("Reset Multi-Value Child", ex);
      }
    }
  }

  ERS_DEBUG(4, "LEAVE reset_children() for object " << obj);
}


	//
	// Remove from relationship REL_NAME object(s) representing OKS_OBJ
	// Return true if the object was modified and false otherwise
	//

bool
G_Object::unlink_child(const OksObject *oks_obj, const char *rel_name)
{
    // remember number of children before start of operation

  size_t n = number_of_children;


    // iterate all child objects

  for(std::list<G_Kid *>::iterator i = children.begin(); i != children.end(); ++i) {
    G_Kid *c = *i;

      // look for relationship with the same name

    if(!strcmp(rel_name, c->rel_name)) {
      if(c->get_cardinality() == G_Kid::simple) {
        G_Child *child = static_cast<G_Child *>(c);
	
	if(child->object && child->object->get_oks_object() == oks_obj) {
	  child->object->parent = 0;
	  child->object = 0;
	  number_of_children--;
	}
      }
      else {
        G_Children *child2 = static_cast<G_Children *>(c);
	
	for(std::list<G_Object *>::iterator j = child2->objects.begin(); j != child2->objects.end();) {
	  if((*j)->get_oks_object() == oks_obj) {
	    (*j)->parent = 0;
	    j = child2->objects.erase(j);
	    number_of_children--;
	  }
	  else
	    ++j;
	}
      }
    }
  }
  
  return (n != number_of_children);
}


void
G_Object::swap_rel_objects(const char *rel_name, const OksObject *oks_obj1, const OksObject *oks_obj2)
{
  ERS_DEBUG(3, "rel: " << rel_name << ", o1: " << oks_obj1 << ", o2:" << oks_obj2 << ", parent: " << obj);

    // iterate all child objects

  for(std::list<G_Kid *>::iterator i = children.begin(); i != children.end();++i) {
    G_Kid *c = *i;
    
      // look for relationship with the same name

    if(!strcmp(rel_name, c->rel_name)) {
      G_Children *child2 = static_cast<G_Children *>(c);
	
      std::list<G_Object *>::iterator j = child2->objects.begin();

      G_Object *o1 = 0;
      G_Object *o2 = 0;

      for(;j != child2->objects.end(); ++j) {
        if      ((*j)->get_oks_object() == oks_obj1) o1 = *j;
        else if ((*j)->get_oks_object() == oks_obj2) o2 = *j;
      }

      if(o1 && o2) {
        for(j = child2->objects.begin();j != child2->objects.end();++j) {
          if(*j == o1) {
	    j = child2->objects.erase(j);
	    j = child2->objects.insert(j, o2);
          }
          else if(*j == o2) {
	    j = child2->objects.erase(j);
	    j = child2->objects.insert(j, o1);
          }
	}
	
	gc->clean_rectangle(x, y, cw, ch);
	draw();

	return;
      }
    }
  }
}


G_Object *
G_Object::link_with_object(const char * rel_name, G_Object * child_g_obj)
{
  ERS_DEBUG(4, "rel: " << rel_name << ", child: " << child_g_obj->obj <<
               ", object: " << (void *)this << ' ' << obj);

  G_Object * new_obj = 0;


    // detect required G_Kid pointer

  G_Kid * kid = 0;

  for(std::list<G_Kid *>::iterator i = children.begin(); i != children.end();++i) {
    kid = *i;

    if(!strcmp(kid->rel_name, rel_name)) {
      if(kid->get_cardinality() == G_Kid::simple) {
        G_Object *child_obj = (static_cast<G_Child *>(kid))->object;

        if(child_obj) {
          if(child_obj->propagate == G_Kid::stop_propagate || child_obj->number_of_copies > 1)
            child_obj->obj = 0;     // child object has to be removed
          else
            child_obj->parent = 0;  // child object has no parent anymore
        }
      }

      break;
    }

    kid = 0;
  }

  if(kid) {

      // link may require creation of new object

    if(parent == child_g_obj) {
      child_g_obj = child_g_obj->make_copy(G_Kid::stop_propagate);
      new_obj = child_g_obj;
    }

    if(kid->get_cardinality() == G_Kid::simple)
      (static_cast<G_Child *>(kid))->object = child_g_obj;
    else
      (static_cast<G_Children *>(kid))->objects.push_back(child_g_obj);

    child_g_obj->parent = this;
  
    number_of_children++;
  }

  return new_obj;
}


void
G_Object::set_number_of_copies(G_Set * objects)
{
  for(G_Set::iterator i = objects->begin(); i != objects->end(); ) {
    G_Set::iterator j(i);
    OksObject * obj((*i)->get_oks_object());
    
    unsigned int count = 1;
    while(true) {
      ++i;
      if(i == objects->end()) break;
      else if((*i)->get_oks_object() == obj) count++;
      else break;
    }

    while(j != i) {
      G_Object * object = *j;
      object->number_of_copies = count;
      ++j;
    }
  }
}


std::string
G_Object::number_of_copies_to_string() const
{
  std::ostringstream s;
  s << number_of_copies;
  return s.str();
}


	//
	// Check that given graphic object has attribute 'name'
	//

bool
G_Object::has_attribute(const std::string & name) const
{
  return ((obj->GetClass()->find_attribute(name) != 0) ? true : false);
}


	//
	// Returns number of attributes for given graphic object
	//

size_t
G_Object::give_number_of_attributes() const
{
  if(g_class->get_class() == 0) {
    return obj->GetClass()->number_of_all_attributes();
  }
  else {
    size_t count = 0;

    std::list<std::string>::const_iterator i = g_class->p_shown_attributes.begin();

    for(;i != g_class->p_shown_attributes.end(); ++i)
      if(has_attribute(*i)) count++;
  
    return count;
  }
}


	//
	// Returns attribute value by name
	//

std::string
G_Object::get_attribute_value(const std::string & attr_name, int format) const
{
  try {
    OksAttribute * a = obj->GetClass()->find_attribute(attr_name);
    return obj->GetAttributeValue(attr_name)->str(a->is_integer() ? (format ? format : (int)a->get_format()) : (int)0);
  }
  catch(oks::exception & ex) {
    ers::error(OksDataEditor::InternalProblem(ERS_HERE, ex.what()));
    return "";
  }
}


	//
	// Sets attribute value by name
	//

void
G_Object::set_attribute_value(const std::string & attr_name, const char * attr_value)
{
  try {
      // old value
    OksData * d2(obj->GetAttributeValue(attr_name));

      // new value
    OksData d;
    d.SetValues(attr_value, obj->GetClass()->find_attribute(attr_name));

      // set value only if old-value != new-value
    if(d != *d2) {
      obj->SetAttributeValue(attr_name, &d);
    }
  }
  catch(oks::exception & ex) {
    report_exception("Bad User Input", ex);
  }
}


const std::string&
G_Object::get_attribute_name(size_t at) const
{
  static std::string empty_str;

  size_t count = 0;


  if(g_class->get_class() == 0) {
    const std::list<OksAttribute *> * c_list = obj->GetClass()->all_attributes();
    if(c_list)
      for(std::list<OksAttribute *>::const_iterator j = c_list->begin(); j != c_list->end(); ++j)
        if(count++ == at) return (*j)->get_name();
  }
  else {
    std::list<std::string>::const_iterator j = g_class->p_shown_attributes.begin();

    for(;j != g_class->p_shown_attributes.end(); ++j)
      if(has_attribute(*j))
        if(count++ == at) return *j;
  }

  std::stringstream text;
  text << "graphic class \'" << g_class->get_name() << "\' contains " << count << " attributes only (requested name for attribute # " << at << ')';
  ers::error(OksDataEditor::InternalProblem(ERS_HERE, text.str().c_str()));

  return empty_str;
}

const G_Rectangle&
G_Object::get_attribute_rect(size_t idx) const
{
  size_t count = 0;

  if(g_class->get_class() == 0) {
    return *(attributes[idx]);
  }
  else {
    std::list<std::string>::const_iterator j = g_class->p_shown_attributes.begin();

    for(;j != g_class->p_shown_attributes.end(); ++j)
      if(has_attribute(*j))
        if(count++ == idx)
          return *(attributes[idx]);
  }

  std::stringstream text;
  text << "graphic object " << obj << " contains " << count << " attributes only (requested rectangle for attribute # " << idx << ')';
  ers::error(OksDataEditor::InternalProblem(ERS_HERE, text.str().c_str()));

  return *(attributes.front());
}


bool
G_Object::is_data_file_read_only() const
{
  return (OksKernel::check_read_only(obj->get_file()));
}


  //
  // Calculate icon text:
  // use object identity and value of attribute
  //

std::string
G_Object::create_icon_text(const char *attr_name) const
{
  std::string icon_text = get_id().c_str();
 
  try {
    OksAttribute * a = obj->GetClass()->find_attribute(attr_name);
    std::string value = get_oks_object()->GetAttributeValue(attr_name)->str(a->is_integer() ? (int)a->get_format() : (int)0);
    icon_text += " (";
    icon_text += value;
    icon_text += ')';
  }
  catch(oks::exception& ex) {
    ers::error(OksDataEditor::InternalProblem(ERS_HERE, ex.what()));
  }

  return icon_text;
}


void
G_Object::add_relationship_oks_object(OksObject *p_obj, OksObject *obj, const char *name)
{
  OksData *d(p_obj->GetRelationshipValue(name));

  if(d->type == OksData::list_type) {
    p_obj->AddRelationshipValue(name, obj);
  }
  else {
    p_obj->SetRelationshipValue(name, obj);
  }
}


void
G_Object::add_relationship_oks_object(OksObject *oks_obj, const char *name)
{
  const std::string * reverse_rel_name = g_class->get_reverse_relationship(name);
 
  if(reverse_rel_name != 0) {
    G_Object::add_relationship_oks_object(oks_obj, get_oks_object(), reverse_rel_name->c_str());
  }

  G_Object::add_relationship_oks_object(get_oks_object(), oks_obj, name);
}


void
G_Object::remove_relationship_oks_object(OksObject *p_obj, OksObject *obj, const char *name)
{
  OksData *d(p_obj->GetRelationshipValue(name));

  if(d->type == OksData::list_type) {
    p_obj->RemoveRelationshipValue(name, obj);
  }
  else {
    p_obj->SetRelationshipValue(name, (OksObject *)0);
  }
}


void
G_Object::remove_relationship_oks_object(OksObject *oks_obj, const char *name)
{
  const char * reverse_rel_name = get_reverse_relationship(name);

  if(reverse_rel_name != 0) {
    G_Object::remove_relationship_oks_object(oks_obj, get_oks_object(), reverse_rel_name);
  }

  G_Object::remove_relationship_oks_object(get_oks_object(), oks_obj, name);
}


bool
G_Object::is_point_inside_object(unsigned short ix, unsigned short iy) const
{
  return (
    ( ix >= x       ) &&
    ( iy >= y       ) &&
    ( ix <= (x + w) ) &&
    ( iy <= (y + h) )
  );
}


bool
G_Object::is_point_inside_composite_object(unsigned short ix, unsigned short iy) const
{
  return (
    ( ix >= x        ) &&
    ( iy >= y        ) &&
    ( ix <= (x + cw) ) &&
    ( iy <= (y + ch) )
  );
}


G_Object::DragAndDropState
G_Object::get_dest_link_state() const
{
  if(get_propagate() == G_Kid::use_propagate) return dest_valid;


    // check for swap (if belongs the same parent via the same relationship)

  if(parent) {
    for(std::list<G_Kid *>::iterator i = parent->children.begin();i != parent->children.end();++i) {
      G_Kid *c = *i;

      if((c)->get_cardinality() == G_Kid::simple) continue;

      G_Children *kids = static_cast<G_Children *>(c);

      unsigned int count = 0;

      for(std::list<G_Object *>::iterator j = kids->objects.begin(); j != kids->objects.end(); ++j)
        if((*j)->dnd_state == G_Object::src || *j == this) count++;

      if(count == 2) return dest_valid;
    }
  }  

  return dest_invalid;
}


G_Object::RefTree *
G_Object::RefTree::find(const std::string &s) const
{
  for(std::list<RefTree *>::const_iterator j = values.begin(); j != values.end(); ++j) {
    if(s == (*j)->name) return *j;
  }

  return 0;
}


G_Object::RefTree *
G_Object::RefTree::get(const std::string &s)
{
  RefTree * ref = find(s);

  if(!ref) {
    RefTree * new_ref = new RefTree(s);
    values.push_back(new_ref);
    return new_ref;
  }

  return ref;
}


void
G_Object::get_references(const G_Object *g_obj, std::list<std::pair<const G_Object *, const char *> > *obj_list, RefTree & refs) const
{
  if(!children.empty()) {
    for(std::list<G_Kid *>::const_iterator i = children.begin(); i != children.end(); ++i) {
      const G_Kid *c = *i;
      bool found = false;

      obj_list->push_back(std::make_pair(this, c->rel_name));

      if(c->get_cardinality() == G_Kid::simple) {
        const G_Child *child = static_cast<const G_Child *>(c);
        if(child->object) {
	  if(child->object->obj == g_obj->obj) found = true;
	  child->object->get_references(g_obj, obj_list, refs);
	}
      }
      else {
        const G_Children *child = static_cast<const G_Children *>(c);
	for(std::list<G_Object *>::const_iterator j = child->objects.begin(); j != child->objects.end(); ++j) {
	  if((*j)->obj == g_obj->obj) {
	    found = true;
	  }
	  
	  (*j)->get_references(g_obj, obj_list, refs);
        }
      }

      if(found) {
	RefTree * ref_tree = &refs;

	for(std::list<std::pair<const G_Object *, const char *> >::iterator k=obj_list->begin(); k != obj_list->end(); ++k) {
            // generate object name

          std::string obj_name = to_string(((*k).first)->obj);


            // get reference to already existing object or create new branch

	  ref_tree = ref_tree->get(obj_name);


            // get reference to already existing relationship or create new branch

	  ref_tree = ref_tree->get((*k).second);
	}
      }

      obj_list->pop_back();
    }
  }
}


const std::string&
G_Object::get_id() const
{
  return obj->GetId();
}


void
G_Object::init_new()
{
  if(is_verbose()) {
    std::cout << " + call init new for " << obj << std::endl;
  }

  const std::list<OksAttribute *> * alist = obj->GetClass()->all_attributes();
  
  if(alist) {
    for(std::list<OksAttribute *>::const_iterator i = alist->begin(); i != alist->end(); ++i) {
      const std::string & aname = (*i)->get_name();
      const std::list<std::string> * elist = g_class->get_env_init_attr(aname);

      if(is_verbose()) {
        std::cout << "  " << (elist ? '+' : '-') << " check attribute \'" << aname << "\'\n";
      }

      if(elist) {
        const char * value = 0;

        for(std::list<std::string>::const_iterator j = elist->begin(); j != elist->end(); ++j) {
          value = getenv((*j).c_str());

          if(is_verbose()) {
            std::cout << "   - check environment variable \'" << *j
	              << "\' state: \'" << (value == 0 ? "no" : *value == 0 ? "empty" : "set") << "\'\n";
          }

	  if(value && *value != 0) break;
	}

	if(value) {
          if(is_verbose()) {
            std::cout << "     *** use value \'" << value << "\' to initialize attribute\n";
          }

          try {
            OksData * d(obj->GetAttributeValue(aname));
	    if(d->type == OksData::list_type) {
	      if(d->data.LIST == 0) d->data.LIST = new OksData::List();
	      d->data.LIST->push_front(new OksData(value));
	    }
	    else {
	      d->Set(value);
	    }
          }
          catch(oks::exception & ex) {
            ers::error(OksDataEditor::InternalProblem(ERS_HERE, ex.what()));
          }
	}
      }
      else if(aname == "Name") {
        try {
          OksData name_d(get_id());
          obj->SetAttributeValue("Name", &name_d);

          if(is_verbose()) {
            std::cout << "    *** use object id \'" << get_id() << "\' to initialize attribute\n";
          }
        }
        catch(oks::exception& ex) {
          ers::error(OksDataEditor::InternalProblem(ERS_HERE, ex.what()));
        }
      }
    }
  }
}

std::string
G_Object::get_icon_text() const
{
  const std::string& s = g_class->get_icon_title();

  if(s.empty()) return get_id();
  
  std::string::size_type beg_idx = s.find_first_of('@');
  
  if(beg_idx == std::string::npos) return s;

  std::string::size_type end_idx = 0;
  std::string icon_title(s, 0, beg_idx);

  while(beg_idx != std::string::npos) {
    end_idx = s.find_first_of('@', beg_idx + 1);

    if(end_idx != std::string::npos) {
      std::string token(s, beg_idx + 1, end_idx - beg_idx - 1);
      
      std::string str;
      
        // replace "@@" by one "@"

      if(token.empty()) {
        str = "@";
      }


        // replace "@ID@" by one "object ID"

      else if(token == "ID") {
        str = get_id();
      }


        // replace "@CLASS@" by one "object CLASS-NAME"

      else if(token == "CLASS") {
        str = obj->GetClass()->get_name();
      }


        // replace "@ATTRIBUTE-X@" by one "value of attribute ATTRIBUTE-X"

      else {
        try {
          OksAttribute * a = obj->GetClass()->find_attribute(token);
	  str = obj->GetAttributeValue(token)->str(a->is_integer() ? (int)a->get_format() : (int)0);
	}
        catch(oks::exception& ex) {
          ers::error(OksDataEditor::InternalProblem(ERS_HERE, ex.what()));
        }
      }

      if(!str.empty()) {
        icon_title.append(str);
      }

      beg_idx = s.find_first_of('@', end_idx + 1);
      if(beg_idx != std::string::npos) icon_title.append(s, end_idx + 1, beg_idx - end_idx - 1);
      else icon_title.append(s, end_idx + 1, s.size() - end_idx - 1);
    }
    else {
      icon_title.append(s, beg_idx, s.size() - end_idx);
      break;
    }
  }

  return icon_title;
}


std::list<const std::string *> *
G_Object::get_list_of_new_objects(const std::list<const G_Class *> * tc, const std::list<const G_Class *> * oc, bool enabled) const
{
  static std::string s_add("Add");
  static std::string s_replace("Replace");
  static std::string s_set("Set");
  static std::string s_disabled("Disabled");
  static std::string s_enabled("Enabled");

  std::list<const std::string *> * rlist = 0;

  const OksClass * c = obj->GetClass();


    // iterate all child objects

  for(std::list<G_Kid *>::const_iterator i = children.begin(); i != children.end(); ++i) {
    G_Kid * kid = *i;
    const char * kid_rel_name = kid->rel_name;
    
    const OksRelationship * r = c->find_relationship(kid_rel_name);
    
    if(!r) continue;
    
    const OksClass * rc = r->get_class_type();

    bool found = false;

    const std::list<const G_Class *> * w_classes[2] = { tc, oc };

    for(int l = 0; ((l < 2) && (found == false)); ++l) {
      for(std::list<const G_Class *>::const_iterator j = w_classes[l]->begin(); j != w_classes[l]->end(); ++j) {
        const OksClass * tclass = (*j)->get_class();
        if(tclass == rc || rc->find_super_class(tclass->get_name()) || tclass->find_super_class(rc->get_name())) {
          found = true;
	  break;
        }
      }
    }

    if(found == false) continue;

    if(!rlist) {
      rlist = new std::list<const std::string *>();
      rlist->push_back(enabled ? &s_enabled : &s_disabled);
    }

      // look for relationship with the same name

    if(kid->get_cardinality() == G_Kid::simple) {
      G_Child *child = static_cast<G_Child *>(kid);
      if(child->object) {
	rlist->push_back(&s_replace);
      }
      else {
	rlist->push_back(&s_set);
      }
    }
    else {
      rlist->push_back(&s_add);
    }

    rlist->push_back(&rc->get_name());
    rlist->push_back(&r->get_name());
  }


  return rlist;
}
