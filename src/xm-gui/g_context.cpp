#include <oks/class.h>
#include <oks/object.h>
#include <oks/xm_popup.h>
#include <oks/xm_utils.h>

#include "g_class.h"
#include "g_context.h"
#include "g_object.h"

#include "data_editor.h"
#include "data_editor_exceptions.h"

#include <Xm/Label.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/Screen.h>
#include <Xm/TextF.h>

#include "one.xpm"
#include "many.xpm"
#include "aggregation.xpm"


G_Context::G_Context() :
font_list        (0),
tips_widget	 (0),
edit_g_obj	 (0),
edit_attr_count	 (0),
attr_edit_widget (0)
{;}


G_Context::~G_Context()
{

}

void
G_Context::initialize()
{
    // Create tips_pixel

  tips_pixel = get_color("#FFFFC0"); // light yellow

    // Create pixmaps

  one_pxm = G_Class::create_pixmap(one_xpm, one_pxm_w, one_pxm_h);
  many_pxm = G_Class::create_pixmap(many_xpm, many_pxm_w, many_pxm_h);
  aggregation_pxm = G_Class::create_pixmap(aggregation_xpm, aggregation_pxm_w, aggregation_pxm_h);
}

int
G_Context::draw_tips()
{
  if(tips_widget) {
    XtDestroyWidget(tips_widget);
    tips_widget = 0;
  }

  if(tips_text.empty()) return 0;

  Arg temp_args[10];
  int i;

  i = 0;
  XtSetArg(temp_args[i], XmNallowShellResize, True); i++;
  XtSetArg(temp_args[i], XmNborderWidth, 1); i++;
  XtSetArg(temp_args[i], XmNx, tips_x); i++;
  XtSetArg(temp_args[i], XmNy, tips_y); i++;
  XtSetArg(temp_args[i], XmNbackground, tips_pixel); i++;

  tips_widget = XtCreatePopupShell("Notifier", overrideShellWidgetClass, get_widget(), temp_args, i);

  i = 0;
  XtSetArg(temp_args[i], XmNalignment, XmALIGNMENT_BEGINNING); i++;
  XtSetArg(temp_args[i], XmNbackground, tips_pixel); i++; 

  XtCreateManagedWidget((char *)tips_text.c_str(),
    xmLabelWidgetClass,
    tips_widget,
    temp_args, i);

  XtPopdown(tips_widget);
  XtPopup(tips_widget, XtGrabNone);

  return 0;
}


	//
	// Destroy previous widget if it exists
	//

void
G_Context::destroy_edit_widget()
{
  if(attr_edit_widget) {

      // if the widget is text field, then read new value

    if(XmIsTextField(attr_edit_widget) && edit_g_obj->is_data_file_read_only() == false) {

       // get new value from text field (it can be multi-value)

      const std::string& attr_name = edit_g_obj->get_attribute_name(edit_attr_count);


        // get new value from text field (it can be multi-value)

      OksXm::AutoCstring new_value2(XmTextFieldGetString(attr_edit_widget));

        // take old value from OKS

      std::string old_value = edit_g_obj->get_attribute_value(attr_name);

        // set new value, only if there are changes

      if(old_value != new_value2.get()) {
        OksAttribute *a = edit_g_obj->get_oks_object()->GetClass()->find_attribute(attr_name);
        char * new_value = strip_brackets(a, new_value2.get());
        edit_g_obj->set_attribute_value(attr_name, new_value);
      }
    }

      // destroy the widget
#if defined(__linux__) && (XmVersion < 2000) // our Lesstif on linux has bug
    XtUnmanageChild(attr_edit_widget);
#else
    XtDestroyWidget(attr_edit_widget);
#endif

    attr_edit_widget = 0;
  }
}


