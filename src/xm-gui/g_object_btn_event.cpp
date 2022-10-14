#include <iostream>

#include <string>

#include <ers/ers.h>

#include <oks/file.h>
#include <oks/relationship.h>
#include <oks/xm_utils.h>
#include <oks/xm_popup.h>

#include "g_class.h"
#include "g_context.h"
#include "g_dnd.h"
#include "g_object.h"
#include "g_window.h"
#include "data_editor_main_dlg.h"
#include "data_editor_exceptions.h"

	//
	// used when process mouse events on object's table
	//

G_Object * 	G_Object::object_with_pressed_icon_button  = 0;
G_Object * 	G_Object::object_with_pressed_child_button = 0;
G_Object * 	G_Object::object_with_pressed_button       = 0;

G_TipsTool *	G_Object::last_tip = 0;

	//
	// Is used by popup menu:
	//  it is 1 when popup menu is shown,
	//  and 0 when it is not shown
	//

static long show_popup_menu = 0;
static XtIntervalId timer = 0;


	//
	// Define functions to enable and disable tooltips
	//

static int enable_ttips = 1;

void
G_Context::enable_tooltips()
{
  enable_ttips = 1;
}

void
G_Context::disable_tooltips()
{
  enable_ttips = 0;
}


	//
	// Creates object popup menu (for system button or icon)
	// The 'Children' menu is available if the object has a child
	//

