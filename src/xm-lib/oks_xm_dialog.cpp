#define _OksXmBuildDll_

#include <stdlib.h>

#include <fstream>
#include <string>

#include <oks/defs.h>
#include <oks/xm_dialog.h>
#include <oks/xm_utils.h>

#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <Xm/Label.h>
#include <Xm/TextF.h>
#include <Xm/Text.h>
#include <Xm/List.h>
#include <Xm/Matrix.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/SeparatoG.h>
#include <Xm/SelectioB.h>
#include <Xm/ScrolledW.h>
#include <Xm/Frame.h>
#include <Xm/PanedW.h>
#include <Xm/ToggleB.h>
#include <Xm/ComboBox.h>


const char * OksDlgEntity::noTitleAndScrollBars = "fntasb";


	// offset of last XmForm item from bottom edge, and
	// offset of first XmForm item from top edge
const Dimension form_y_offset = 5;
const Dimension form_x_offset = 5;

	// XmFrame parameters
const Dimension frame_shadow_thickness = 2;
const Dimension frame_margin_width = 2;
const Dimension frame_margin_height = 2;

void
setMarginHeight(Widget w_dest, Widget w_src)
{
  Dimension destMarginHeight, destHeight, srcHeight;

  XtVaGetValues(w_dest, XmNheight, &destHeight, XmNmarginHeight, &destMarginHeight, NULL);
  XtVaGetValues(w_src, XmNheight, &srcHeight, NULL);

  Dimension newDestMarginHeight = (srcHeight - destHeight + 2*destMarginHeight)/2;

  XtVaSetValues(w_dest, XmNmarginHeight, newDestMarginHeight, NULL);
}


	//
	// The function is used to calculate height of a widget
	// It was introduced because of problems with RowColumn widget
	//

static Dimension
getWidgetHeight(Widget w)
{
  Dimension h = 0;

    // get height by normal way

  XtVaGetValues(w, XmNheight, &h, NULL);


    // return if the widget is not RowColumn

  if(!XmIsRowColumn(w)) return h;


    //
    // height of RowColumn in OptionMenu state is a contant ( 16 ?! )
    // that is less than height of it's option lable or option button
    //

  Widget w2 = XmOptionLabelGadget(w);
  if(!w2) return h;

  Dimension h2;
  XtVaGetValues(w2, XmNheight, &h2, NULL);

  h = ((h2 > h) ? h2 : h);
  
  w2 = XmOptionButtonGadget(w);
  if(!w2) return h;

  XtVaGetValues(w2, XmNheight, &h2, NULL);

  return ((h2 > h) ? h2 : h);
}


Widget
OksDlgEntity::get_parent_widget() const
{
  return ( (type == List || type == Text) ? XtParent(d.w) : d.w );
}


OksForm::OksForm()
{
  form = 0;
}


OksForm::OksForm(Widget parent)
{
  Arg args[5];

  XtSetArg(args[0], XmNautoUnmanage, false);

  form = XmCreateForm(parent, (char *)"form", args, 1);

  OksXm::add_mouse_wheel_action(form);
  OksXm::add_mouse_wheel_action(XtParent(form));
}


OksForm::~OksForm()
{
  while(entities.size()) {
    OksDlgEntity *e = entities.front();
    entities.pop_front();

    if(e->type == OksDlgEntity::Form) delete e->d.f;
    else if(e->type == OksDlgEntity::PanedWindow) delete e->d.p;

    delete e;
  }

  // the valgrind reports problem since the forms (including nested)
  // are destroyed by parent shell dialog => comment out below

/*  if(form && XtIsManaged(form)) {
    XtUnmanageChild(form);
    XtDestroyWidget(form);
    form = 0;
  }*/
}

void
OksForm::show() const
{
  XtManageChild(form);
}

size_t
get_max_width(const std::list<const char *>& items, bool ignore_last)
{
  size_t x = 6; // min width

  for(std::list<const char *>::const_iterator i = items.begin(); i != items.end(); ++i) {
    if(ignore_last && (*i) == items.back()) continue;
    size_t len = strlen(*i);
    if(len > x) x = len;
  }

  return x;
}


