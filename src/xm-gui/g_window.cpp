#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <vector>

#include <ers/ers.h>

#include <oks/xm_popup.h>
#include <oks/xm_utils.h>
#include <oks/relationship.h>

#include <Xm/Xm.h>
#include <Xm/DrawingA.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/Screen.h>
#include <Xm/ScrollBar.h>

#include <oks/xm_utils.h>

#include "data_editor_main_dlg.h"
#include "data_editor_exceptions.h"
#include "g_window_info.h" 
#include "g_ref_tree.h"

#include "g_class.h"
#include "g_object.h"

#include "exit.xpm"


const char * G_Window::choose_file_name_str     = "-- choose database file --";
const char * G_Window::create_new_file_name_str = "create new database file ...";

void
G_Window::draw(XExposeEvent *exposeEvent)
{
  ERS_DEBUG( 1, "enter draw()" );

  g_context.p_icon_width    = g_context.get_params().get_u16_parameter(OksParameterNames::get_icon_width_name());
  g_context.p_ch_obj_dx     = g_context.get_params().get_u16_parameter(OksParameterNames::get_ch_obj_dx_name());
  g_context.p_ch_obj_dy     = g_context.get_params().get_u16_parameter(OksParameterNames::get_ch_obj_dy_name());
  g_context.p_ch_max_width  = g_context.get_params().get_u16_parameter(OksParameterNames::get_ch_max_width_name());
  g_context.p_obj_dx        = g_context.get_params().get_u16_parameter(OksParameterNames::get_obj_dx_name());
  g_context.p_obj_dy        = g_context.get_params().get_u16_parameter(OksParameterNames::get_obj_dy_name());

  {
    const std::string * s = g_context.get_params().get_string_parameter(OksParameterNames::get_arrange_objects_name());

    if(*s == OksParameterNames::get_arrange_obj_as_column_name()) obj_arrange_type = AsColumn;
    else if(*s == OksParameterNames::get_arrange_obj_as_row_name()) obj_arrange_type = AsRow;
    else if(*s == OksParameterNames::get_arrange_obj_by_classes_name()) obj_arrange_type = ByClasses;
    else if(*s == OksParameterNames::get_arrange_obj_wrap_any_name()) obj_arrange_type = WrapAny;
    else obj_arrange_type = WrapByClass;
  }

  {
    const std::string * s = g_context.get_params().get_string_parameter(OksParameterNames::get_arrange_children_name());

    if(*s == OksParameterNames::get_arrange_one_child_per_line_name()) child_arrange_type = OnePerLine;
    else child_arrange_type = ManyPerLine;
  }

  G_Context::disable_tooltips();
  G_Object::clean_motion_timer(&g_context);

  Dimension x = g_context.get_params().get_drawing_area_x_margin();
  unsigned long y = g_context.get_params().get_drawing_area_y_margin();
  Dimension x_max_right = x;
  Dimension row_max_y = 0;


    // the rectangle defines visible part of view that has to be re-drawn

  G_Rectangle rect;

  if(exposeEvent)
    rect.set(exposeEvent->x, exposeEvent->y, exposeEvent->width, exposeEvent->height);
  else {
    Dimension rw, rh;
    XtVaGetValues(d_area, XtNwidth, &rw, XtNheight, &rh, NULL);

    rect.set(0, 0, rw, rh);
  }

  ERS_DEBUG(2, "draw window x=" << rect.get_x() << ", y=" << rect.get_y() << ", w="<< rect.get_width() << ", h=" << rect.get_height() << ", arrange_type=" << (int)obj_arrange_type );


    // clean rectangle first

  if(
   !g_context.get_fc() &&
   rect.get_width() > 0 &&
   rect.get_height() > 0
  )
    g_context.clean_rectangle(
      ((rect.get_x() > g_context.get_params().get_drawing_area_x_margin()) ? rect.get_x() : g_context.get_params().get_drawing_area_x_margin()),
      ((rect.get_y() > g_context.get_params().get_drawing_area_y_margin()) ? rect.get_y() : g_context.get_params().get_drawing_area_y_margin()),
      rect.get_width(),
      rect.get_height()
    );

    // 'max_width' is width of drawing window
    //  - precize value of visible area width is required by arrange types 'WrapAny' & 'WrapByClass'
    //  - set it to max short value for other arrange types

  Dimension max_width = 0;

  {
    Dimension frame_margin_width = 0;
    Dimension frame_shadow_thickness = 0;

    XtVaGetValues(
      XtParent(XtParent(d_area)),
      XmNmarginWidth, &frame_margin_width,
      XmNshadowThickness, &frame_shadow_thickness,
      XmNwidth, &max_width,
      NULL
    );

    if(obj_arrange_type != WrapAny && obj_arrange_type != WrapByClass) {
      max_width = 32767;
    }

    max_width -= 2 * (frame_margin_width + frame_shadow_thickness);

    ERS_DEBUG(2, "set maximum drawing area width =" << max_width );
  }



    //
    // Draw Objects
    //

  {
    std::list<G_Object *>::iterator i;

      // reset shown attribute

    for(i = objects.begin(); i != objects.end(); ++i) {
      (*i)->set_non_shown();
    }


      // remember graphic class corresponding to current row

    const G_Class *g_class = 0;


      // draw top-level objects

    for(i = objects.begin(); i != objects.end(); ++i) {
      G_Object *o = *i;

      if(!is_top_level(o)) continue;  // skip nested objects
      o->draw(false);                 // calculate size of object

      ERS_DEBUG(2, "Object " << o->get_oks_object() << ": test x = " << x << ", y = " << y);

        // process and possibly wrap non-first item
      if(g_class && (obj_arrange_type != AsColumn)) {
        if(
          ((obj_arrange_type != WrapAny) && (obj_arrange_type != AsRow) && (o->get_graphic_class() != g_class)) ||
          ((x + o->get_mw() + g_context.get_params().get_drawing_area_x_margin()) >= max_width)
	) {
	  y += g_context.get_obj_dy() + row_max_y;
	  row_max_y = 0;
	  x = g_context.get_params().get_drawing_area_x_margin();
          ERS_DEBUG(2, "Object " << o->get_oks_object() << ": wrap x = " << x << ", y = " << y);
	}
      }

      g_class = o->get_graphic_class();


        // test 'y' coordinate

      if(y > 32767) {
        std::ostringstream text;
        text << "cannot display all objects because:"
                "  the y coordinate for object " << o->get_oks_object() << " is " << y << ",\n"
                "  but the max supported value is 32767.\n"
                "  If you are using a \"wrap to window size\" parameter, make\n"
                "  the window wider, or reduce number of objects to be shown.";
        ers::error(OksDataEditor::Problem(ERS_HERE, text.str().c_str()));
        break;
      }

      o->set_x(x);
      o->set_y(static_cast<Dimension>(y));


        //
	// do not display objects which are:
	//  - reason 1 - righter then view area
	//  - reason 2 - down then view area
	//  - reason 3 - lefter then view area
	//  - reason 4 - up then view area
	//

      if(
       ( x < (rect.get_x() + rect.get_width())  ) &&
       ( y < (rect.get_y() + rect.get_height()) ) &&
       ( (x + o->get_mw()) > rect.get_x() )       &&
       ( (y + o->get_mh()) > rect.get_y() )
      ) {
	ERS_DEBUG( 1, "redraw object " << o->get_oks_object() << " at (x=" << o->get_x() << ", y=" << o->get_y() << ')');
	o->draw(true);
      }
      else {
	ERS_DEBUG( 2, "object " << o->get_oks_object() << " at (x=" << o->get_x() << ", y=" << o->get_y()
	                        << ", w=" << o->get_mw() << ", h=" << o->get_mh() << ") is out of rect (x="
				<< rect.get_x() << ", y=" << rect.get_y()
	                        << ", w=" << rect.get_width() << ", h=" << rect.get_height() << ")");
      }


        // calculate position for next object

      Dimension mw = o->get_mw();
      Dimension mh = o->get_mh();

      if(obj_arrange_type == AsColumn) {
        y += mh + g_context.get_obj_dy();
        if(x_max_right < mw) x_max_right = mw;
      }
      else {
        x += mw + g_context.get_obj_dx();
        if(x_max_right < x) x_max_right = x;
	if(row_max_y < mh) row_max_y = mh;
      }

      ERS_DEBUG( 2, "set x max right = " << x_max_right << ", row_max_y = " << row_max_y << ", new x = " << x << ", y = " << y);
    }
  }


    // set right offsets depending on arrange type

  if(obj_arrange_type != AsColumn) {
    y += row_max_y;

//    if(obj_arrange_type != AsRow)
//      x_max_right -= g_context.get_obj_dx();
  }


    //
    // Set new size for DrawingForm widget
    // (only required if the size was changed!)
    //


  {
      // detect current size of window

    Dimension c_width = 0;
    Dimension c_height = 0;

    XtVaGetValues(d_area, XtNwidth, &c_width, XtNheight, &c_height, NULL);


      // calculate new size of window

    Dimension n_width = x_max_right + g_context.get_params().get_drawing_area_x_margin();
    
    y += g_context.get_params().get_drawing_area_y_margin();
    Dimension n_height = (y > 32767) ? 32767 : static_cast<Dimension>(y);

    ERS_DEBUG( 1, "drawing area width = " << n_width << ", height = " << n_height);


      // reset size of window when new and current sizes are different

    Arg	args[2];
    int	n = 0;

    if(c_width != n_width) {
      if(max_width != c_width || (obj_arrange_type != WrapAny && obj_arrange_type != WrapByClass)) {
        XtSetArg(args[n], XtNwidth, n_width); n++;
      }
    }

    if(c_height != n_height) {
      XtSetArg(args[n], XtNheight, n_height); n++;
    }

    if(n) {
      XtSetValues(d_area, args, n);
      ERS_DEBUG( 1, "set new drawing area width = " << n_width << ", height = " << n_height);
    }
  }


    //
    // Fix bugs with Lesstif on linux
    // the drop site has to be re-register with new XtNwidth and XtNheight values
    //

#if defined(__linux__) && (XmVersion < 2000)
  unregister_drop_site(d_area);
  register_drop_site(d_area);
#endif

  G_Context::enable_tooltips();

  ERS_DEBUG( 1, "exit draw()" );
}