void
G_Object::create_popup_menu(XButtonEvent *event, G_Window * win)
{
  std::list<const std::string *> * rlist = get_list_of_new_objects();

  {
    OksPopupMenu popup(XtParent(gc->get_widget()));
    popup.setDestroyAC(systemButtonPopupUnmapNotifyAC, (XtPointer)this);

    if(!parent)
      popup.addItem("Hide", idHide, systemButtonPopupCB, (XtPointer)this);
  
    if(get_complex_state() == G_Object::table)
      popup.addItem("Minimize", idMinimize, systemButtonPopupCB, (XtPointer)this);
    else
      popup.addItem("Maximize", idMaximize, systemButtonPopupCB, (XtPointer)this);

    if(number_of_children) {
      if(get_composite_state() == G_Object::complete)
        popup.addItem("Hide relationships", idHideChildren, systemButtonPopupCB, (XtPointer)this);
      else
        popup.addItem("Show relationships", idShowChildren, systemButtonPopupCB, (XtPointer)this);
    }
    else {

        // check if object actually has some relationships

      bool has_rels = false;
      
      const std::list<std::string> * rels = g_class->get_relationships();

      if(rels && !rels->empty()) {
	for(std::list<std::string>::const_iterator j = rels->begin(); j != rels->end(); ++j) {
	  if(obj->GetClass()->find_relationship(*j) != 0) {
	    OksData *d(obj->GetRelationshipValue((*j)));

	    if(d) {
	      if(
	       (d->type == OksData::object_type && d->data.OBJECT) ||
	       (d->type == OksData::uid_type && d->data.UID.object_id) ||
	       (d->type == OksData::list_type && d->data.LIST && !d->data.LIST->empty())
	      ) {
	        has_rels = true;
	        break;
	      }
	    }
	  }
	}
      }

      if(has_rels) {
        popup.addItem("Show relationships", idShowChildrenTopObject, systemButtonPopupCB, (XtPointer)this);
      }
      else {
        popup.addDisableItem("No relationships");
      }
    }

    popup.add_separator();

    popup.addItem("Copy Reference", idCopyReference, systemButtonPopupCB, (XtPointer)this);

    bool is_read_only = OksKernel::check_read_only(obj->get_file());

    if(get_clipboard_obj()) {
      std::string s("Link with ");
      s += to_string(get_clipboard_obj());

      if(is_read_only || get_propagate() != G_Kid::use_propagate )
        popup.addDisableItem(s.c_str());
      else
        popup.addItem(s.c_str(), idSetLink, systemButtonPopupCB, (XtPointer)this);
    }
    else
      popup.addDisableItem("Link with (null)");

    popup.add_separator();

    if(parent || number_of_copies > 1)
      popup.addItem("Referenced By", idReferencedBy, systemButtonPopupCB, (XtPointer)this);
    else
      popup.addDisableItem("Referenced By");

    popup.addItem("Contained in", idContainedIn, systemButtonPopupCB, (XtPointer)this);

    popup.add_separator();

    if(win->is_data_file_choosen()) {
      popup.addItem("Copy", idCopy, systemButtonPopupCB, (XtPointer)this);
    }
    else {
      popup.addDisableItem("Copy");
    }

    if(is_read_only) {
      popup.addDisableItem("Rename");
    }
    else {
      popup.addItem("Rename",  idRename, systemButtonPopupCB, (XtPointer)this);
    }

    if(get_complex_state() == G_Object::icon) {
      if(is_read_only) {
        popup.addDisableItem("Modify");
      }
      else {
        popup.addItem("Modify",  idModify, systemButtonPopupCB, (XtPointer)this);
      }
    }

    popup.add_separator();

    if(rlist) {
      Widget cascade = popup.addCascade("Create Child");
      const std::string& s_enabled = *rlist->front();
      bool enabled = ( s_enabled == "Enabled" && !is_read_only ? true : false );
      rlist->pop_front();
      popup.add_separator();

      int count = 0;

      for(std::list<const std::string *>::iterator li = rlist->begin(); li != rlist->end();) {
	std::string s(*(*li));
	++li;
	s += " new \'";
	s += *(*li);
	++li;
	s += "\' via \'";
	s += *(*li);
	++li;
	s += '\'';

	if(enabled)
          popup.addItem(s.c_str(), idAddNew + (count++), systemButtonPopupCB, (XtPointer)this, cascade);
	else
          popup.addDisableItem(s.c_str(), cascade);
      }
    }

    if(is_read_only) {
      popup.addDisableItem("Delete");
    }
    else {
      popup.addItem("Delete",  idDelete, systemButtonPopupCB, (XtPointer)this);
    }


      // allow remove object from relationship only if parent exists and writable
      // and detect if the object can be unlinked

    if(parent && !OksKernel::check_read_only(parent->obj->get_file())) {

        // check constraints of direct relationship

      const char * rel_name = 0;
      size_t num_of_rel_objs = 0;

      for(std::list<G_Kid *>::const_iterator i = parent->children.begin(); i != parent->children.end() && !rel_name; ++i) {
        const G_Kid *c = *i;

        if(c->get_cardinality() == G_Kid::simple) {
          const G_Child *child = static_cast<const G_Child *>(c);
          if(child->object == this) {
	    rel_name = c->rel_name;
	  }
        }
        else {
          const G_Children *child = static_cast<const G_Children *>(c);
	  for(std::list<G_Object *>::const_iterator j = child->objects.begin(); j != child->objects.end(); ++j) {
	    if(*j == this) {
	      rel_name = c->rel_name;
	      num_of_rel_objs = child->objects.size();
	      break;
	    }
          }
        }
      }

      const OksRelationship * direct_rel = parent->obj->GetClass()->find_relationship(rel_name);
      
      bool is_valid = (direct_rel->get_low_cardinality_constraint() == OksRelationship::Zero || num_of_rel_objs > 1);


        // check constraints of reverse relationship (if it exists)

      if(is_valid) {
        const char * reverse_rel_name = parent->get_reverse_relationship(rel_name);
	
	if(reverse_rel_name) {
	  const OksRelationship * reverse_rel = obj->GetClass()->find_relationship(reverse_rel_name);

	  if(reverse_rel->get_low_cardinality_constraint() != OksRelationship::Zero) {
	    try {
	      OksData * rev_d(obj->GetRelationshipValue(reverse_rel_name));
	  
	      if(
	       rev_d->type == OksData::object_type ||
	       (rev_d->type == OksData::list_type && rev_d->data.LIST->size() < 2)
	      ) is_valid = false;
	    }
	    catch(oks::exception& ex) {
              ers::error(OksDataEditor::InternalProblem(ERS_HERE, ex.what()));
	      is_valid = false;
	    }
	  }
        }
      }

      if(is_valid)
        popup.addItem("Unlink with parent", idUnlinkWithParent, systemButtonPopupCB, (XtPointer)this);
      else
        popup.addDisableItem("Unlink with parent");
    }


    popup.show((XButtonPressedEvent *)event);

    show_popup_menu = -1;

    while(show_popup_menu == -1)
      XtAppProcessEvent(XtWidgetToApplicationContext(gc->get_widget()), XtIMAll);
  }


    // process the link below after first popup menu is destroyed

  if(show_popup_menu == idSetLink) {
    DragAndDropTransferResult *t_info = get_user_dnd_action(get_clipboard_obj(), event);

    if(t_info) {
      if(t_info->get_type() == DragAndDropTransferResult::link)
        OksDataEditorMainDialog::link_rel_objects(
	  t_info->get_rel_name(),
	  const_cast<G_Object *>(this),
	  get_clipboard_obj()
        );

      delete t_info;
    }
  }
  else if(show_popup_menu == idRename) {
    std::string title("Rename object ");
    title += to_string(get_oks_object());
    std::string new_id = OksXm::ask_name(gc->get_widget(), "Rename object", title.c_str(), get_id().c_str(), renameHelpCB, (XtPointer)parent);
    if(!new_id.empty() && new_id != get_id()) {
      try {
        get_oks_object()->set_id(new_id);
        update_object_id(get_oks_object());
      }
      catch(oks::exception& ex) {
        report_exception("Rename Object", ex);
      }
    }
  }
  else if(show_popup_menu == idCopy) {
    std::string title("Copy object ");
    title += to_string(get_oks_object());
    std::string new_id = OksXm::ask_name(gc->get_widget(), "Copy object", title.c_str(), get_id().c_str(), renameHelpCB, (XtPointer)parent);
    if(!new_id.empty()) {
      if(obj->GetClass()->get_object(new_id)) {
        std::ostringstream text;
        text << "object with id " << obj << " already exists in class \"" << obj->GetClass()->get_name() << '\"';
        report_error("Copy object", text.str().c_str());
      }
      else {
        if(win->set_selected_file_active()) {
          try {
            OksObject * o = new OksObject(*obj, new_id.c_str());
            if(G_Object * obj = G_Object::find_g_object(o, &win->objs_set, true)) {
              obj->set_table_state();
              win->draw(0);
              win->make_object_visible(obj);
            }
          }
          catch(oks::exception& ex)
          {
            report_error("Copy object", ex.what());
          }
	}
      }
    }
  }
  else if(show_popup_menu >= idAddNew) {
    int count = show_popup_menu - idAddNew;
    std::list<const std::string *>::iterator li = rlist->begin();

    while(count--) {
      ++li;++li;++li;
    }

    ++li;  // skip action

    const std::string& class_name = *(*li);
    ++li;

    add_child(class_name, *(*li));
 }

  delete rlist;
}


	//
	// Callback to process system button popup callbacks
	//