void
G_Context::create_edit_widget(G_Object *g_obj, size_t count, XButtonEvent *event, int format)
{
  destroy_edit_widget();

	//
	// Save edited OKS object and attribute name
	//

  edit_g_obj = g_obj;
  edit_attr_count = count;


	//
	// Read attribute name, rectangle & value
	//

  const std::string& attr_name = edit_g_obj->get_attribute_name(edit_attr_count);
  const G_Rectangle& rect = edit_g_obj->get_attribute_rect(edit_attr_count);
  OksAttribute *a = edit_g_obj->get_oks_object()->GetClass()->find_attribute(attr_name);
  const std::string value(g_obj->get_attribute_value(attr_name, format));

  OksData::Type	type = a->get_data_type();
  
  bool is_obj_read_only = g_obj->is_data_file_read_only();

  Arg temp_args[16];
  int i;

  if(font_list) {
    XmFontListFree(font_list);
    font_list = 0;
  }

  font_list = XmFontListCreate(get_font(), XmSTRING_DEFAULT_CHARSET);


    // create popup menu for enumeration or boolean types

  if(type == OksData::bool_type || type == OksData::enum_type) {
    size_t num_of_tokens = 1;
    
    if(type == OksData::enum_type) {

	  // get number of tokens

      for(const char *p = a->get_range().c_str(); *p; ++p)
        if(*p == ',') num_of_tokens++;

	  // nothing to do if there is only one token!

      if(num_of_tokens == 1) {
	return;
      }
    }

    OksPopupMenu popup(get_widget());
    popup.setDestroyAC(unselectAC, (XtPointer)this);
    
    std::list<Widget> menu_items;
     
    if(type == OksData::bool_type) {
      if(a->get_is_multi_values()) {
        ers::error(OksDataEditor::InternalProblem(ERS_HERE, "Multi-value boolean is not supported\nThink better about it's replacement by some enumeration"));
      }
      else {
        const char * set_value = "false";
        int set_id = 0;

        if(value == "false") {
          set_value = "true";
	  set_id = 1;
        }

        menu_items.push_back(popup.add_label("Set \'boolean\' value"));

	XtVaSetValues(
	  popup.add_separator(),
	  XmNseparatorType, XmSINGLE_DASHED_LINE,
	  XmNmargin, 2,
	  NULL
	);

        menu_items.push_back(
	  is_obj_read_only
	    ? popup.addDisableItem(set_value)
	    : popup.addItem(set_value, set_id, singleBoolCB, (XtPointer)this)
	);
      }
    }
    else {
      int label_printed = 0;

      if(a->get_is_multi_values()) {
        OksData::List * dlist = 0;


	  // get current attribute values

	try {
	  OksData *d(edit_g_obj->get_oks_object()->GetAttributeValue(a->get_name()));
	  if(d->data.LIST && !d->data.LIST->empty()) { dlist = d->data.LIST; }
	}
	catch(oks::exception& ex) {
          ers::error(OksDataEditor::InternalProblem(ERS_HERE, ex.what()));
	}
	
	
	  // create arrays which will keep info about add and remove values

        std::string** add_tokens = new std::string * [num_of_tokens];
        std::string** remove_tokens = new std::string * [num_of_tokens];

	size_t add_ptr = 0;
	size_t remove_ptr = 0;

        {
	  for(size_t j=0; j<num_of_tokens; ++j) {
	    add_tokens[j] = remove_tokens[j] = 0;
	  }
	}


          // create iterator

        Oks::Tokenizer t(a->get_range(), " ,");
        std::string token;

        while(!(token = t.next()).empty()) {
	  int found = 0;

	  if(dlist) {
	    for(OksData::List::iterator it = dlist->begin(); it != dlist->end(); ++it) {
	      if(token == (*it)->data.ENUMERATION->c_str()) {
	        found =1;
		break;
	      }
	    }
	  }

	  if(found)
	    remove_tokens[remove_ptr++] = new std::string(token);
          else
	    add_tokens[add_ptr++] = new std::string(token);
        }


	if(add_ptr) {
	  std::string label_str("Add \'enum\' value");
	  if(add_ptr > 1) label_str += 's';

	  menu_items.push_back(popup.add_label(label_str.c_str()));

	  XtVaSetValues(
	    popup.add_separator(),
	    XmNseparatorType, XmSINGLE_DASHED_LINE,
	    XmNmargin, 2,
	    NULL
	  );

	  for(size_t j=0; j<add_ptr; ++j) {
	    menu_items.push_back(
	      is_obj_read_only
	        ? popup.addDisableItem(add_tokens[j]->c_str())
		: popup.addItem(add_tokens[j]->c_str(), 1, multiTokenCB, (XtPointer)this)
	    );

	    delete add_tokens[j];
	  }

	  if(remove_ptr) {
	    popup.add_separator();
	  }
	}

	if(remove_ptr) {
	  std::string label_str("Remove \'enum\' value");
	  if(remove_ptr > 1) label_str += 's';

	  menu_items.push_back(popup.add_label(label_str.c_str()));

	  XtVaSetValues(
	    popup.add_separator(),
	    XmNseparatorType, XmSINGLE_DASHED_LINE,
	    XmNmargin, 2,
	    NULL
	  );

	  for(size_t j=0; j<remove_ptr; ++j) {
	    menu_items.push_back(
	      is_obj_read_only
	        ? popup.addDisableItem(remove_tokens[j]->c_str())
		: popup.addItem(remove_tokens[j]->c_str(), 0, multiTokenCB, (XtPointer)this)
	    );
	    delete remove_tokens[j];
	  }
	}

	delete [] add_tokens;
	delete [] remove_tokens;
      }
      else {
        Oks::Tokenizer t(a->get_range(), " ,");
        std::string token;

        while(!(token = t.next()).empty()) {
          if(token != value) {
	    if(!label_printed) {
	      menu_items.push_back(popup.add_label("Set \'enum\' value"));
              popup.add_separator();
	      label_printed = 1;
	    }

	    menu_items.push_back(popup.addItem(token.c_str(), 0, singleTokenCB, (XtPointer)this));
          }
        }
      }
    }

    selected_attr.set(
      rect.get_x() + shadow_thickness,
      rect.get_y() + shadow_thickness,
      rect.get_width() - 3 * shadow_thickness,
      rect.get_height() - 3 * shadow_thickness
    );

    draw_shadowed_rectangle(
      selected_attr.get_x(),
      selected_attr.get_y(),
      selected_attr.get_width(),
      selected_attr.get_height()
    );

    while(!menu_items.empty()) {
      XtVaSetValues(menu_items.front(), XmNfontList, font_list, NULL);
      menu_items.pop_front();
    }

    popup.show(event);
  }


    // create edit text field widget for non-enumeration and non-boolean types

  else {
    i = 0;
    XtSetArg(temp_args[i], XmNfontList, font_list); i++;
    XtSetArg(temp_args[i], XmNx, rect.x); i++;
    XtSetArg(temp_args[i], XmNy, rect.y); i++;
    XtSetArg(temp_args[i], XmNheight, rect.h); i++;
    XtSetArg(temp_args[i], XmNwidth, rect.w); i++;
    XtSetArg(temp_args[i], XmNvalue, value.c_str()); i++;
    XtSetArg(temp_args[i], XmNshadowThickness, shadow_thickness); i++;
    XtSetArg(temp_args[i], XmNhighlightThickness, highlight_thickness); i++;
    XtSetArg(temp_args[i], XmNmarginWidth, get_params().get_font_x_margin()); i++;
    XtSetArg(temp_args[i], XmNmarginHeight, get_params().get_font_y_margin()); i++;

    attr_edit_widget = XtCreateManagedWidget(value.c_str(),
      xmTextFieldWidgetClass,
      get_widget(),
      temp_args, i
    );

    if(is_obj_read_only) {
      OksXm::TextField::set_not_editable(attr_edit_widget);
      XtSetSensitive(attr_edit_widget, false);
    }

    XtAddCallback(
      attr_edit_widget,
      XmNlosingFocusCallback,
      G_Context::editControlLosingFocusCB,
      (XtPointer)this
    );
  }
}