Widget
OksForm::add(OksDlgEntity::Type type, short id, void *data)
{
  Widget	w = 0;
  OksForm	*f;

  XmString	xm_string = 0;
  Arg		args[20];
  int		n = 0;
  size_t	number_of_items = entities.size();

  OksDlgEntity * entity = new OksDlgEntity();

  bool itemHasTitle =
    ((data != OksDlgEntity::noTitleAndScrollBars) ? true : false);

    // create widget's title
  {
    const char * s(0);

    if( type != OksDlgEntity::OptionMenu && type != OksDlgEntity::PanedWindow && itemHasTitle) {
      s = reinterpret_cast<const char *>(data);
    }

    if(type == OksDlgEntity::ComboBox) {
      s = (reinterpret_cast<std::list<const char *> *>(data))->back();
    }

    if(s) xm_string = OksXm::create_string(s);
  }


  if(number_of_items) {

      //
      //  +--------+......topAttachment.....+--------+
      //  | e->d.w | .                      |  last  |
      //  +--------+ .                      +--------+
      //             .
      //       +--------+
      //       |   new  |
      //       +--------+ 
      //
      //  @@ Igor Soloviev 30/11/98 @@
      //  BUG: topAttachment from new widget to last
      //       does not work with LessTif if last
      //	 widget top's attachment uses OPPOSITE_WIDGET
      //

    OksDlgEntity *e = entities.back();


      // calculate height of the row of widgets

    unsigned row_height = getWidgetHeight(e->d.w);


      // this is the pointer to left widget from row

    Widget left_w = e->d.w;


      // scan widgets till left one will be found

    {
      Widget lw = e->d.w;

      while(lw) {
        unsigned char top_attachment = 0;
        unsigned char left_attachment = 0;

        XtVaGetValues(lw,
          XmNtopAttachment, &top_attachment,
          XmNleftAttachment, &left_attachment,
          XmNleftWidget, &w,
          NULL
        );

        if(left_attachment == XmATTACH_FORM || lw == w) break;

        lw = w;

        if(lw && top_attachment == XmATTACH_OPPOSITE_WIDGET) {
          Dimension h = getWidgetHeight(lw);
          if(h > row_height) row_height = h;
          left_w = lw;
        }
      }
    }

    if(e->type == OksDlgEntity::TextField) {
      std::list<OksDlgEntity*>::reverse_iterator i = entities.rbegin();
      ++i; ++i; // access second element from end
      e = *i;
    }


      // calculate new top attachment (3 + offset)

    unsigned long top_offset = (row_height - getWidgetHeight(left_w)) + 3;

      // store information about top attachment in the widget's userData
      // (value of XmNtopOffset can be overwritten if user attach widget by previous)

    XtSetArg(args[n], XmNuserData, (XtPointer)top_offset);  n++;


      // take into account that left widget is child of scrooled list of text

    if(XmIsText(left_w) || XmIsList(left_w)) left_w = XtParent(left_w);

    XtSetArg(args[n], XmNtopOffset, top_offset);  n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET);  n++;
    XtSetArg(args[n], XmNtopWidget, left_w);  n++;
  }
  else {
    XtSetArg(args[n], XmNtopOffset, ((type != OksDlgEntity::Menu) ? form_y_offset : 0));  n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM);  n++;
  }

  if(type == OksDlgEntity::Separator || type == OksDlgEntity::Menu) {
    XtSetArg(args[n], XmNleftOffset, ((type != OksDlgEntity::Menu) ? 1 : 0));  n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);  n++;
    XtSetArg(args[n], XmNrightOffset, ((type != OksDlgEntity::Menu) ? 1 : 0));  n++;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM);  n++;

    w = ((type == OksDlgEntity::Separator) ?
      XmCreateSeparatorGadget (form, (char *)"separator", args, n) :
      XmCreateMenuBar (form, (char *)"menu_bar", args, n)
    );
  }
  else if(type == OksDlgEntity::OptionMenu) {
    Widget pulldown = XmCreatePulldownMenu(form, (char *)"pulldown_menu", 0, 0);

    std::list<const char *> * menu_items(reinterpret_cast<std::list<const char *> *>(data));
    const char * last_button(menu_items->back());

    for(std::list<const char *>::iterator i = menu_items->begin(); i != menu_items->end(); ++i) {
      const char * buttonName = *i;
      OksDlgEntity * entity2;

      xm_string = OksXm::create_string(buttonName);	

      if(buttonName != last_button) {
        entity2 = new OksDlgEntity();

        w = XmCreatePushButtonGadget(pulldown, (char *)"option_menu_item", 0, 0);

        XtVaSetValues(w, XmNlabelString, xm_string, NULL);

        entity2->id = 0;
      }
      else {
        entity2 = entity;

        XtSetArg(args[n], XmNleftOffset, 3);  n++;
        XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);  n++;
        XtSetArg(args[n], XmNlabelString, xm_string);  n++;
        XtSetArg(args[n], XmNsubMenuId, pulldown);  n++;

        w = XmCreateOptionMenu(form, (char *)"simple", args, n);

        entity2->id = id;
      }

      XtManageChild(w);

      entity2->d.w  = w;
      entity2->data = (void *)buttonName;
      entity2->type = type;

      entities.push_back(entity2);

      XmStringFree(xm_string);
    }

      // adjust number of columns, if there are too many items in the menu for single one
    {
      static Dimension screen_h = HeightOfScreen(XtScreen(pulldown));

      Dimension height;
      XtVaGetValues(pulldown, XmNheight, &height, NULL);

      short num = height / screen_h;

      if(num > 0) {
        num++;
        XtVaSetValues(pulldown, XmNpacking, XmPACK_COLUMN, XmNnumColumns, num, NULL);
      }
    }

    return w;
  }
  else {
    if(xm_string) {
      XtSetArg(args[n], XmNlabelString, xm_string); n++;
    }

    XtSetArg(args[n], XmNleftOffset, type != OksDlgEntity::PanedWindow ? form_x_offset : 1);  n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);  n++;

    if(type == OksDlgEntity::PushButton)
      XtManageChild (w = XmCreatePushButton(form, (char *)"button", args, n));
    else if(type == OksDlgEntity::ToggleButton)
      XtManageChild (w = XmCreateToggleButton(form, (char *)"toggle", args, n));
    else {
      if(type != OksDlgEntity::Menu && type != OksDlgEntity::PanedWindow && xm_string) {
        const char *resource_name = "simple";

        if(type == OksDlgEntity::Label) {
          if(id == OksDlgEntity::idPixmap)
            resource_name = "popup";
          else
            resource_name = "header";
        }
        else if(type == OksDlgEntity::Form)
          resource_name = "header";

        w = XmCreateLabel(form, (char *)resource_name, args, n);
        XtManageChild(w);

	OksXm::add_mouse_wheel_action(XtParent(w)); // forward to form

        OksDlgEntity* entity2 = new OksDlgEntity();

        entity2->d.w	= w;
        entity2->data	= 0;
        entity2->type	= OksDlgEntity::Label;
        entity2->id	= OksDlgEntity::idLabel;

        entities.push_back(entity2);
      }

      if(
       type != OksDlgEntity::PanedWindow &&
       (type != OksDlgEntity::Form || itemHasTitle)
      )
        n = 0;

      if(type == OksDlgEntity::TextField) {
        XtSetArg(args[n], XmNtopOffset, 0);  n++;
        XtSetArg(args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET);  n++;
        XtSetArg(args[n], XmNtopWidget, w);  n++;

        Dimension left_offset = (xm_string && ((const char *)data)[0]) ? 10 : 0;

        XtSetArg(args[n], XmNleftOffset, left_offset);  n++;
        XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET);  n++;
        XtSetArg(args[n], XmNleftWidget, w);  n++;

        XtManageChild (w = XmCreateTextField(form, (char *)"field", args, n));

        setMarginHeight((entities.back())->d.w, w);

        OksXm::add_mouse_wheel_action(w); // forward to form
      }
      else if(type == OksDlgEntity::ComboBox) {
        XtSetArg(args[n], XmNtopOffset, 0);  n++;
        XtSetArg(args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET);  n++;
        XtSetArg(args[n], XmNtopWidget, w);  n++;

        XtSetArg(args[n], XmNleftOffset, 10);  n++;
        XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET);  n++;
        XtSetArg(args[n], XmNleftWidget, w);  n++;

        std::list<const char *> * menu_items(reinterpret_cast<std::list<const char *> *>(data));
        unsigned int menu_size(menu_items->size()-1);

        XmStringTable str_list = (XmStringTable) XtMalloc (menu_size * sizeof (XmString));

        unsigned int count = 0;
        for(std::list<const char *>::iterator i = menu_items->begin(); count < menu_size; ++i) {
          str_list[count++] = XmStringCreateLocalized((char *)(*i));
        }

        XtSetArg (args[n], XmNitems, str_list);  n++;
        XtSetArg (args[n], XmNitemCount, menu_size); n++;

        short tf_w = (short) get_max_width(*menu_items, true);
        XtSetArg (args[n], XmNcolumns, tf_w); n++;

        w = XmCreateDropDownList (form, (char *)"combo", args, n);
  
        for (count = 0; count < menu_size; count++) XmStringFree (str_list[count]);
        XtFree ((char *) str_list);

        XtManageChild(w);

        setMarginHeight((entities.back())->d.w, w);

        OksXm::add_mouse_wheel_action(w); // forward to form
      }
      else if(type == OksDlgEntity::PanedWindow) {
        XtSetArg(args[n], XmNmarginHeight, 1);  n++;
        XtSetArg(args[n], XmNmarginWidth, 1);  n++;
        XtManageChild (w = XmCreatePanedWindow(form, (char *)"paned_window", args, n));
        
        OksDlgEntity* entity2 = new OksDlgEntity();

        entity2->d.p	= new OksPanedWindow();
        entity2->data	= data;
        entity2->type	= OksDlgEntity::PanedWindow;
        entity2->id	= id;

        entities.push_back(entity2);

        type = OksDlgEntity::PanedWindowWidget;
      }
      else if(type == OksDlgEntity::Text) {
        XtSetArg(args[n], XmNtopOffset, 2);  n++;
        XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET);  n++;
        XtSetArg(args[n], XmNtopWidget, w);  n++;

        XtSetArg(args[n], XmNleftOffset, form_x_offset);  n++;
        XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);  n++;

        XtSetArg(args[n], XmNrows, 2); n++;
        XtSetArg(args[n], XmNeditMode, XmMULTI_LINE_EDIT); n++;

        w = XmCreateScrolledText(form, (char *)"text", args, n);

        OksXm::add_mouse_wheel_action(w);
      }
      else if(type == OksDlgEntity::List) {
        XtSetArg(args[n], XmNtopOffset, 2);  n++;
        XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET);  n++;
        XtSetArg(args[n], XmNtopWidget, w);  n++;

        XtSetArg(args[n], XmNleftOffset, form_x_offset);  n++;
        XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);  n++;

        XtSetArg(args[n], XmNvisibleItemCount,  3); n++;
        XtSetArg(args[n], XmNscrollVertical, True); n++;

        XtSetArg(args[n], XmNlistSizePolicy, XmRESIZE_IF_POSSIBLE); n++;
        XtSetArg(args[n], XmNselectionPolicy, XmBROWSE_SELECT); n++;

        w = XmCreateScrolledList(form, (char *)"list", args, n);

        OksXm::add_mouse_wheel_action(w);
      }
      else if(type == OksDlgEntity::Matrix) {
        XtSetArg(args[n], XmNtopOffset, 2);  n++;
        XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET);  n++;
        XtSetArg(args[n], XmNtopWidget, w);  n++;
	
        XtSetArg(args[n], XmNleftOffset, form_x_offset);  n++;
        XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);  n++;