G_Window::G_Window(const G_WindowInfo * w_info, OksDataEditorMainDialog * p) :
  OksDialog	      ("",  p),
  parent	      (p),
  p_win_info	      (w_info),
  title		      (w_info->title()),
  dnd_dest_obj	      (0),
  obj_arrange_type    (G_Window::WrapByClass),
  child_arrange_type  (G_Window::ManyPerLine)
{
  parent->create_g_classes();

  if(OksDataEditorMainDialog::is_verbose()) {
    std::cout << "* create G_Window(" << w_info->title() << ")\n";
  }

  setTitle(title.c_str());
  setIcon((OksDialog *)parent);
  
  create_drawing_area();
  
  const std::list<G_WindowInfo::Item *> & items = w_info->items();
  std::list<G_WindowInfo::Item *>::const_iterator i;

  for(i = items.begin(); i != items.end(); ++i) {
    G_WindowInfo::Item * item = *i;

    if(item->type() == G_WindowInfo::Item::ClassType) {
      G_WindowInfo::Class * citem = static_cast<G_WindowInfo::Class *>(item);
      G_WindowInfo::Class::TopLevelShow tls = citem->top_level_show();

      if(OksDataEditorMainDialog::is_verbose()) {
        std::cout << "  - check class \'" << citem->g_class()->get_name() << "\'\n";
      }

      if(tls != G_WindowInfo::Class::DoNotShow && citem->create_top_level_objs() == true)
        add_shown_menu(
          citem->top_level_name().c_str(),
	  citem->g_class(),
	  tls == G_WindowInfo::Class::ShowWithoutUsedMenu ? true : false
        );
    }
    else {
      G_WindowInfo::Separator * sitem = static_cast<G_WindowInfo::Separator *>(item);
      if(sitem->break_at() != G_WindowInfo::Separator::NewObject)
        add_shown_menu(0, 0, false);
    }
  }


    // create objects

  create_objects();

  for(i = items.begin(); i != items.end(); ++i) {
    G_WindowInfo::Item * item = *i;
    
    if(item->type() == G_WindowInfo::Item::ClassType) {
      G_WindowInfo::Class * citem = static_cast<G_WindowInfo::Class *>(item);
      G_WindowInfo::Class::TopLevelShow tls = citem->top_level_show();
      G_WindowInfo::Class::ShownWithChildren swc = citem->shown_with_children();


        // add to top-level rest of the objects

      if(tls != G_WindowInfo::Class::DoNotShow) {
        if(citem->create_top_level_objs() == false) {
          add_shown_menu(
            citem->top_level_name().c_str(),
	    citem->g_class(),
	    tls == G_WindowInfo::Class::ShowWithoutUsedMenu ? true : false
          );
	}
      }
      else {
        other_classes.push_back(citem->g_class());
      }


	// find objects which should be shown with children

      if(swc != G_WindowInfo::Class::NoneObj) {
        std::set<OksObject *> root_objects;
        if(swc == G_WindowInfo::Class::RootObj) {
          citem->g_class()->find_root_objects(citem->root_relationship_name(), root_objects);
        }

        for(std::list<G_Object *>::iterator i = objects.begin(); i != objects.end(); ++i) {
          G_Object * g_obj = *i;
          const char * show_reason = 0;

          if(swc == G_WindowInfo::Class::AnyObj && g_obj->get_graphic_class() == citem->g_class()) {
            show_reason = "any object of class";
          }

          if(root_objects.find(g_obj->get_oks_object()) != root_objects.end()) {
            show_reason = "root object";
          }

          if(show_reason) {
            g_obj->show_children();
            if(OksDataEditorMainDialog::is_verbose()) {
              std::cout << "  # show object " << g_obj->get_oks_object() << " with children (" << show_reason << ')' << std::endl;
            }
          }
        }
      }
    }
  }


    // create list of object datafiles

  add_in_list_of_files();
  add_separator();


    // add help & close buttons

  XtAddCallback(
    add_push_button(idHelp, "Help"),
    XmNactivateCallback, helpCB,
    0
  );

  XtAddCallback(
    add_push_button(OksDlgEntity::idCloseButton, "Close"),
    XmNactivateCallback, closeCB,
    (XtPointer)this
  );
  OksXm::set_close_cb(XtParent(get_form_widget()), closeCB, (void *)this);
  attach_previous(OksDlgEntity::idCloseButton);

  attach_bottom(idDrawingForm);


  show();
  g_context.init(d_area);
  g_context.initialize();


    // add resize callback and draw first time

  XtAddCallback(XtParent(XtParent(d_area)), XmNresizeCallback, resizeCB, (XtPointer)this);


    // add ConfDB GUI specific parameters

  g_context.get_params() = OksGC::Parameters::def_params();

}