void
G_Context::editControlLosingFocusCB(Widget, XtPointer client_data, XtPointer)
{
  ((G_Context *)client_data)->destroy_edit_widget();
}


void
G_Context::multiTokenCB(Widget w, XtPointer client_data, XtPointer)
{
  G_Context *lthis = ((G_Context *)client_data);
  G_Object *g_obj = lthis->edit_g_obj;

  const std::string& attr_name = g_obj->get_attribute_name(lthis->edit_attr_count);

  XmString xms = 0;
  
  XtVaGetValues(w, XmNlabelString, &xms, NULL);
  
  OksXm::AutoCstring value(OksXm::string_to_c_str(xms));


  OksData * d;

  try {
    d = g_obj->get_oks_object()->GetAttributeValue(attr_name);
  }
  catch(oks::exception& ex) {
    ers::error(OksDataEditor::InternalProblem(ERS_HERE, ex.what()));
    return;
  }


    // add token

  if(OksXm::get_user_data(w)) {
    if(!d->data.LIST) d->data.LIST = new OksData::List();
  
    OksData *d2 = new OksData(value.get());
    d2->type = OksData::enum_type;

    d->data.LIST->push_back(d2);
  }


    // remove token

  else {
    for(OksData::List::iterator i = d->data.LIST->begin(); i != d->data.LIST->end(); ++i) {
      if(!strcmp((*i)->data.STRING->c_str(), value.get())) {
        OksData * td = *i;
	d->data.LIST->erase(i);
	delete td;
        break;
      }
    }
  }

  try {
    g_obj->get_oks_object()->SetAttributeValue(attr_name, d);
  }
  catch(oks::exception& ex) {
    ers::error(OksDataEditor::InternalProblem(ERS_HERE, ex.what()));
  }

  lthis->selected_attr.set(0, 0, 0, 0);
}