//        XtSetArg(args[n], (char *)XmNfill, True);  n++;
        XtSetArg(args[n], (char *)XmNtraverseFixedCells, True);  n++;
        XtSetArg(args[n], (char *)XmNallowColumnResize, True);  n++;
        XtSetArg(args[n], (char *)XmNboldLabels, True);  n++;
        XtSetArg(args[n], (char *)XmNbuttonLabels, True);  n++;
        XtSetArg(args[n], (char *)XmNcellShadowThickness, 1);  n++;
        XtSetArg(args[n], (char *)XmNcellShadowType, XmSHADOW_IN);  n++;
        XtSetArg(args[n], (char *)XmNgridType, XmGRID_SHADOW_OUT);  n++;

        w = XtCreateManagedWidget((char *)"XbaeMatrix", xbaeMatrixWidgetClass, form, args, n);
      }
      else if(type == OksDlgEntity::Form) {
        if(itemHasTitle) {
        	XtSetArg(args[n], XmNtopOffset, 2);  n++;
        	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET);  n++;
        	XtSetArg(args[n], XmNtopWidget, w);  n++;

        	XtSetArg(args[n], XmNleftOffset, form_x_offset);  n++;
        	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);  n++;
        }

        XtSetArg(args[n], XmNshadowType, XmSHADOW_IN);  n++;
        XtSetArg(args[n], XmNshadowThickness, frame_shadow_thickness);  n++;
        XtSetArg(args[n], XmNmarginWidth, frame_margin_width);  n++;
        XtSetArg(args[n], XmNmarginHeight, frame_margin_height);  n++;

        Widget frameWindow = XmCreateFrame(form, (char *)"frame", args, n);
        XtManageChild(frameWindow);

        if(itemHasTitle) {
          n = 0;
          XtSetArg(args[n], XmNscrollingPolicy, XmAUTOMATIC);  n++;
          XtSetArg(args[n], XmNheight, 400);  n++;

          Widget scrolledWindow = XmCreateScrolledWindow(frameWindow, (char *)"scrolled window", args, n);
          XtManageChild(scrolledWindow);

          f = new OksForm(scrolledWindow);
        }
        else
          f = new OksForm(frameWindow);

        f->show();

        OksDlgEntity* entity2 = new OksDlgEntity();

        entity2->d.f	= f;
        entity2->data	= data;
        entity2->type	= type;
        entity2->id	= id;

        entities.push_back(entity2);

        type = OksDlgEntity::ScrolledWindow;
        w = frameWindow;
      }
    }

    if(xm_string) XmStringFree(xm_string);
  }
  
  if(type != OksDlgEntity::Menu || type != OksDlgEntity::TextField || type != OksDlgEntity::ComboBox) XtManageChild (w);

  entity->d.w	= w;
  entity->data	= data;
  entity->id	= id;
  entity->type	= type;

  entities.push_back(entity);

  return w;
}