void
G_Window::create_drawing_area()
{
  std::string s = title + " Objects";
  add_form(idDrawingForm, s.c_str());
 
  attach_right(idDrawingForm);

  d_area = XmCreateDrawingArea(
    get_form(idDrawingForm)->get_form_widget(),
    (char *)"DrawingArea",
    0,
    0
  );

  XtVaSetValues(d_area,
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
    XtNwidth, 200,
    XtNheight, 300,
    XmNresizePolicy, XmRESIZE_NONE,
    NULL
  );

  XtManageChild(d_area);


    // always show scrollbars
    // to avoid possible infinite loops when "wrap objects" is used

  XtVaSetValues(
    XtParent(XtParent(XtParent(d_area))),
    XmNscrollBarDisplayPolicy, XmSTATIC,
    NULL
  );


    // save the pointer to graphic window

  OksXm::set_user_data(d_area, (XtPointer)this);


    // make drawing area drop site

  register_drop_site(d_area);


  XtAddCallback(d_area, XmNexposeCallback, drawCB, (XtPointer)this);
  XtAddCallback(d_area, XmNinputCallback,  inputCB, (XtPointer)this);
  XtAddEventHandler(d_area, PointerMotionMask, False, motionAC, (XtPointer)this);
  XtAddEventHandler(d_area, LeaveWindowMask, False, leaveAC, (XtPointer)this);

  XtAddEventHandler(XtParent(XtParent(d_area)), ButtonPressMask, False, popupAC, (XtPointer)this);

  add_separator();


    // add mouse wheel actions on the drawing area

  OksXm::add_mouse_wheel_action(d_area); // mouse wheel actions on the drawing area
}