void
G_Object::systemButtonPopupCB(Widget w, XtPointer client_data, XtPointer)
{
  G_Object *obj = (G_Object *)client_data;
  
  long action = (long)OksXm::get_user_data(w);

  switch(action) {
    case idMinimize:
      obj->set_icon_state();
      obj->set_invalid();
      break;

    case idModify:
    case idMaximize:
      obj->set_table_state();
      obj->set_invalid();
      break;

    case idShowChildren:
      obj->show_children();
      obj->set_invalid();
      break;

    case idShowChildrenTopObject:
      OksDataEditorMainDialog::show_relationships(obj);
      break;

    case idHideChildren:
      obj->hide_children();
      obj->set_invalid();
      break;

    case idHide:
      obj->hide();
      obj->set_invalid();
      break;

    case idCopyReference:
      set_clipboard_obj(obj->get_oks_object());
      break;

    case idReferencedBy:
      OksDataEditorMainDialog::show_references(obj);
      break;

    case idContainedIn:
      obj->show_contained_in();
      break;

    case idDelete:
      ERS_DEBUG( 1, "delete object " << obj);
      try {
        OksObject::destroy(obj->get_oks_object());
      }
      catch(oks::exception& ex) {
        obj->report_exception("Delete Object", ex);
      }
      break;

    case idSetLink:
    case idAddNew:
    case idCopy:
    case idRename:
        // process the action in caller
      show_popup_menu = action;
      break;

    case idUnlinkWithParent:
      remove_parent_link(obj);
      break;

    default:
        // process idAddNew in caller
      if(action >= idAddNew) show_popup_menu = action;
      break;
  }
}