void
OksForm::change_combo_box(short id, std::list<const char *>& items)
{
  Widget w = get_widget(id);

  unsigned int menu_size(items.size());

  XmStringTable str_list = (XmStringTable) XtMalloc (menu_size * sizeof (XmString));

  unsigned int count = 0;
  for(std::list<const char *>::iterator i = items.begin(); count < menu_size; ++i) {
    str_list[count++] = OksXm::create_string(*i);
  }

    // resize combo box (set text field width)

  short old_w(0);
  XtVaGetValues(w, XmNcolumns, &old_w, NULL);

  size_t new_w = get_max_width(items, false);

  if((short)new_w != old_w) {
    old_w = (short)new_w;
    XtVaSetValues(w, XmNcolumns, old_w, NULL);
  }

  XtVaSetValues(w, XmNitems, str_list, XmNitemCount, menu_size, NULL);


    // free temporal resources

  for (count = 0; count < menu_size; count++) XmStringFree (str_list[count]);
  XtFree ((char *) str_list);
}


Widget
OksForm::get_previous(short id) const
{
  Widget w = 0;
  
  std::list<OksDlgEntity*>::const_iterator i;
  
  for(i = entities.begin(); i != entities.end(); ++i) {
    OksDlgEntity *e = *i;

    if(!e->id || e->type == OksDlgEntity::Form) continue;
	
    if( id == e->id ) break;
    w = e->d.w;
  }
  
  return w;
}


void
OksForm::attach_previous(short id, int user_dx, bool set_mx) const
{
  Widget	w = 0;
  Widget	w2 = 0;
  OksDlgEntity	*e2 = 0;
  OksDlgEntity	*e = 0;

  std::list<OksDlgEntity*>::const_iterator i;

  for(i = entities.begin(); i != entities.end(); ++i) {
    e = *i;

    if(
     !e->id ||
     e->type == OksDlgEntity::Form ||
     e->type == OksDlgEntity::PanedWindow
    ) continue;
	
    if( id == e->id ) {
      w = e->d.w;
      break;
    }
    else {
      w2 = e->d.w;
      e2 = e;
    }
  }

  if(!w) {
    Oks::error_msg("OksForm::attach_previous()")
      << "Can't find entity # " << id << std::endl;

    return;
  }

  Widget tf_w(0);

  if(e->type == OksDlgEntity::TextField) {
    tf_w = w;

    std::list<OksDlgEntity*>::const_reverse_iterator i = entities.rbegin();
    for(; i != entities.rend(); ++i) {
      if(e == *i) {
        ++i; ++i;
        break;
      }
    }

    w = w2;
    e = e2;
    e2 = *i;
  }

  w2 = e2->get_parent_widget();

  if(user_dx == 0xFFFF) user_dx = form_x_offset;

  XtVaSetValues(w,
    XmNleftOffset, user_dx,
    XmNleftAttachment, XmATTACH_WIDGET,
    XmNleftWidget, w2,
    XmNtopOffset, (int)0,
    XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
    XmNtopWidget, w2,
    NULL
  );

  if(set_mx == false) return;

  if(e->type == OksDlgEntity::OptionMenu && e2->type == OksDlgEntity::ToggleButton) setMarginHeight(w, w2);
  else if(e->type == OksDlgEntity::PushButton && e2->type == OksDlgEntity::TextField) setMarginHeight(w, w2);
  else if(e->type == OksDlgEntity::ToggleButton && e2->type == OksDlgEntity::TextField) setMarginHeight(w, w2);
  else if(e->type == OksDlgEntity::ToggleButton && e2->type == OksDlgEntity::ToggleButton) setMarginHeight(w, w2);
  else if(e->type == OksDlgEntity::PushButton && e2->type == OksDlgEntity::ToggleButton) setMarginHeight(w, w2);

    // align text field after toggle button
  else if(e->type == OksDlgEntity::Label && e2->type == OksDlgEntity::ToggleButton) setMarginHeight(w2, w);

    // align text field after option menu
  else if(e->type == OksDlgEntity::ToggleButton && e2->type == OksDlgEntity::OptionMenu) {

      // workaround problem with RowColumn's XmNmarginHeight (change it to get real height)
    {
      Dimension w2h = 0, w2mh = 0;
      XtVaGetValues(w2, XmNheight, &w2h, XmNmarginHeight, &w2mh, NULL);
      XtVaSetValues(w2, XmNmarginHeight, (w2mh+1), NULL);
      XtVaSetValues(w2, XmNmarginHeight, w2mh, NULL);
    }

    setMarginHeight(w, w2);
  }
  else if(e->type == OksDlgEntity::Label && e2->type == OksDlgEntity::OptionMenu) {

    {
      Dimension w2h = 0, w2mh = 0;
      XtVaGetValues(w2, XmNheight, &w2h, XmNmarginHeight, &w2mh, NULL);
      XtVaSetValues(w2, XmNmarginHeight, (w2mh+1), NULL);
      XtVaSetValues(w2, XmNmarginHeight, w2mh, NULL);
    }

    setMarginHeight(w, w2);            // increase height of the label up to option menu

    if(tf_w) setMarginHeight(tf_w, w); // increase height of the text field up to updated label height
  }
}