void
G_Context::singleTokenCB(Widget w, XtPointer client_data, XtPointer)
{
  G_Context *lthis = ((G_Context *)client_data);
  G_Object *g_obj = lthis->edit_g_obj;

  const std::string& attr_name = g_obj->get_attribute_name(lthis->edit_attr_count);
  const OksAttribute * a = g_obj->get_oks_object()->GetClass()->find_attribute(attr_name);

  XmString xms = 0;

  XtVaGetValues(w, XmNlabelString, &xms, NULL);

  OksXm::AutoCstring value(OksXm::string_to_c_str(xms));

  try {
    OksData d;
    d.data.ENUMERATION = a->get_enum_value(value.get(), strlen(value.get())); //new OksString(value.get());
    d.type = OksData::enum_type;
    g_obj->get_oks_object()->SetAttributeValue(attr_name, &d);
  }
  catch(oks::exception& ex) {
    ers::error(OksDataEditor::InternalProblem(ERS_HERE, ex.what()));
  }

  lthis->selected_attr.set(0, 0, 0, 0);
}

void
G_Context::multiBoolCB(Widget w, XtPointer client_data, XtPointer)
{
  G_Context *lthis = ((G_Context *)client_data);
  G_Object *g_obj = lthis->edit_g_obj;

  const std::string& attr_name = g_obj->get_attribute_name(lthis->edit_attr_count);

  try {
    OksData *d(g_obj->get_oks_object()->GetAttributeValue(attr_name));
    d->data.BOOL = (bool)OksXm::get_user_data(w);
    g_obj->get_oks_object()->SetAttributeValue(attr_name, d);
  }
  catch(oks::exception& ex) {
    ers::error(OksDataEditor::InternalProblem(ERS_HERE, ex.what()));
  }
}

void
G_Context::singleBoolCB(Widget w, XtPointer client_data, XtPointer)
{
  G_Context *lthis = ((G_Context *)client_data);
  G_Object *g_obj = lthis->edit_g_obj;

  const std::string& attr_name = g_obj->get_attribute_name(lthis->edit_attr_count);

  try {
    OksData *d(g_obj->get_oks_object()->GetAttributeValue(attr_name));
    d->data.BOOL = (bool)OksXm::get_user_data(w);
    g_obj->get_oks_object()->SetAttributeValue(attr_name, d);
  }
  catch(oks::exception& ex) {
    ers::error(OksDataEditor::InternalProblem(ERS_HERE, ex.what()));
  }
}


void
G_Context::unselectAC(Widget, XtPointer user_data, XEvent *event, Boolean *)
{
  if(event->type != UnmapNotify) return;

  G_Context *lthis = (G_Context *)user_data;

  if(lthis->selected_attr.get_width()) {
    Dimension x1 = lthis->selected_attr.get_x();
    Dimension x2 = x1 + lthis->selected_attr.get_width();
    Dimension y1 = lthis->selected_attr.get_y();
    Dimension y2 = y1 + lthis->selected_attr.get_height();

    XGCValues values;
    XGetGCValues(lthis->get_display(), lthis->get_gc(), GCForeground, &values);

    unsigned long fg = values.foreground;

    XtVaGetValues(XtParent(lthis->get_widget()), XmNbackground, &values.foreground, NULL);

    XChangeGC(lthis->get_display(), lthis->get_gc(), GCForeground, &values);

    lthis->draw_line(x1, y1, x2, y1);
    lthis->draw_line(x1, y1, x1, y2);
    lthis->draw_line(x1, y2, x2, y2);
    lthis->draw_line(x2, y1, x2, y2);

    values.foreground = fg;
    XChangeGC(lthis->get_display(), lthis->get_gc(), GCForeground, &values);

    lthis->selected_attr.set(0, 0, 0, 0);
  }
}