void
G_Window::add_in_list_of_files()
{
  std::list<const char *> files;

  files.push_back(choose_file_name_str);
  files.push_back(create_new_file_name_str);


  for(std::list<G_Object *>::iterator i = objects.begin(); i != objects.end(); ++i) {
    G_Object *o = *i;

    if(find(use_datafile_classes.begin(), use_datafile_classes.end(), o->get_graphic_class()) == use_datafile_classes.end()) continue;

    const std::string& file = o->get_oks_object()->get_file()->get_full_file_name();


      // check if the file is already in the list

    bool found = false;

    for(std::list<const char *>::iterator fi=files.begin(); fi != files.end(); ++fi) {
      if(file == *fi) {
        found = true;
	break;
      }
    }

    if(!found)
      files.push_back(file.c_str());
  }

  for(OksFile::Map::const_iterator j = parent->data_files().begin(); j != parent->data_files().end(); ++j) {
//    if(type() == (*j)->get_type()) { //// FIXME

        // check if the file is already in the list

      bool found = false;

      for(std::list<const char *>::iterator fi=files.begin(); fi != files.end(); ++fi) {
        if(j->second->get_full_file_name() == *fi) {
          found = true;
	  break;
        }
      }

      if(!found)
        files.push_back(j->second->get_full_file_name().c_str());
//    }
  }

  files.push_back("Datafile for\nnew objects:");

  Widget w = add_option_menu(idInFile, &files);
  set_option_menu_cb(idInFile, G_Window::changeDataFileCB, (XtPointer)this);

  Widget pulldown;

  XtVaGetValues(w, XmNsubMenuId, &pulldown, NULL);

  Cardinal num;
  Widget *children;
  
  XtVaGetValues(pulldown, XmNnumChildren, &num, XmNchildren, &children, NULL);

//  if(OksKernel::check_read_only(parent->get_root_file())) XtSetSensitive(children[1], false); //// FIXME

  for(unsigned int n=2; n<num; n++) {
    XmString s;
    XtVaGetValues(children[n], XmNlabelString, &s, NULL);
    OksXm::AutoCstring value(OksXm::string_to_c_str(s));
    XmStringFree (s);

    if(OksKernel::check_read_only(parent->find_data_file(value.get())))
      XtSetSensitive(children[n], false);
  }
  
  for(std::list<Widget>::iterator iw = file_sensible.begin(); iw != file_sensible.end(); ++iw)
    XtSetSensitive(*iw, false);
}


G_Window::~G_Window()
{
  unregister_drop_site(d_area);


     // remove all callbacks and event handlers

  XtRemoveAllCallbacks(d_area, XmNexposeCallback);
  XtRemoveAllCallbacks(d_area, XmNinputCallback);
  XtRemoveEventHandler(d_area, PointerMotionMask, False, motionAC, (XtPointer)this);
  XtRemoveEventHandler(d_area, LeaveWindowMask, False, leaveAC, (XtPointer)this);
  XtRemoveEventHandler(XtParent(XtParent(d_area)), ButtonPressMask, False, popupAC, (XtPointer)this);


     // remove objects

  for(std::list<G_Object *>::const_iterator i = objects.begin(); i != objects.end(); ++i) {
    (*i)->unlink_children();
  }

  while(!objects.empty()) {
    G_Object * o = objects.front();
    objects.pop_front();
    delete o;
  }

  objs_set.clear();


    // remove trash objects

  while(!trash_objs.empty()) {
    G_Object * o = trash_objs.front();
    trash_objs.pop_front();
    delete o;
  }


    // clear top-level classes

  top_classes.clear();


    // remove from parent and update menu bar

  parent->remove_window(this);
  parent->update_menu_bar();
}

void
G_Window::add_top_class(const G_Class *c)
{
  top_classes.push_back(c);
}

void
G_Window::remove_top_class(const G_Class *c)
{
  top_classes.remove(c);
}

void
G_Window::add_use_datafile_classes(const G_Class *c)
{
  use_datafile_classes.push_back(c);
}


bool
G_Window::is_top_level(const G_Object * obj) const
{
  if(obj->is_hiden() == true) return false;
  if(obj->get_propagate() == G_Kid::stop_propagate) return false;
  if(obj->is_referenced()) return false;

  for(std::list<const G_Class*>::const_iterator i = top_classes.begin(); i != top_classes.end(); ++i) {
    if(obj->get_graphic_class() == *i) {
      return true;
    }
  }

  return false;
}


void
G_Window::set_scrollbar_pos(Widget w, Dimension pos) const
{
  if(w) {
    int value, slider_size, increment, page_increment, max;

    XtVaGetValues(w, XmNmaximum, &max, NULL);
    XmScrollBarGetValues(w, &value, &slider_size, &increment, &page_increment);

    if(pos > (max - slider_size)) pos = max - slider_size;
    
    if(pos != value)
      XmScrollBarSetValues(w, pos, slider_size, increment, page_increment, true);
  }
}


void
G_Window::make_object_visible(const G_Object * obj) const
{
    // detect if the object should be drawn

    // if(!is_top_level(obj)) return;  // FIXME: check if needed


    // get vertical & horisontal scrollbars and set their position

  Widget v_sb_w = 0, h_sb_w = 0;

  XtVaGetValues(
    XtParent(XtParent(get_form(idDrawingForm)->get_form_widget())),
    XmNverticalScrollBar, &v_sb_w,
    XmNhorizontalScrollBar, &h_sb_w,
    NULL
  );

  set_scrollbar_pos(v_sb_w, obj->get_y());
  set_scrollbar_pos(h_sb_w, obj->get_x());
}


void
G_Window::make_shown(unsigned long count)
{
  G_Window::ShowInfo *info = 0;

  for(std::list<ShowInfo>::iterator i=show_infos.begin(); i != show_infos.end(); ++i) {
    if(!(*i).name) continue;

    if(!count--) {
      if((*i).show_state != G_Window::ShowAll) {
        (*i).show_state = G_Window::ShowAll;
	info = &(*i);
      }
    }

    if((*i).show_extended) {
      if(!count--) {
        if((*i).show_state != G_Window::ShowUsed) {
          (*i).show_state = G_Window::ShowUsed;
	  info = &(*i);
        }
      }

      if(!count--) {
        if((*i).show_state != G_Window::ShowNoneUsed) {
          (*i).show_state = G_Window::ShowNoneUsed;
	  info = &(*i);
        }
      }
    }

    if(!count--) {
      if((*i).show_state != G_Window::ShowNone) {
        (*i).show_state = G_Window::ShowNone;
	info = &(*i);
      }
    }
  }


  if(info) {
    for(std::list<G_Object *>::iterator i = objects.begin(); i != objects.end(); ++i) {
      G_Object *o = *i;


        // ignore objects of other grapchic classes

      if(o->get_graphic_class() != info->g_class) continue;


        // set given object shown or hiden

      if(info->show_state == ShowAll)
        o->show();
      else if(info->show_state == ShowNone)
        o->hide();
      else {
        if(o->is_used()) {
          if(info->show_state == ShowUsed) o->show();
	  else o->hide();
        }
        else {
          if(info->show_state == ShowUsed) o->hide();
	  else o->show();
        }
      }
    }
  }
}