void
G_Object::unpress_table_buttons()
{
    // unpress the minimize button if there is an object with pressed one

  if(object_with_pressed_icon_button) {
    object_with_pressed_icon_button->draw_table_minimize_btn();
    object_with_pressed_button = object_with_pressed_icon_button;
    object_with_pressed_icon_button = 0;
  }


    // unpress the children button if there is an object with pressed one

  if(object_with_pressed_child_button) {
    object_with_pressed_child_button->draw_children_btn();
    object_with_pressed_button = object_with_pressed_child_button;
    object_with_pressed_child_button = 0;
  }
}

	//
	// Callback to process system button popup callbacks
	//

void
G_Object::process_button_event(XButtonEvent *event, G_Window * win)
{
  Dimension ix = event->x;
  Dimension iy = event->y;

    //
    // remove tips tool if exist
    //

  process_motion_event(ix, iy);


    //
    // left mouse button pressed
    // * converts icon to table
    // * allows manipulations with table and edit
    //

  if(event->button == 1) {
    if( ( event->state & Button1Mask ) == 0 ) {
      if(get_complex_state() == G_Object::table) {
        if(test_table_minimize_btn(ix, iy)) {
          draw_table_minimize_btn_pressed();
          object_with_pressed_icon_button = this;
        }
        else if(test_children_btn(ix, iy)) {
          draw_children_btn_pressed();
          object_with_pressed_child_button = this;
        }
        else if(test_table_system_btn(ix, iy)) {
          draw_table_system_btn_pressed();
          create_popup_menu(event, win);
        }
        else {
          size_t count = 0;
          for(std::vector<G_Rectangle *>::iterator i = attributes.begin();i != attributes.end();++i) {
            if((*i)->is_point_inside(ix, iy)) {
              gc->create_edit_widget(this, count, event);
              break;
            }

            count++;
          }
        }
      }
      else if(get_complex_state() != G_Object::table) {
        set_table_state();
        set_invalid();
      }
    }
    else {
      if(get_complex_state() != G_Object::icon && object_with_pressed_button == this) {
        if(test_table_minimize_btn(ix, iy)) {
          gc->destroy_edit_widget();        // @@ TEST @@
          set_icon_state();
          set_invalid();
          clean_motion_timer(gc);
        }
        else if(test_children_btn(ix, iy)) {
          if(get_composite_state() == single)
            show_children();
          else
            hide_children();

          set_invalid();
        }
      }
    }
  }


    //
    // middle mouse button pressed
    // it is used for drag&drop
    //
  
  else if(event->button == 2) {
    if((event->state & Button2Mask) == 0) {

        // draw object selected

      dnd_state = src;
      draw();


        // clean tooltips timer and disable them

      clean_motion_timer(gc);
      G_Context::disable_tooltips();


        // create drag context

      create_drag_context(event);
    }
  }


    //
    // right mouse button pressed
    // it is used:
    //  * to create popup menu on icon
    //  * to show additional attribute info
    //
  
  else if(event->button == 3) {
    if(get_complex_state() == G_Object::icon) {
      create_popup_menu(event, win);
    }
    else {
      size_t count = 0;

      for(std::vector<G_Rectangle *>::iterator i = attributes.begin();i != attributes.end(); ++i) {
        if((*i)->is_point_inside(ix, iy)) {
          show_attr_info(count, event);
          break;
        }

        count++;
      }
    }
  }
}


void
G_Object::toolTipsCB(XtPointer call_data, XtIntervalId*)
{
  G_TipsTool *tt = (G_TipsTool *)call_data;

  timer = 0;

  if(!enable_ttips) return;	// do not show tipstool if there are disabled
  if(show_popup_menu) return;	// do not show tipstool if there is popup menu

  Window wnd, root_window;
  int root_x, root_y, win_x, win_y;
  unsigned int mask;

  XQueryPointer(tt->obj->gc->get_display(), tt->obj->gc->get_xt_window(), &root_window, &wnd,
          &root_x, &root_y, &win_x, &win_y, &mask);

  tt->obj->gc->tips_x    = root_x - win_x + tt->box.x + tt->box.w / 2;
  tt->obj->gc->tips_y    = root_y - win_y + tt->box.y + tt->box.h + 2 /* double line width */;
  tt->obj->gc->tips_text = tt->get_text();

  tt->obj->gc->draw_tips();
}