void
OksForm::attach_previous(Widget id, Widget left_w, Widget top_w, XtArgVal dy, XtArgVal attachment_t) const
{
  XtVaSetValues(
    id,
    XmNleftOffset, form_x_offset, XmNleftAttachment, XmATTACH_WIDGET, XmNleftWidget, left_w,
    NULL
  );

  if(top_w) {
    XtVaSetValues(
      id,
      XmNtopOffset, dy, XmNtopAttachment, attachment_t, XmNtopWidget, top_w,
      NULL
    );
  }
}


void
OksForm::attach_right(short id) const
{
  Widget	     w = get_widget(id);
  OksDlgEntity::Type t = get_type(id);
  
  if( !w ) return;
  
  if(t == OksDlgEntity::Text || t == OksDlgEntity::List) w = XtParent(w);
  
  XtVaSetValues(w, 
    XmNrightOffset, (t != OksDlgEntity::PanedWindow ? form_x_offset : 1),
    XmNrightAttachment, XmATTACH_FORM,
    NULL
  );
}


void
OksForm::attach_bottom(short id) const
{
  std::list<OksDlgEntity*>::const_reverse_iterator i = entities.rbegin();

  Widget	w = 0;
  OksDlgEntity	*e = 0;

  for(; i != entities.rend(); ++i) {
    e = *i;
    if(e->type == OksDlgEntity::Form || e->type == OksDlgEntity::PanedWindow) continue;
    w = e->get_parent_widget();
    char attachment = XmATTACH_NONE;

    XtVaGetValues(w, XmNtopAttachment, &attachment, NULL);

    if(attachment == XmATTACH_WIDGET) break;
  }

  XtVaSetValues(w,
    XmNbottomAttachment, XmATTACH_FORM,
    XmNbottomOffset, (e->type == OksDlgEntity::PanedWindowWidget ? 3 : form_y_offset),
    NULL
  );


  if((!id) || (id == e->id)) return;

  XtVaSetValues(w, XmNtopAttachment, XmATTACH_NONE, NULL);

  int bottom_offset = 3;

  for(; i != entities.rend(); ++i) {
    e = *i;
    if(!e->id || e->type == OksDlgEntity::Form || e->type == OksDlgEntity::PanedWindow) continue;

    Widget w2 = e->get_parent_widget();

    char attachment = XmATTACH_NONE;

    XtVaGetValues(w2, XmNtopAttachment, &attachment, NULL);

    if(attachment == XmATTACH_WIDGET || attachment == XmATTACH_FORM) {
      XtVaSetValues(w2,
        XmNbottomOffset, bottom_offset,
        XmNbottomAttachment, XmATTACH_WIDGET,
        XmNbottomWidget, w,
        NULL
      );  	


        // get top attachment stored in the userData
        // it will be used during next loop iteration!

      XtPointer ptr;
      XtVaGetValues(w2, XmNuserData, &ptr, NULL);
      bottom_offset = (int)((unsigned long)ptr);

      if(id == e->id) return;

      XtVaSetValues(w2, XmNtopAttachment, XmATTACH_NONE, NULL);

      w = w2;	
    }
  }
}