void
G_Window::add_shown_menu(const char * name, const G_Class * c, bool reduced)
{
  if(OksDataEditorMainDialog::is_verbose()) {
    std::cout << "    call add_shown_menu(";
    if(c) {
      std::cout << c->get_name() << " points to (OksClass *)" << (void *)c->get_class();
    }
    else {
      std::cout << "(null)";
    }
    std::cout << ", reduced = " << reduced << ")\n";
  }


    // register information about shown classes

  show_infos.push_back(ShowInfo(name, c, !reduced));

  if(!c || !c->get_class()) return;


    // show objects of given class as entry point

  add_top_class(c);
  
  
    // only objects of fully-shown classes are taking into account
    // when create list of database files for update

  if(reduced == false)
    add_use_datafile_classes(c);
}


void
G_Window::remove_from_set(G_Object * g_obj)
{
  std::pair<G_Object::G_Set::iterator, G_Object::G_Set::iterator> ppos = objs_set.equal_range(g_obj);
  G_Object::G_Set::iterator i = ppos.first;
  G_Object::G_Set::iterator i2 = ppos.second;

  for(; i != i2; ++i) {
    if(g_obj == *i) {
      objs_set.erase(i);
      return;
    }
  }
}


void
G_Window::delete_object(const OksObject *oks_obj)
{
  bool remove_obj = false;

  std::list<G_Object *>::iterator i = objects.begin();


    // remove objects pointed to the same oks object

  for(; i != objects.end();) {
    G_Object *obj = *i;

    if(obj->get_oks_object() == oks_obj || parent->is_dangling(obj->get_oks_object())) {
      i = objects.erase(i);
      remove_from_set(obj);
      trash_objs.push_back(obj);
      obj->unlink_children();
      remove_obj = true;
    }
    else
      ++i;
  }


    //
    // if an object was deleted
    //

  if(remove_obj) {
    std::list<const G_Class *>::iterator begin_pos = top_classes.begin();
    std::list<const G_Class *>::iterator end_pos = top_classes.end();

    while(remove_obj) {
      ERS_DEBUG( 2, "enter remove object loop deleting object " << (void *)oks_obj);
      remove_obj = false;

        // set child->parent to null if the parent was deleted
      for(i = objects.begin(); i != objects.end(); ++i)
        (*i)->reset_children(&objs_set);

        // remove non-top-level objects which lost parent
      for(i = objects.begin(); i != objects.end();) {
        G_Object *obj = *i;

        if(
	 ( !obj->is_referenced() ) &&
	 (
	  ( obj->get_propagate() == G_Kid::stop_propagate ) ||
	  ( find(begin_pos, end_pos, obj->get_graphic_class()) == end_pos)
	 )
	) {
          ERS_DEBUG( 2, "remove graphical object " << (void *)obj << " => " << obj->get_oks_object());

          i = objects.erase(i);
	  remove_from_set(obj);
          trash_objs.push_back(obj);
          obj->unlink_children();
          remove_obj = true;
        }
	else
	  ++i;
      }
    }

      // reset number of object copies
    G_Object::set_number_of_copies(&objs_set);

      // redraw window
    draw(0);
  }
}


void
G_Window::remove_object_parent_link(const OksObject *obj, const OksObject *p_obj, const char * rel_name)
{
  std::list<G_Object *>::iterator i = objects.begin();
  bool updated = false;

  std::list<G_Object *> parents;


    //
    // iterate objects and look for parent
    //

  for(; i != objects.end(); ++i)
    if((*i)->get_oks_object() == p_obj)
      parents.push_back((*i));

  while(!parents.empty()) {
    G_Object * o = parents.front();
    parents.pop_front();

    if(o->unlink_child(obj, rel_name)) updated = true;
  }

    //
    // redraw window if an object was updated
    //

  if(updated) {
    std::list<const G_Class *>::iterator begin_pos = top_classes.begin();
    std::list<const G_Class *>::iterator end_pos = top_classes.end();

    while(updated) {
      updated = false;

        // remove non-top-level objects which lost parent
      for(i = objects.begin(); i != objects.end();) {
        G_Object *lost_obj = *i;

        if(
	 ( !lost_obj->is_referenced() ) &&
	 (
	  ( lost_obj->get_propagate() == G_Kid::stop_propagate ) ||
	  ( find(begin_pos, end_pos, lost_obj->get_graphic_class()) == end_pos)
	 )
	) {
          i = objects.erase(i);
	  remove_from_set(lost_obj);
          trash_objs.push_back(lost_obj);
          lost_obj->unlink_children();
          updated = true;
        }
	else
	  ++i;
      }
    }

    G_Object::set_number_of_copies(&objs_set);
    draw(0);
  }
}


void
G_Window::swap_rel_objects(OksObject *parent_oks_obj, const char *rel_name, const OksObject *o1, const OksObject *o2)
{
// FIXME objs_set
  for(std::list<G_Object *>::iterator i = objects.begin(); i != objects.end(); ++i)
    if((*i)->get_oks_object() == parent_oks_obj)
      (*i)->swap_rel_objects(rel_name, o1, o2);
}