const char *
G_RelationshipButtonTipsTool::get_text() const
{
  if(obj->get_number_of_children() == 0)
    return (
      (obj->get_propagate() == G_Kid::stop_propagate)
        ? "Relationships are not available"
	: "No Relationships"
    );
  else
    return (
      (obj->get_composite_state() == G_Object::complete)
        ? "Hide Relationships"
	: "Show Relationships"
    );
}


const char *
G_ObjectTipsTool::get_text() const
{
  static std::string text;

  text = "OKS Object: ";
  text += G_Object::to_string(obj->get_oks_object());
  
  return text.c_str();
}


std::string
G_Object::to_string(const OksObject *o)
{
  std::string s = "\"";
  s += o->GetId();
  s += '@';
  s += o->GetClass()->get_name();
  s += '\"';

  return s;
}


int
G_Object::process_motion_event(Dimension ix, Dimension iy)
{
  if(is_point_inside_composite_object(ix, iy)) {
    for(std::list<G_TipsTool *>::iterator i = tips.begin();i != tips.end();++i) {
      G_TipsTool *tt = *i;
      
      if(tt->test_pointer(ix, iy)) {
        if(last_tip != tt) {
          last_tip = tt;
          clean_motion_timer(gc);

          timer = XtAppAddTimeOut(
            XtWidgetToApplicationContext(gc->get_widget()), 1000, G_Object::toolTipsCB, (XtPointer)tt
          );
        }

        return 1;
      }
    }
  }

  return 0;
}


void
G_Object::systemButtonPopupUnmapNotifyAC(Widget, XtPointer obj, XEvent *event, Boolean *)
{
  if(event->type != UnmapNotify) return;
  
  G_Object *o = (G_Object *)obj;
  
  if(o->get_complex_state() == G_Object::table) o->draw_table_system_btn();
  
  if(show_popup_menu == -1) show_popup_menu = 0;
}


void
G_Object::dropPopupUnmapNotifyAC(Widget, XtPointer, XEvent *event, Boolean *)
{
  if(event->type != UnmapNotify) return;
  
  if(show_popup_menu == idRelationshipNone) show_popup_menu = 0;
}


void
G_Object::clean_motion_timer(G_Context * g_context)
{
  if(timer)
    XtRemoveTimeOut(timer);
  
  timer = 0;
  last_tip = 0;
  
  g_context->tips_text = "";
  g_context->draw_tips();
}