Widget
OksForm::set_value(short id, const char *value, int pos, bool selectItem)
{
  const char * fname = "OksForm::set_value()";

  ERS_DEBUG( 5, "call OksForm::set_value(" << id << ", \'" << value << "\')");

  std::list<OksDlgEntity*>::iterator i = entities.begin();

  for(;i != entities.end(); ++i) {
    OksDlgEntity *e = *i;

    if(id == e->id) {
      Widget w = e->d.w;

      if(e->type == OksDlgEntity::OptionMenu) {
        Widget last_btn_w = 0;
	
	std::list<OksDlgEntity*>::reverse_iterator i2 = entities.rbegin();

	while(1) {
	  if(*i2 == e) break;
	  ++i2;
	}

	++i2;

        for(;i2 != entities.rend(); ++i2) {
	  e = *i2;

	  if(!last_btn_w) last_btn_w = e->d.w;

	  if(e->data && !strcmp(value, (char *)e->data)) {
            XtVaSetValues(w, XmNmenuHistory, e->d.w, NULL);
            return e->d.w;
          }

          if(e->id) {
            XmString xm_string = OksXm::create_string(value);	

            OksDlgEntity *new_e = new OksDlgEntity();

	    Widget pulldown;

            XtVaGetValues(w, XmNsubMenuId, &pulldown, NULL);

	    Widget new_w = XmCreatePushButtonGadget(pulldown, (char *)"option_menu_item", 0, 0);
            XtVaSetValues(new_w, XmNlabelString, xm_string, NULL);
            XtManageChild(new_w);
	    
	    XtVaSetValues(w, XmNmenuHistory, new_w, NULL);
	    
	    new_e->id = 0;
            new_e->d.w = new_w;
            new_e->data	= (void *)value;
            new_e->type	= OksDlgEntity::OptionMenu;


              // find position pointed by reverse iterator

            i = entities.begin();
	    
	    while(true) {
	      if(*i == *i2) break;
	      ++i;
	    }

              // insert new element at position pointed by reverse iterator

            ++i; // 21-02-03 *** IS: otherwise the element is put in a wrong place
            entities.insert(i, new_e);

            XmStringFree(xm_string);
	  
            return new_w;
          }
        }
      }
      else if(e->type == OksDlgEntity::TextField) {
        XmTextFieldSetString(w, const_cast<char *>(value));
        return w;
      }
      else if(e->type == OksDlgEntity::ComboBox) {
        XmString xm_string = OksXm::create_string(value);
        XmComboBoxSelectItem (w, xm_string);
        XmStringFree(xm_string);
        return w;
      }
      else if(e->type == OksDlgEntity::Text) {
        XmTextSetString(w, const_cast<char *>(value));
        return w;
      }
      else if(e->type == OksDlgEntity::Label) {
        XmString xm_s = OksXm::create_string(value);
	XtVaSetValues(w, XmNlabelString, xm_s, NULL);
        XmStringFree(xm_s);
        return w;
      }
      else if(e->type == OksDlgEntity::ToggleButton) {
        XmToggleButtonSetState(
          w, 
          (
            !strcmp(value, "On") ||
            !strcmp(value, "ON") ||
            !strcmp(value, "on") ||
            !strcmp(value, "1")
          ) ? true : false,
          false
	);
			
        return w;
      }
      else if(e->type == OksDlgEntity::List) {
        XmString xm_string = OksXm::create_string(value);
		
        XmListAddItem(w, xm_string, pos);
        if(selectItem) XmListSelectItem (w, xm_string, false);
        XmStringFree(xm_string);
			
        return w;
      }
      else {
        Oks::error_msg(fname)
          << "the method shall be used for OptionMenu, ComboBox, Text, Text Field or List.\n";
  				
        return 0;
      }
    }
  }

  Oks::error_msg(fname) << "Can't find entity # " << id << std::endl;

  return 0;
}

char *
OksForm::get_value(short id) const
{
  const char * fname = "OksForm::get_value()";

  std::list<OksDlgEntity*>::const_iterator i = entities.begin();

  for(; i != entities.end() ;++i) {
    OksDlgEntity *e = *i;

    if(!e->id) continue;

    if(id == e->id) {
      if(e->type == OksDlgEntity::OptionMenu) {
        Widget w;
        XtVaGetValues(e->d.w, XmNmenuHistory, &w, NULL);

        std::list<OksDlgEntity*>::const_reverse_iterator i2 = entities.rbegin();

	while(1) {
	  if(*i2 == *i) break;
	  ++i2;
	}

	while(i2 != entities.rend()) {
	  e = *i2;
	  if(e->d.w == w) return (char *)(e->data);
	  ++i2;
	}

        return 0;
      }
      else {
        Oks::error_msg(fname)
          << "In this implementation \"get_value\" must be used for OptionMenu only.\n";

        return 0;
      }
    }
  }
  
  Oks::error_msg(fname) << "Can't find entity # " << id << std::endl;

  return 0;
}


Widget
OksForm::get_widget(short id) const
{
  std::list<OksDlgEntity*>::const_iterator i = entities.begin();

  for(; i != entities.end() ;++i) {
    OksDlgEntity *e = *i;

    if(
     id == e->id &&
     e->type != OksDlgEntity::Form &&
     e->type != OksDlgEntity::PanedWindow
    ) return e->d.w;
  }

  return 0;
}


OksForm*
OksForm::get_form(short id) const
{
  std::list<OksDlgEntity*>::const_iterator i = entities.begin();

  for(; i != entities.end() ;++i) {
    OksDlgEntity *e = *i;

    if(id == e->id && e->type == OksDlgEntity::Form) return e->d.f;
  }

  return 0;
}


OksPanedWindow*
OksForm::get_paned_window(short id) const
{
  std::list<OksDlgEntity*>::const_iterator i = entities.begin();

  for(; i != entities.end() ;++i) {
    OksDlgEntity *e = *i;

    if(id == e->id && e->type == OksDlgEntity::PanedWindow) return e->d.p;
  }

  return 0;
}


OksDlgEntity::Type
OksForm::get_type(short id) const
{
  std::list<OksDlgEntity*>::const_iterator i = entities.begin();

  for(; i != entities.end() ;++i) {
    OksDlgEntity *e = *i;

    if(id == e->id) return e->type;
  }

  return OksDlgEntity::Unknown;
}

void
OksForm::set_option_menu_cb(short id, XtCallbackProc f, XtPointer p) const
{
  std::list<OksDlgEntity*>::const_iterator i = entities.begin();

  for(; i != entities.end() ;++i) {
    if(id == (*i)->id) {
      std::list<OksDlgEntity*>::const_reverse_iterator i2 = entities.rbegin();

      while(1) {
        if(*i2 == *i) break;
	++i2;
      }

      for(++i2; i2 != entities.rend() && (*i2)->id == 0;++i2)
        XtAddCallback((*i2)->d.w, XmNactivateCallback, f, p);

      return;
    }
  }
}