void
G_Window::create_child_objects(G_Object *g_obj, std::list<G_Object *>::iterator& i_obj, bool link_children)
{
  if(OksDataEditorMainDialog::is_verbose()) {
    std::cout << " - call G_Window::create_child_objects(" << g_obj->get_oks_object() << ")\n";
  }

  const G_Class * obj_g_class = g_obj->get_graphic_class();

    // temporal container used to draw children of object without defined class

  std::list<std::string> r_list;

  if(obj_g_class->get_class() == 0) {
    const std::list<OksRelationship *> * c_list = g_obj->get_oks_object()->GetClass()->all_relationships();
    if(c_list)
      for(std::list<OksRelationship *>::const_iterator j = c_list->begin(); j != c_list->end(); ++j)
        r_list.push_back((*j)->get_name());
  }

    // initialize reference

  const std::list<std::string> * rels = &r_list;
  if(obj_g_class->get_class() != 0) rels = obj_g_class->get_relationships();


  for(std::list<std::string>::const_iterator i = rels->begin(); i != rels->end(); ++i) {
    const OksObject *oks_obj = g_obj->get_oks_object();
    const OksClass *oks_class = oks_obj->GetClass();
    const OksRelationship *oks_rel = oks_class->find_relationship(*i);
    
    if(oks_rel) {
      try {
        OksData *d(oks_obj->GetRelationshipValue(*i));

        bool use_propagate = (
	  G_Kid::get_propagate_type(oks_class, oks_rel) == G_Kid::use_propagate ? true : false
	);
        
	if(d->type == OksData::object_type && d->data.OBJECT) {
	  OksObject *child_oks_obj = d->data.OBJECT;
	  
	  if(OksDataEditorMainDialog::is_verbose()) {
	    std::cout << "  - looking for " << child_oks_obj << " ... ";
	  }
	  
          G_Object * child_g_obj = G_Object::find_g_object(child_oks_obj, &objs_set, true);

	  if(!child_g_obj) {
            if(OksDataEditorMainDialog::is_verbose()) {
	      std::cout << "need to create new\n";
            }

            G_Object * child_g_obj = create_new_object(child_oks_obj, use_propagate);

	    if(child_g_obj) {
              objects.push_back(child_g_obj);
	      objs_set.insert(child_g_obj);
	      create_child_objects(child_g_obj, i_obj, link_children);
	      if(link_children) {
	        child_g_obj->set_children(&objects, &objs_set);
	      }
	    }
	  }
	  else if(OksDataEditorMainDialog::is_verbose()) {
	    std::cout << "found already created (" << (void *)child_g_obj << ")\n";
	  }
	}
	else if(d->type == OksData::uid_type || d->type == OksData::uid2_type) {
          std::ostringstream text;
          text << "object \'" << *d << "\' referenced by object " << oks_obj << " via \'" << *i << "\' is not loaded";
          ers::error(OksDataEditor::Problem(ERS_HERE, text.str().c_str()));
	}
	else if(d->type == OksData::list_type && d->data.LIST){
	  OksData::List * dlist = d->data.LIST;
	  for(OksData::List::iterator j = dlist->begin(); j != dlist->end(); ++j) {
	    OksData *d2 = *j;

	    if(d2->type == OksData::object_type && d2->data.OBJECT) {
	      OksObject *child_oks_obj = d2->data.OBJECT;

  	      if(OksDataEditorMainDialog::is_verbose()) {
	        std::cout << "  - looking for " << child_oks_obj << " ... ";
	      }

              G_Object * child_g_obj = G_Object::find_g_object(child_oks_obj, &objs_set, true);

	      if(!child_g_obj) {
                if(OksDataEditorMainDialog::is_verbose()) {
	          std::cout << "need to create new\n";
                }

	        G_Object * child_g_obj = create_new_object(child_oks_obj, use_propagate);

	        if(child_g_obj) {
                  objects.push_back(child_g_obj);
                  objs_set.insert(child_g_obj);
	          create_child_objects(child_g_obj, i_obj, link_children);
		  if(link_children) {
	            child_g_obj->set_children(&objects, &objs_set);
		  }
	        }
	      }
	      else if(OksDataEditorMainDialog::is_verbose()) {
	        std::cout << "found already created (" << (void *)child_g_obj << ")\n";
	      }
	    }
	    else if(d2->type == OksData::uid_type || d2->type == OksData::uid2_type) {
              std::ostringstream text;
              text << "object \'" << *d2 << "\' referenced by object " << oks_obj << " via \'" << *i << "\' is not loaded";
              ers::error(OksDataEditor::Problem(ERS_HERE, text.str().c_str()));
	    }
	  }
	}
      }
      catch(oks::exception& ex) {
        ers::error(OksDataEditor::InternalProblem(ERS_HERE, ex.what()));
      }
    }
  }

  if(OksDataEditorMainDialog::is_verbose()) {
    std::cout.flush();
  }
}


void
G_Window::link_rel_objects(const char *rel_name, OksObject *parent_oks_obj, OksObject *child_oks_obj)
{
  ERS_DEBUG ( 2, "rel_name: " << rel_name << ", parent: " << parent_oks_obj << ", child: " << child_oks_obj );

  const OksClass *parent_oks_class = parent_oks_obj->GetClass();

  OksRelationship *oks_rel = parent_oks_class->find_relationship(rel_name);

  if(oks_rel) {
    G_Kid::Propagate kid_propagate = G_Kid::get_propagate_type(parent_oks_class, oks_rel);
    bool use_propagate = (kid_propagate == G_Kid::use_propagate ? true : false);
    bool modified = false;
    
    std::list<G_Object *>::iterator i;

    for(i = objects.begin(); i != objects.end(); ++i)
      if((*i)->get_propagate() == G_Kid::use_propagate && (*i)->get_oks_object() == parent_oks_obj) {

          // test if graphic object already shown in this window

        G_Object * g_obj = G_Object::find_g_object(child_oks_obj, &objs_set, true);


          // create graphic object if it does not exist in this window

        if(g_obj) {
	  if(g_obj->is_referenced() || kid_propagate == G_Kid::stop_propagate) {
            g_obj = g_obj->make_copy(kid_propagate);
	    g_obj->set_children(&objects, &objs_set);
	    objects.push_back(g_obj);
	    objs_set.insert(g_obj);
	  }
	}
        else {
          g_obj = create_new_object(child_oks_obj, use_propagate);

	  if(g_obj) {
	    create_child_objects(g_obj, i, true);
	    g_obj->set_children(&objects, &objs_set);
	    objects.push_back(g_obj);
	    objs_set.insert(g_obj);
	  }
        }

        if(g_obj) {
	  G_Object * new_obj = (*i)->link_with_object(rel_name, g_obj);
	  if(parent->uses_object(child_oks_obj)) g_obj->set_used(true);
          modified = true;

	  if(new_obj) {
	    objects.push_back(new_obj);
	    objs_set.insert(new_obj);
	  }
	}
      }

    if(modified) {

        // remove non-toplevel objects which lost parent

      for(i = objects.begin(); i != objects.end();) {
	G_Object *lost_obj = *i;

        if(lost_obj->get_oks_object() == 0) {
	  i = objects.erase(i);
	  remove_from_set(lost_obj);
	  delete lost_obj;
	}
	else
	  ++i;
      }


        // reset numbers ob object copies and redraw window

      G_Object::set_number_of_copies(&objs_set);
      draw(0);
    }
  }
}