DragAndDropTransferResult *
G_Object::get_user_dnd_action(OksObject *src_obj, XButtonEvent * event) const
{

  if(src_obj == obj) {
    std::ostringstream text;
    text << "cannot link object " << obj << " with self";
    report_error("Copy object", text.str().c_str());

    return 0;
  }

    // those variables are used to store values for swap action while
    // popup menu is waiting for user answer

  G_Object   * swap_parent_object = 0;
  G_Children * swap_children = 0;
  
  
    // the objects were already linked
    
  bool wereLinked = false;

    // declare popup menu (later we will see if it is required)

  OksPopupMenu *popup = 0;


  if(get_propagate() == G_Kid::use_propagate && !g_class->p_shown_relationships.empty()) {

      // get destination object class type

    const OksClass *dest_obj_class = obj->GetClass();


      // get source object class and all subclasses types

    const OksClass *src_obj_class = src_obj->GetClass();
    const OksClass::FList * src_obj_superclasses = src_obj_class->all_super_classes();


      // iterate over all object relationships

    unsigned short menu_item_id = idRelationshipSet;


    for(std::list<std::string>::const_iterator i = g_class->p_shown_relationships.begin();i != g_class->p_shown_relationships.end(); ++i, menu_item_id++) {
      const OksRelationship *r = dest_obj_class->find_relationship(*i);

      if(r) {
        const OksClass * r_type = r->get_class_type();

        bool objClassIsValid = (r_type == src_obj_class) ? true : false;

        if(!objClassIsValid && src_obj_superclasses) {
          for(OksClass::FList::const_iterator isc = src_obj_superclasses->begin(); isc != src_obj_superclasses->end(); ++isc)
            if(r_type == *isc) {
	      objClassIsValid = true;
	      break;
	    }
        }

        if(objClassIsValid) {
          std::string item_name;

          OksData *d(obj->GetRelationshipValue(*i));

          if(d->type == OksData::object_type || d->type == OksData::uid_type || d->type == OksData::uid2_type) {
	    if(d->type == OksData::object_type && d->data.OBJECT) {

	        // check if objects were already linked via tested relationship

	      if(src_obj == d->data.OBJECT) {
	        wereLinked = true;
	        continue;
	      }

	      item_name = "Replace " + G_Object::to_string(d->data.OBJECT);
	    }
	    else {
	      item_name = "Set";
	    }

	    item_name += " via relationship \'";
	    item_name += *i;
	    item_name += '\'';
	  }
	  else if(d->type == OksData::list_type) {

	      // check if objects were already linked via tested relationship

            wereLinked = false;

	    for(OksData::List::iterator j = d->data.LIST->begin(); j != d->data.LIST->end(); ++j) {
	      if((*j)->type == OksData::object_type && (*j)->data.OBJECT == src_obj) {
	        wereLinked = true;
		break;
              }
            }

	    if(wereLinked == true) continue;

	    item_name = "Append to relationship \'";
	    item_name += *i;
	    item_name += '\'';
	  }

          if(!popup) {

              // create popup menu

	    popup = new OksPopupMenu(gc->get_widget());


	      // create popup menu title

            std::string s = "Choose link from object " + G_Object::to_string(obj);
            popup->add_label(s.c_str());

	    s = "to object " + G_Object::to_string(src_obj);
            popup->add_label(s.c_str());

            popup->add_separator();
	  }

          if(is_data_file_read_only())
            popup->addDisableItem(item_name.c_str());
	  else
            popup->addItem(item_name.c_str(), menu_item_id, G_Object::dragPopupCB, reinterpret_cast<XtPointer>(menu_item_id));
        }
      }
    }
  }


    // check that both drag and drop objects may belong one parent
    // and referenced by the same relationship

  if(parent) {
    for(std::list<G_Kid *>::iterator i = parent->children.begin(); i != parent->children.end(); ++i) {
      G_Kid *c = *i;

      if(c->get_cardinality() == G_Kid::simple) continue;
      
      G_Children *kids = static_cast<G_Children *>(c);
      
      unsigned int count = 0;
      
      for(std::list<G_Object *>::iterator j = kids->objects.begin(); j != kids->objects.end(); ++j)
        if((*j)->dnd_state != G_Object::none) count++;

      if(count == 2) {
        if(!popup)
	  popup = new OksPopupMenu(gc->get_widget());
	else {
          popup->add_separator();
          popup->add_separator();
	}

#if !defined(__linux__) || (XmVersion > 2000)
        std::string s("Swap object ");
	s += G_Object::to_string(obj);
	s += "\nand object ";
	s += G_Object::to_string(src_obj);
	s += "\ninside relationship \'";
	s += (*i)->rel_name;
	s += '\'';
#else
        std::string s("Swap ");
	s += G_Object::to_string(obj);
	s += " and ";
	s += G_Object::to_string(src_obj);
	s += " in \'";
	s += (*i)->rel_name;
	s += '\'';
#endif

        if(parent->is_data_file_read_only())
          popup->addDisableItem(s.c_str());
	else {
          popup->addItem(s.c_str(), idRelationshipMove, G_Object::dragPopupCB, (XtPointer)idRelationshipMove);

	    // fill transer variables
	  swap_children = kids;
          swap_parent_object = parent;
	}

	break;
      }
    }
  }


    // show popup menu

  if(!popup) {
    if(wereLinked) {
      report_error("Link Objects", "objects were already linked via available relationship(s)");
    }
    else {
      std::ostringstream text;
      text << "cannot find suitable link between chosen objects:\n"
              "1. destination object " << obj;

      if(g_class->p_shown_relationships.empty())
        text << " has no relationships and cannot be linked";
      else
        text << " cannot be linked with source object " << src_obj
             << " via available relationships\n"
                "2. both objects do not belong to the same relationship (i.e. cannot be swapped)";
      report_error("Link Objects", text.str().c_str());
    }

    return 0;
  }

  G_Window::stop_auto_scrolling();

  popup->setDestroyAC(dropPopupUnmapNotifyAC, (XtPointer)this);

  popup->show((XButtonPressedEvent *)event);

  show_popup_menu = idRelationshipNone;

  while(show_popup_menu == idRelationshipNone)
    XtAppProcessEvent(XtWidgetToApplicationContext(gc->get_widget()), XtIMAll);

  delete popup;

  if(!show_popup_menu) return 0;

  if(show_popup_menu == idRelationshipMove) {
    show_popup_menu = 0;
    return new DragAndDropMoveTransferResult(swap_children->rel_name, swap_parent_object);
  }
  else {
    unsigned short menu_number = show_popup_menu - idRelationshipSet;
    show_popup_menu = 0;

    const char * rn = 0;
    for(std::list<std::string>::const_iterator i = g_class->p_shown_relationships.begin(); rn == 0; ++i) {
      if(menu_number-- == 0) rn = (*i).c_str();
    }
    return new DragAndDropLinkTransferResult(rn);
  }
}


	//
	// Callback to process system button popup callbacks
	//