void
OksForm::map_widget(short id, bool b) const
{
  Widget w = get_widget(id);
  if(w) {
    if(b == false && XtIsManaged(w) == true) {
      XtUnmanageChild(w);
    }
    else if(b == true && XtIsManaged(w) == false) {
      XtManageChild(w);
    }
  }
}


Widget
OksForm::add_radio_button(short id, const char *d)
{
  Widget w = add(OksDlgEntity::ToggleButton, id, (void *)d);
  XtVaSetValues(w, XmNindicatorType, XmONE_OF_MANY, NULL);
  return w;
}


OksPanedWindow::~OksPanedWindow()
{
  while(items.size()) {
    OksDlgEntity *e = items.front();
    delete e->d.f;
    delete e;
    items.pop_front();
  }
}

void
OksPanedWindow::add_form(OksForm *f, short id)
{
  OksDlgEntity*	entity = new OksDlgEntity();

  entity->d.f	= f;
  entity->data	= 0;
  entity->id	= id;
  entity->type	= OksDlgEntity::Form;

  items.push_back(entity);

  XtVaSetValues(f->get_form_widget(), XmNallowResize, True, NULL);

  f->show();
}


OksForm*
OksPanedWindow::get_form(short id) const
{
  std::list<OksDlgEntity*>::const_iterator i;

  for(i=items.begin(); i != items.end(); ++i)
    if((*i)->id == id) return (*i)->d.f;

  return 0;
}


OksDialog::OksDialog() : OksForm ()
{
  ;
}


OksDialog::OksDialog(const char *title, const OksTopDialog *top) : OksForm ()
{
  Arg args[5];

  XtSetArg(args[0], XmNautoUnmanage, false);
  XtSetArg(args[1], XmNtitle, title);
  XtSetArg(args[2], XmNiconName, title);
  XtSetArg(args[3], XmNallowShellResize, true);

  form = XmCreateForm(
    XtCreatePopupShell(
      (char *)top->app_class_name(),
      topLevelShellWidgetClass,
      top->app_shell(),
      args,
      4
    ),
    (char *)"form",
    args,
    1
  );
}


OksDialog::~OksDialog()
{
  if(form && XtParent(form)) {
    XtDestroyWidget(XtParent(form));
    form = 0;
  }
}


void
OksDialog::show(XtGrabKind type) const
{
  OksForm::show();
  Widget shell_w = XtParent(form);
  XtRealizeWidget(shell_w);
  XtPopup(shell_w, type);
}

void
OksDialog::setTitle(const char *s, const char *s2, const char *s3) const
{
  std::string t(s);
  
  if(s2) {
    t += " : ";
    t += s2;

    if(s3) {
      t += '@';
      t += s3;
    }
  }

  XtVaSetValues(
    XtParent(form),
    XmNtitle, t.c_str(),
    XmNiconName, t.c_str(),
    NULL
  );
}

void
OksDialog::setIcon(Pixmap pixmap) const
{
  if(pixmap)
    XtVaSetValues(XtParent(form), XmNiconPixmap, pixmap, NULL);
}


void
OksDialog::setIcon(OksDialog *d) const
{
  Pixmap pixmap;

  XtVaGetValues(XtParent(d->form), XmNiconPixmap, &pixmap, NULL);
  
  setIcon(pixmap);
}


void
OksDialog::createOksDlgHeader(Pixmap *pixmap, XtEventHandler hFN, void *hFN_arg, short id1, const char *s1, short id2, const char *s2, short id3, const char *s3)
{
  Widget w = add(OksDlgEntity::Label, OksDlgEntity::idPixmap, (char *)"");
 
  if(*pixmap)
    XtVaSetValues(w, XmNlabelType, XmPIXMAP, XmNlabelPixmap, *pixmap, NULL);
  else {
    XmString xm_string = OksXm::create_string("========\n|pixmap|\n========");
    XtVaSetValues(w, XmNlabelString, xm_string, NULL);
  }

  if(hFN) XtAddEventHandler(w, ButtonPressMask, False, hFN, (XtPointer)hFN_arg);

  Widget tf1 = add_text_field(id1, s1);
  attach_right(id1);
  attach_previous(get_previous(id1), w, w, 0, XmATTACH_OPPOSITE_WIDGET);

  add_text_field(id2, s2);
  attach_right(id2);
  attach_previous(get_previous(id2), w, tf1, 2, XmATTACH_WIDGET);

  if(id3 && s3) {
    add_text_field(id3, s3);
    attach_right(id3);
    attach_previous(get_previous(id3), w, 0, 0, 0);
  }

  add_separator();
}

void
OksDialog::setMinSize() const
{
  Widget w = XtParent(form);
  Dimension width, height;
  
  XtVaGetValues(w, XmNwidth, &width, XmNheight, &height, NULL);
  XtVaSetValues(w, XmNminWidth, width + 5, XmNminHeight, height + 5, NULL);

  XtVaSetValues(XtParent(form), XmNallowShellResize, false, NULL);
}


void
OksDialog::setMaxSize(Dimension width, Dimension height) const
{
  Widget w = XtParent(form);
  XtVaSetValues(w, XmNmaxWidth, width, XmNmaxHeight, height, NULL);

  XtVaSetValues(XtParent(form), XmNallowShellResize, false, NULL);
}


	//
	// Set minimal possible height of the form
	// when all it's child are visible.
	//