G_Object *
G_Window::find_object(Dimension x, Dimension y) const
{
  for(std::list<G_Object *>::const_iterator i = objects.begin(); i != objects.end(); ++i) {
    G_Object *o = *i;

    if(o->is_point_inside_object(x, y) && !o->is_hiden() && o->is_shown())
      return o;
  }

  return 0;
}


static int
is_less(G_Object * o1, G_Object * o2)
{
  return (o1->get_id() < o2->get_id());
}


void
G_Window::sort_by_id()
{
  size_t n = objects.size();
  if(!n) return;

  std::vector<G_Object *> vec;
  vec.reserve(n);

  while(!objects.empty()) {
    vec.push_back(objects.front());
    objects.pop_front();
  }
  
  std::sort(vec.begin(), vec.end(), is_less);

  for(std::list<const G_Class *>::iterator j = top_classes.begin();j != top_classes.end(); ++j) {
    for(size_t i=0;i<n;i++)
      if(vec[i] && vec[i]->get_graphic_class() == *j) {
        objects.push_back(vec[i]);
	vec[i] = 0;
      }
  }

  for(size_t i=0;i<n;i++)
    if(vec[i])
      objects.push_back(vec[i]);
}


void
G_Window::change_used(std::list<OksObject *>& olist, bool used)
{
  std::list<OksObject *>::iterator j = olist.begin();

  for(std::list<G_Object *>::iterator i = objects.begin(); i != objects.end(); ++i) {
    G_Object *g_obj = *i;

    for(j=olist.begin(); j != olist.end(); ++j)
      if(*j == g_obj->get_oks_object()) {
        g_obj->set_used(used);
        if(!g_obj->is_hiden() && g_obj->is_shown()) g_obj->draw();
	break;
      }
  }
}


int
G_Window::show_references(const G_Object *g_obj)
{
  G_Object::RefTree ref_tree("");

  for(std::list<G_Object *>::iterator i = objects.begin(); i != objects.end(); ++i) {
    if((*i) == g_obj) {
      for(i = objects.begin(); i != objects.end(); ++i) {
	std::list<std::pair<const G_Object *, const char *> > the_list;
	if((*i)->get_parent() == 0) {

            // print out name of window if the object is referenced from top

	  if((*i)->get_oks_object() == g_obj->get_oks_object()) {
	    std::string s("Window \'");
	    s += get_title();
	    s += '\'';
	    ref_tree.get(s);
	  }

	  (*i)->get_references(g_obj, &the_list, ref_tree);
	}
      }

      break;  // FIXME: check if this break works
    }
  }

  if(!ref_tree.values.empty()) {
    new ConfdbGuiRefTree(g_obj, get_parent(), ref_tree, &g_context);
  }

  return 0;
}

void
G_Window::show_relationships_ob_object(G_Object * p)
{
  G_Object * p2 = p;

  while(p2) {
    if(p2->get_composite_state() == G_Object::single) {
      p2->show_children();
      p2->set_invalid();
    }
    p2 = p2->get_parent();
  }

  make_object_visible(p);
  draw(0);
}

void
G_Window::okShowRelsCB(Widget, XtPointer data, XtPointer)
{
  ObjAndWindow * p = reinterpret_cast<ObjAndWindow*>(data);
  p->p_win->show_relationships_ob_object(p->p_obj);
  delete p;
}

void
G_Window::cancelShowRelsCB(Widget, XtPointer data, XtPointer)
{
  delete reinterpret_cast<ObjAndWindow*>(data);
}