void
G_Object::dragPopupCB(Widget, XtPointer client_data, XtPointer)
{
  show_popup_menu = (long)client_data;
}


struct G_ObjectEditInfo {
  G_Object *	 object;
  size_t	 count;
  XButtonEvent * event;
  int		 format;
};


bool
G_Object::is_url(const char * value)
{
  return (
    value &&
    (
      !strncmp(value, "http:", 5) ||
      !strncmp(value, "file:", 5)
    )
  );
}


void
G_Object::show_attr_info(size_t count, XButtonEvent * event)
{
  OksPopupMenu popup(gc->get_widget());

  popup.setDestroyAC(systemButtonPopupUnmapNotifyAC, (XtPointer)this);
  
  static G_ObjectEditInfo info1;	// edit as decimal (default)
  static G_ObjectEditInfo info2;	// edit as hex

  info1.object = info2.object = this;
  info1.count  = info2.count  = count;
  info1.event  = info2.event  = event;
  info1.format = OksAttribute::Dec;
  info2.format = OksAttribute::Hex;
  
  const std::string& attribute_name = get_attribute_name(count);

  OksData * d = 0;
  try {
    d = obj->GetAttributeValue(attribute_name);
  }
  catch(oks::exception& ex) {
    ers::error(OksDataEditor::InternalProblem(ERS_HERE, ex.what()));
  }

  OksAttribute * a = obj->GetClass()->find_attribute(attribute_name);

  bool is_url = (
    d != 0 &&
    d->type == OksData::string_type &&
    d->data.STRING != 0 &&
    G_Object::is_url(d->data.STRING->c_str())
  );

  XtVaSetValues(
    popup.addItem((a->is_integer() ? "Edit (dec)" : "Edit"), 0, editCB, (XtPointer)&info1),
    XmNfontList, gc->font_list, NULL
  );


  if(a->is_integer()) {
    XtVaSetValues(
      popup.addItem("Edit (hex)", 0, editCB, (XtPointer)&info2),
      XmNfontList, gc->font_list, NULL
    );
  }

  if(is_url) {
    XtVaSetValues(
      popup.addItem("Show URL", 0, urlCB, (XtPointer)d->data.STRING),
      XmNfontList, gc->font_list, NULL
    );
  }

  popup.show((XButtonPressedEvent *)event);
}


void
G_Object::editCB(Widget, XtPointer client_data, XtPointer)
{
  G_ObjectEditInfo * info = (G_ObjectEditInfo *)client_data;
  info->object->gc->create_edit_widget(info->object, info->count, info->event, info->format);
}


void
G_Object::urlCB(Widget, XtPointer client_data, XtPointer)
{
  const OksString * url = (const OksString *)client_data;
  show_url(url);
}


void
G_Object::show_contained_in() const
{
  std::string label("The object ");
  label += to_string(obj);
  label += " is contained in\n";

  if(OksKernel::check_read_only(obj->get_file())) label += "read-only ";

  label += " database file \'";
  label += obj->get_file()->get_full_file_name();
  label += '\'';

  Widget dlg = OksXm::create_dlg_with_pixmap(
    gc->get_widget(), "Contained in",
    ((used && g_class->p_used_image) ? g_class->p_used_image : g_class->p_normal_image),
    label.c_str(),
    "Ok", 0, 0, 0, 0
  );

  XtManageChild(dlg);
}