void
OksForm::set_min_height() const
{
  OksDlgEntity *e = entities.back();
  Widget lw = e->get_parent_widget();

  Dimension lh, ly, fh;

  XtVaGetValues(lw, XmNheight, &lh, XmNy, &ly, NULL);

  int new_fh = ly + lh + form_y_offset;

  Widget frame_w = XtParent(XtParent(get_form_widget()));

  XtVaGetValues(frame_w, XmNheight, &fh, NULL);

  if(fh > new_fh) {
    XtVaSetValues(frame_w, XmNheight, new_fh, NULL);
  }
}

static Dimension base_x = 0;   // position of main window
static Dimension base_y = 0;

static Dimension last_x = 0;   // position of last child window
static Dimension last_y = 0;

static Dimension screen_w = 0; // screen size
static Dimension screen_h = 0;

static void init_screen_data(Widget w)
{
  if(!screen_w) {
    screen_w = WidthOfScreen(XtScreen(w));
    screen_h = HeightOfScreen(XtScreen(w));

    ERS_DEBUG(1, "screen_w: " << screen_w << ", screen_h: " << screen_h);
  }
}

void
OksDialog::resetCascadePos(Widget w, Dimension new_x, Dimension new_y)
{
  init_screen_data(w);

  if(new_x != base_x || new_y != base_y) {
    base_x = new_x;
    base_y = new_y;

    if(base_x + screen_w/4 > screen_w) base_x = (screen_w/4)*3;
    if(base_y + screen_h/4 > screen_h) base_y = (screen_h/4)*3;

    ERS_DEBUG(1, "base_x: " << base_x << ", base_y: " << base_y);

    last_x = base_x;
    last_y = base_y;
  }
}

void
OksDialog::setCascadePos() const
{
  Widget widget = XtParent(form);

  init_screen_data(widget);

  last_x += screen_w/32;
  last_y += screen_h/32;

  Dimension w, h;
 
  XtVaGetValues(widget, XmNwidth, &w, XmNheight, &h, NULL);

  if(
   ((last_x + w) > screen_w) ||
   ((last_y + h) > screen_h)
  ) {
    last_x = base_x;
    last_y = base_y;
  }

  XtVaSetValues(widget, XmNx, last_x, XmNy, last_y, NULL);
}


Widget
OksDialog::addCloseButton(const char * msg)
{
  Widget w = add_push_button(OksDlgEntity::idCloseButton, msg);
  
  XtVaSetValues(
    w,
    XmNrightOffset, form_x_offset, XmNrightAttachment, XmATTACH_FORM,
    XmNleftAttachment, XmATTACH_NONE,
    NULL
  );
  	
  return w;
}


#ifdef WIN32
  extern "C" { void HCLXmInit(void); }
#endif 

static void xt_warning (char *message)
{
  if(ers::debug_level() >= 1) {
    fprintf (stderr, "Xt Warning: %s\n", message);
  }
}

OksTopDialog::OksTopDialog(const char *title, int *app_argc, char **app_argv, const char *class_name, const char * fallback_resources[][2]) :
  OksDialog		(),
  p_app_class_name	(class_name),
  p_fallback_resources  (0)
{
  Arg	args[10];
  int	n = 0;

#ifdef WIN32
  HCLXmInit();
#endif

  XtToolkitInitialize ();

  p_app_context = XtCreateApplicationContext();

  XtAppSetWarningHandler (p_app_context, xt_warning);

  if(fallback_resources) {
    unsigned int i = 0;
    unsigned int len=0;

    for(; fallback_resources[i][0] != 0; ++i) { ++len; }

    p_fallback_resources = new char * [len+1];

    for(i = 0; fallback_resources[i][0] != 0; ++i) {
      std::string s;
      s += fallback_resources[i][0];
      s += + ": ";
      s += fallback_resources[i][1];
      p_fallback_resources[i] = new char[s.size() + 1];
      strcpy(p_fallback_resources[i], s.c_str());
    }

    p_fallback_resources[i] = 0;

    XtAppSetFallbackResources(p_app_context, p_fallback_resources);
  }

  p_display = XtOpenDisplay (p_app_context, 0, app_argv[0], p_app_class_name, 0, 0, app_argc, app_argv);

  if(!p_display) {
    Oks::error_msg(title) << "can't open display, exiting...\n";
    exit(1);
  }

  XtSetArg(args[n], XmNallowShellResize, true); n++;
  XtSetArg(args[n], XmNtitle, title); n++;
  XtSetArg(args[n], XmNargc, *app_argc); n++;
  XtSetArg(args[n], XmNargv, app_argv); n++;


    // fix KDE bug

  if(fallback_resources) {
    XrmDatabase db;

    db = XtDatabase(p_display);

    for(unsigned int i = 0; fallback_resources[i][0] != 0; ++i) {
      if(strstr(fallback_resources[i][0], "ground")) {
        XrmPutStringResource(&db, fallback_resources[i][0], fallback_resources[i][1]);
      }
    }
  }

  p_app_shell = XtAppCreateShell(app_argv[0], p_app_class_name, (WidgetClass)applicationShellWidgetClass, p_display, args, n );

  n = 0;
  XtSetArg(args[n], XmNautoUnmanage, false); n++;
  XtSetArg(args[n], XmNallowShellResize, true); n++;

  form = XmCreateForm(p_app_shell, (char *)"form", args, n);

  OksXm::init_mouse_wheel_actions(p_app_context);
}


OksTopDialog::~OksTopDialog()
{
  if(p_fallback_resources) {
    for(unsigned int i = 0; p_fallback_resources[i] != 0; ++i) {
      delete [] p_fallback_resources[i];
    }
    delete [] p_fallback_resources ;
    p_fallback_resources = 0;
  }
}