bool
G_Window::show_relationships(const G_Object *g_obj)
{
    // search for given graphical object
  {
    bool found = false;
    for(std::list<G_Object *>::iterator i = objects.begin(); i != objects.end(); ++i) {
      if((*i) == g_obj) {
        found = true;
        break;
      }
    }

    if(found == false) return false;
  }

  G_Object * p = 0;

  for(std::list<G_Object *>::iterator i = objects.begin(); i != objects.end(); ++i) {
    if((*i)->get_oks_object() == g_obj->get_oks_object()) {
      if((*i)->get_number_of_children()) {
        p = *i;
        break;
      }
    }
  }

  {
    std::ostringstream s;

    s << "Object " << g_obj->get_oks_object() << " is referenced by " << g_obj->get_parent()->get_oks_object() << ".\n"
         "Also in this view it is referenced " << g_obj->get_number_of_copies() << " times by other objects.\n"
         "The relationships of the " << g_obj->get_oks_object() << " cannot be shown here.\n"
         "Instead you can see them at ";

    if(p->get_parent()) s << "parent object " << p->get_parent()->get_oks_object();
    else s << "top level of window";
    s << ".\nPress [OK] to go there and see relationships or [Cancel] to abandon the operation.\n";

    Widget dlg = OksXm::create_dlg_with_pixmap(get_form_widget(), "Move and Show Relationships", exit_xpm, s.str().c_str(), "OK", "Cancel", 0, 0, 0);

    XtVaSetValues(dlg, XmNdialogStyle, XmDIALOG_APPLICATION_MODAL, NULL);

    ObjAndWindow * data = new ObjAndWindow(this, p);


    XtAddCallback(dlg, XmNokCallback, okShowRelsCB, reinterpret_cast<XtPointer>(data));
    XtAddCallback(dlg, XmNcancelCallback, cancelShowRelsCB, reinterpret_cast<XtPointer>(data));
    OksXm::set_close_cb(XtParent(dlg), cancelShowRelsCB, reinterpret_cast<XtPointer>(data));

    XtManageChild(dlg);
  }

  return true;
}
void
G_Window::create_objects()
{
  if(OksDataEditorMainDialog::is_verbose()) {
    std::cout << "Create top-classes objects in window \'" << title << '\'' << std::endl;
  }

  for(std::list<const G_Class *>::iterator j = top_classes.begin(); j != top_classes.end(); ++j) {
    if(OksDataEditorMainDialog::is_verbose()) {
      std::cout << "* create objects of class \'" << (*j)->get_name() << '\'' << std::endl;
    }

    if(std::list<OksObject *> * olist = (*j)->get_class()->create_list_of_all_objects()) {
      while(!olist->empty()) {
        if(G_Object::find_g_object(olist->front(), &objs_set, true) == 0) {
          G_Object * new_obj = create_new_object(olist->front(), true);
          objects.push_back(new_obj);
          objs_set.insert(new_obj);
	}
	olist->pop_front();
      }

      delete olist;
    }
  }

  if(OksDataEditorMainDialog::is_verbose()) {
    std::cout << "Create children of top-classes objects in window \'" << title << '\'' << std::endl;
  }

  std::list<G_Object *>::iterator i;

  for(i = objects.begin(); i != objects.end() ; ++i) {
    G_Object *o = *i;
    create_child_objects(o, i, false);
    if(get_parent()->uses_object(o->get_oks_object())) o->set_used(true);
  }


    // link objects (i.e. set children objects)

  if(OksDataEditorMainDialog::is_verbose()) {
    std::cout << "Link objects in window \'" << title << '\'' << std::endl;
  }

  for(i = objects.begin(); i != objects.end() ; ++i) {
    G_Object *o = *i;
    o->set_children(&objects, &objs_set);
  }


  G_Object::set_number_of_copies(&objs_set);
}


void
G_Window::create_new_datafile(const char * file_name, const char * file_id)
{
    // create new data file

  try {

    OksFile * fh = parent->new_data(file_name);

    parent->refresh_data();

      // remember that oks xm library needs pointer to string which is valid during
      // the window life-time, so get the pointer from oks kernel and do not use
      // 'file_name' pointer which will be destroyed before return from the method

    const std::string& data_file = parent->get_active_data()->get_full_file_name();
    Widget new_btn_w = set_value(idInFile, data_file.c_str());

    if(new_btn_w) {
      XtAddCallback(new_btn_w, XmNactivateCallback, G_Window::changeDataFileCB, (XtPointer)this);
      changeDataFileCB(new_btn_w, this, 0);
//FIXME
//      parent->create_new_datafile(data_file.c_str(), file_id);
//      parent->set_data_file_type(data_file, type());

      fh->set_type(file_id);

        // add to the "root" database file

      if(!parent->p_root_objects.empty()) {
        OksFile * root_fh = (*parent->p_root_objects.begin())->get_file();
	root_fh->add_include_file(file_name);
	parent->update_data_file_row(root_fh);
      }
    }

  }
  catch (oks::exception & ex) {
    OksDataEditorMainDialog::report_exception("Create New Data File", ex, get_form_widget());
  }
}


void
G_Window::update_object_id(const OksObject * oks_obj)
{
  for(std::list<G_Object *>::iterator i = objects.begin(); i != objects.end(); ++i) {
    G_Object *g_obj = *i;
    if(g_obj->get_oks_object() == oks_obj) {
      draw(0);
      return;
    }
  }
}


  //
  // Returns true if there is a chosen datafile for creation of new objects
  //

bool
G_Window::is_data_file_choosen() const
{
  XmString s;
  Widget files_w = get_widget(idInFile);
  Widget btn = 0;
  XtVaGetValues(files_w, XmNmenuHistory, &btn, NULL);
  XtVaGetValues(btn, XmNlabelString, &s, NULL);
  OksXm::AutoCstring s_file_name(OksXm::string_to_c_str(s));

  return (strcmp(s_file_name.get(), choose_file_name_str) && strcmp(s_file_name.get(), create_new_file_name_str));
}


std::list<const std::string *> *
G_Window::get_list_of_new_objects(const G_Object * o) const
{
  for(std::list<G_Object *>::const_iterator i = objects.begin(); i != objects.end(); ++i) {
    if(*i == o) {
      return (
        o->get_list_of_new_objects(
          &top_classes,
          &other_classes,
          is_data_file_choosen()
        )
      );
    }
  }

  return 0;
}


bool
G_Window::set_selected_file_active() const
{
  XmString s;
  Widget files_w = get_widget(idInFile);
  Widget btn = 0;
  XtVaGetValues(files_w, XmNmenuHistory, &btn, NULL);
  XtVaGetValues(btn, XmNlabelString, &s, NULL);

  OksXm::AutoCstring file_name(OksXm::string_to_c_str(s));
  OksFile * fh = parent->find_data_file(file_name.get());

  if(parent->get_active_data() != fh) {
    try {
      parent->set_active_data(fh);
      parent->refresh_data();
    }
    catch(oks::exception& ex) {
      OksDataEditorMainDialog::report_exception("Set Selected File Active", ex, get_form_widget());
      return false;
    }
  }

  return true;
}

void
G_Object::report_exception(const char * title, const oks::exception& ex) const
{
  OksDataEditorMainDialog::report_exception(title, ex, p_window->get_form_widget());
}

void
G_Object::report_error(const char * title, const char * message) const{
  OksDataEditorMainDialog::report_error(title, message, p_window->get_form_widget());
}

