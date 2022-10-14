#include "g_object.h"
#include "g_class.h"
#include "g_context.h"

	//
	// Draw system button
	//

void
G_Object::draw_table_system_btn()
{
  gc->draw_shadowed_rectangle(sys_btn.x, sys_btn.y, sys_btn.w, sys_btn.h , G_Context::shadow_out);

  Dimension d5 = sys_btn.w / 5;
  Dimension d3 = (sys_btn.h * 3) / 8;

  gc->draw_shadowed_rectangle(
    sys_btn.x + d5,
    sys_btn.y + d3,
    sys_btn.w - 2 * d5,
    sys_btn.h - 2 * d3,
    G_Context::shadow_in
  );
}


void
G_Object::draw_table_system_btn_pressed()
{
  gc->draw_shadowed_rectangle(
    sys_btn.x,
    sys_btn.y,
    sys_btn.w,
    sys_btn.h,
    G_Context::shadow_in
  );
}


int
G_Object::test_table_system_btn(Dimension x1, Dimension y1)
{
  return sys_btn.is_point_inside(x1, y1);
}





	//
	//     -------------
	//     | (a)   (b) |
	//     |  *-----*  |
	//     |   \   /   |
	//     |    \ /    |
	//     |     *     |
	//     |    (c)    |
	//     -------------
	//
	

void
G_Object::draw_table_minimize_btn()
{
  gc->draw_shadowed_rectangle(
    icon_btn.x,
    icon_btn.y,
    icon_btn.w,
    icon_btn.h,
    G_Context::shadow_out
  );

  Dimension d2 = icon_btn.w / 2;
  Dimension d5 = icon_btn.w / 5;
  Dimension xa = icon_btn.x + d5;
  Dimension xb = icon_btn.x + icon_btn.w - d5;
  Dimension xc = icon_btn.x + d2;
  Dimension ya = icon_btn.y + d5;
  Dimension yb = ya;
  Dimension yc = icon_btn.y + icon_btn.h - d5;

  gc->draw_line(xa, ya, xb, yb, OksGC::get_bottom_shadow_color());
  gc->draw_line(xa, ya, xc, yc, OksGC::get_bottom_shadow_color());
  gc->draw_line(xb, yb, xc, yc, OksGC::get_top_shadow_color());
}


int
G_Object::test_table_minimize_btn(Dimension x1, Dimension y1)
{
  return icon_btn.is_point_inside(x1, y1);
}


void
G_Object::draw_table_minimize_btn_pressed()
{
  gc->draw_shadowed_rectangle(
    icon_btn.x,
    icon_btn.y,
    icon_btn.w,
    icon_btn.h,
    G_Context::shadow_in
  );
}


	//
	// Draw children button
	//
	//      (1)*---*(2)
	//         |   |
	//     (12)|   |(3)
	// (11)*---*   *---*(4)
	//     |           |
	// (10)*---*   *---*(5)
	//      (9)|   |(6)
	//         |   |
	//      (8)*---*(7)
	//
	// Available states:
	// 	'+' - show children (full picture)
	// 	'-' - hide children (rectangle 11-4-5-10)
	// 	'o' - no children   (rectangle 12-3-6-9)
	//

void
G_Object::draw_children_btn()
{
  gc->draw_shadowed_rectangle(
    child_btn.x,
    child_btn.y,
    child_btn.w,
    child_btn.h,
    G_Context::shadow_out
  );

  Dimension d6 = child_btn.w / 5;
  Dimension d4 = (child_btn.w * 4) / 9;
  
  Dimension x1  = child_btn.x + d4;
  Dimension x2  = child_btn.x + child_btn.w - d4;
  Dimension x4  = child_btn.x + child_btn.w - d6;
  Dimension x11 = child_btn.x + d6;
  Dimension y1  = child_btn.y + d6;
  Dimension y8  = child_btn.y + child_btn.h - d6;
  Dimension y11 = child_btn.y + d4;
  Dimension y10 = child_btn.y + child_btn.h - d4;

  if(!number_of_children)
     gc->draw_shadowed_rectangle(x1, y11, x2 - x1, y10 - y11, G_Context::shadow_in);
  else if(get_composite_state() == complete)
     gc->draw_shadowed_rectangle(x11, y11, x4 - x11, y10 - y11, G_Context::shadow_in);
  else {
    gc->draw_line(x1,  y1,  x2,  y1,  OksGC::get_bottom_shadow_color());
    gc->draw_line(x2,  y1,  x2,  y11, OksGC::get_top_shadow_color());
    gc->draw_line(x2,  y11, x4,  y11, OksGC::get_bottom_shadow_color());
    gc->draw_line(x4,  y11, x4,  y10, OksGC::get_top_shadow_color());
    gc->draw_line(x4,  y10, x2,  y10, OksGC::get_top_shadow_color());
    gc->draw_line(x2,  y10, x2,  y8,  OksGC::get_top_shadow_color());
    gc->draw_line(x2,  y8,  x1,  y8,  OksGC::get_top_shadow_color());
    gc->draw_line(x1,  y8,  x1,  y10, OksGC::get_bottom_shadow_color());
    gc->draw_line(x1,  y10, x11, y10, OksGC::get_top_shadow_color());
    gc->draw_line(x11, y10, x11, y11, OksGC::get_bottom_shadow_color());
    gc->draw_line(x11, y11, x1,  y11, OksGC::get_bottom_shadow_color());
    gc->draw_line(x1,  y11, x1,  y1,  OksGC::get_bottom_shadow_color());
  }
}

void
G_Object::draw_children_btn_pressed()
{
  gc->draw_shadowed_rectangle(
    child_btn.x,
    child_btn.y,
    child_btn.w,
    child_btn.h,
    G_Context::shadow_in
  );
}

int
G_Object::test_children_btn(Dimension x1, Dimension y1)
{
  return child_btn.is_point_inside(x1, y1);
}


int
G_Object::test_table_class_name_btn(Dimension x1, Dimension y1)
{
  return class_btn.is_point_inside(x1, y1);
}

int
G_Object::test_table_object_id_btn(Dimension x1, Dimension y1)
{
  return id_btn.is_point_inside(x1, y1);
}
