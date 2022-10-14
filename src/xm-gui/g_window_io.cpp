#include <ers/ers.h>

#include <Xm/Xm.h>
#include <Xm/DrawingA.h>
#include <Xm/Label.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/SelectioB.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>

#include <fstream>

#include <oks/xm_popup.h>
#include <oks/xm_utils.h>

#include "data_editor_main_dlg.h"
#include "data_editor_exceptions.h"
#include "g_window.h"
#include "g_window_info.h"

#include "g_class.h"
#include "g_object.h"

#include "exit.xpm"


XtIntervalId			G_Window::auto_scrolling_timer;
G_Window *			G_Window::auto_scrolling_window;
G_Window::AddFile_Result	G_Window::p_add_file_result;

extern OksDataEditorMainDialog * mainDlg;

void
G_Window::closeCB(Widget, XtPointer client_data, XtPointer)
{
  delete (G_Window *)client_data;
}

void
G_Window::helpCB(Widget, XtPointer, XtPointer)
{
  mainDlg->show_help_window("GraphicalWindow.html");
}


void
G_Window::drawCB(Widget, XtPointer clientData, XtPointer callData)
{
  ((G_Window *)clientData)->draw(
    (XExposeEvent *)((XmDrawingAreaCallbackStruct *)callData)->event
  );
}


void
G_Window::resizeCB(Widget, XtPointer clientData, XtPointer)
{
  ERS_DEBUG( 1, "call G_Window::resizeCB()" );
  ((G_Window *)clientData)->draw(0);
}


void
G_Window::inputCB(Widget, XtPointer clientData, XtPointer callData)
{
  XmDrawingAreaCallbackStruct * d = (XmDrawingAreaCallbackStruct *)callData;
  if(d->event->type == ButtonPress || d->event->type == ButtonRelease) {
    ((G_Window *)clientData)->input(&d->event->xbutton);
  }
}


void
G_Window::motionAC(Widget, XtPointer client_data, XEvent *event, Boolean *)
{
  ((G_Window *)client_data)->motion(
    ((XMotionEvent *)event)->x,
    ((XMotionEvent *)event)->y
  );
}


void
G_Window::leaveAC(Widget, XtPointer client_data, XEvent */*event*/, Boolean *)
{
  ((G_Window *)client_data)->motion(
    (Dimension)-1,
    (Dimension)-1
  );
}


void
G_Window::input(XButtonEvent *event)
{
    // unpress table button if there is any one

  G_Object::unpress_table_buttons();


    // remove tips tool if exist

  g_context.set_tips_text("");
  g_context.draw_tips();


    // find an object where the event happens

  G_Object *obj = 0;

  Dimension ix = event->x;
  Dimension iy = event->y;

  for(std::list<G_Object *>::iterator i = objects.begin(); i != objects.end(); ++i) {
    if((*i)->is_shown()) {
      if((*i)->is_point_inside_object(ix, iy)) {
        obj = *i;
	break;
      }
    }
  }


    // if object found

  if(obj) {
    size_t num_of_obj = objects.size();

    obj->process_button_event(event, this);

    if(event->button == 2 && (event->state & Button2Mask) == 0)
      auto_scrolling_window = this;
    
    if(
     (objects.size() != num_of_obj) ||
     (obj->is_valid() == false)
    ) draw(0);
  }
  else {
    create_popup((XButtonPressedEvent *)event);
  }
}


void
G_Window::popupAC(Widget, XtPointer client_data, XEvent *event, Boolean *)
{
  if (((XButtonEvent *)event)->button != Button3) return;

  ((G_Window *)client_data)->create_popup((XButtonPressedEvent *)event);
}

void
G_Window::create_popup(XButtonPressedEvent * event)
{
  if(event->button != Button3 || (event->state & Button3Mask) != 0) return;

  OksPopupMenu popup(d_area);

  {
    Widget cascade = popup.addCascade("Create New");
    bool enabled = is_data_file_choosen();
    size_t count = idAddNew;

    for(std::list<const G_Class *>::iterator i = top_classes.begin(); i != top_classes.end(); ++i) {
      const char * class_name = (*i)->get_name().c_str();

      if(enabled)
        popup.addItem(class_name, count++, popupCB, (XtPointer)this, cascade);
      else
        popup.addDisableItem(class_name, cascade);
    }

    popup.add_separator();
  }

  {
    Widget cascade = popup.addCascade("Show on Top");

    unsigned long count = idShow;

    for(std::list<ShowInfo>::iterator i=show_infos.begin(); i != show_infos.end(); ++i) {
      if((*i).name) {
        Widget cascade2 = popup.addCascade((*i).name, cascade);

        popup.addToggleItem("All", count++, (*i).show_state == G_Window::ShowAll, popupCB, (XtPointer)this, cascade2);

        if((*i).show_extended) {
          popup.addToggleItem("Used", count++, (*i).show_state == G_Window::ShowUsed, popupCB, (XtPointer)this, cascade2);
          popup.addToggleItem("Non-Used", count++, (*i).show_state == G_Window::ShowNoneUsed, popupCB, (XtPointer)this, cascade2);
        }

        popup.addToggleItem("None", count++, (*i).show_state == G_Window::ShowNone, popupCB, (XtPointer)this, cascade2);
      }
      else {
        popup.add_separator(cascade);
      }
    }

    popup.add_separator();
  }


  {
/*    Widget cascade = popup.addCascade("Arrange");

    popup.addItem("As row", idArrangeAsRow, popupCB, (XtPointer)this, cascade);
    popup.addItem("As column", idArrangeAsColumn, popupCB, (XtPointer)this, cascade);
    popup.addItem("By classes", idArrangeByClasses, popupCB, (XtPointer)this, cascade);
    popup.addItem("Wrap any", idWrapAny, popupCB, (XtPointer)this, cascade);
    popup.addItem("Wrap by classes", idWrapByClasses, popupCB, (XtPointer)this, cascade);

    popup.add_separator(cascade);

    popup.addItem("Many children per line", idManyChildrenPerLine, popupCB, (XtPointer)this, cascade);
    popup.addItem("One child per line", idOneChildPerLine, popupCB, (XtPointer)this, cascade);

    popup.add_separator(cascade);
*/
//    popup.addItem("Sort by ID", idSortByID, popupCB, (XtPointer)this, cascade);
    popup.addItem("Sort by ID", idSortByID, popupCB, (XtPointer)this);

    popup.add_separator();
  }

  popup.addItem("Properties", idProperties, popupCB, (XtPointer)this);
  popup.addItem("Print", idPrint, popupCB, (XtPointer)this);
  popup.addItem("Refresh", idRefresh, popupCB, (XtPointer)this);

  popup.show(event);
}

void
G_Window::popupCB(Widget w, XtPointer client_data, XtPointer)
{
  long user_input = (long)OksXm::get_user_data(w);

  G_Window * window = (G_Window *)client_data;

  switch(user_input) {
    case idSortByID:
      window->sort_by_id();
      break;

    case idProperties:
      window->g_context.create_parameters_dialog(window->parent, window->title, refresh, (XtPointer)window);
      break;

    case idPrint:
      window->g_context.create_print_dialog(window->parent, window->title, refresh, (XtPointer)window, window->parent->data_files());
      break;

    default:
      if(user_input >= idShow && user_input < idAddNew) {
        window->make_shown(user_input - idShow);
      }
      else if(user_input >= idAddNew) {
	user_input -= idAddNew;

	std::list<const G_Class *>::iterator i = window->top_classes.begin();
        while(user_input--) ++i;

        window->make_new_object_dlg(0, &(*i)->get_name(), 0);
      }
  }

  window->draw(0);
}


void
G_Window::motion(Dimension ix, Dimension iy)
{
  for(std::list<G_Object *>::iterator i = objects.begin(); i != objects.end(); ++i)
    if((*i)->process_motion_event(ix, iy)) return;
  
  G_Object::clean_motion_timer(&g_context);
}


void
G_Window::changeDataFileCB(Widget w, XtPointer data, XtPointer)
{
  G_Window *dlg = (G_Window *)data;

  XmString s;
  XtVaGetValues(w, XmNlabelString, &s, NULL);

  OksXm::AutoCstring value(OksXm::string_to_c_str(s));

  bool choosen = (
    (!strcmp(value.get(), choose_file_name_str)) ? false :
    (!strcmp(value.get(), create_new_file_name_str)) ? false :
    true
  );

  OksFile * fh = dlg->parent->find_data_file(value.get());
  OksFile * old = dlg->parent->get_active_data();

  if(fh != old) {
    try {
      dlg->parent->set_active_data(fh);
      dlg->parent->update_data_file_row(fh);
      if(old) dlg->parent->update_data_file_row(old);
    }
    catch(oks::exception& ex) {
      OksDataEditorMainDialog::report_exception("Change Data File", ex, dlg->get_form_widget());
      choosen = false;
      dlg->set_value(idInFile, choose_file_name_str);
    }
  }

  for(std::list<Widget>::iterator iw = dlg->file_sensible.begin(); iw != dlg->file_sensible.end(); ++iw)
    XtSetSensitive(*iw, choosen);

  if(!strcmp(value.get(), create_new_file_name_str)) dlg->add_new_data_file();

  XmStringFree(s);
}


void
G_Window::add_new_data_file()
{
  Arg args[4];
  XmString str = OksXm::create_string("Input optional database file type\n(put any string or leave empty)");
  Widget dialog;

  XtSetArg(args[0], XmNallowShellResize, true);
  XtSetArg(args[1], XmNdialogStyle, XmDIALOG_APPLICATION_MODAL);
  XtSetArg(args[2], XmNselectionLabelString, str);
  XtSetArg(args[3], XmNchildPlacement, XmPLACE_BELOW_SELECTION);

  dialog = XmCreatePromptDialog(get_parent()->get_form_widget(), (char *)"prompt dialog", args, 4);

  std::string s("Create new \'");
  s += type()->title();
  s += "\' database file";

  XtVaSetValues(XtParent(dialog), XmNtitle, s.c_str(), NULL);

  XtSetArg(args[0], XmNmarginWidth, 0);
  XtSetArg(args[1], XmNmarginHeight, 0);
  Widget work_area = XmCreateRowColumn(dialog, (char *)"workarea", args, 2);
  XtManageChild(work_area);

  XmTextFieldSetString(
    XmSelectionBoxGetChild(dialog, XmDIALOG_VALUE_TEXT),
    const_cast<char *>(type()->title().c_str())
  );

  XtManageChild(XmCreateLabel(work_area, (char *)"Name of file (the root-file-related or an absolute path):", 0, 0));

  Widget text_field = XmCreateTextField(work_area, (char *)"field", args, 0);
  XtManageChild (text_field);
  XmTextFieldSetString(text_field, (char *)".data.xml");
  OksXm::set_user_data(dialog, (XtPointer)text_field);


      // add callback for standard buttons

  XtAddCallback(dialog, XmNokCallback, G_Window::createNewDataFileCB, (XtPointer)this);
  XtAddCallback(dialog, XmNcancelCallback, G_Window::createNewDataFileCB, (XtPointer)this);
  XtAddCallback(dialog, XmNhelpCallback, G_Window::createNewDataFileCB, (XtPointer)this);

  XtManageChild(dialog);

  XmStringFree(str);
}


void
G_Window::createNewDataFileCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  G_Window * window = (G_Window *)client_data;

  if(((XmSelectionBoxCallbackStruct *)call_data)->reason == XmCR_OK) {

      // read file name and id

    OksXm::AutoCstring file_name(XmTextGetString((Widget)OksXm::get_user_data(w)));
    OksXm::AutoCstring file_id(XmTextGetString(XmSelectionBoxGetChild(w, XmDIALOG_VALUE_TEXT)));

      // check if the file already exists

    std::ifstream f(file_name.get());
    if(f) {
      std::string s("File \'");
      s += file_name.get();
      s += "\' already exists.\n";

      Widget dlg = OksXm::create_dlg_with_pixmap(window->parent->menuBar, "File Exists", exit_xpm, s.c_str(), "Overwrite", "Add", "Cancel", 0, 0);

      XtVaSetValues(dlg, XmNdialogStyle, XmDIALOG_APPLICATION_MODAL, NULL);

      XtAddCallback(dlg, XmNokCallback, addFileCB, (XtPointer)AddFile_Overwrite);
      XtAddCallback(dlg, XmNcancelCallback, addFileCB, (XtPointer)AddFile_Add);
      XtAddCallback(XmSelectionBoxGetChild(dlg, XmDIALOG_HELP_BUTTON),
        XmNactivateCallback, addFileCB, (XtPointer)AddFile_Cancel);

      OksXm::set_close_cb(XtParent(dlg), addFileCB, (XtPointer)AddFile_Cancel);

      XtManageChild(dlg);

      p_add_file_result = AddFile_NoAnswer;

      while(p_add_file_result == AddFile_NoAnswer)
        XtAppProcessEvent(XtWidgetToApplicationContext(dlg), XtIMAll);

      XtDestroyWidget(XtParent(dlg));


        // if 'Overwrite' choosen

      if(p_add_file_result == AddFile_Overwrite)
        window->create_new_datafile(file_name.get(), file_id.get());


        // if 'Add' choosen

      else if(p_add_file_result == AddFile_Add) {
        window->create_new_datafile(file_name.get(), file_id.get());
//	mainDlg->change_datafiles();  // FIXME
      }
    }


      // file does not exists, create new data file

    else
      window->create_new_datafile(file_name.get(), file_id.get());

  }
  else if(((XmSelectionBoxCallbackStruct *)call_data)->reason == XmCR_HELP) {
    mainDlg->show_help_window("new-file.html");
  }
}


void
G_Window::addFileCB(Widget, XtPointer data, XtPointer)
{
//  p_add_file_result = (AddFile_Result)data;
  long i = reinterpret_cast<long>(data);
  p_add_file_result = (AddFile_Result)i;
}


struct NewObjData {
  Widget		obj_id_w;
  Widget		class_w;
  G_Object *		parent;
  const std::string *	class_name;
  const std::string *	relationship_name;
  G_Window *		window;
};


void
G_Window::newObjCB(Widget, XtPointer client_data, XtPointer call_data)
{
  NewObjData * data = static_cast<NewObjData *>(client_data);

  if(((XmSelectionBoxCallbackStruct *)call_data)->reason == XmCR_OK && data) {

      // read object id

    OksXm::AutoCstring value(XmTextGetString(data->obj_id_w));


      // read object type if defined

    char * type = 0;

    if(data->class_w) {
      XmString s;
      Widget btn = 0;
      XtVaGetValues(data->class_w, XmNmenuHistory, &btn, NULL);
      XtVaGetValues(btn, XmNlabelString, &s, NULL);
      type = OksXm::string_to_c_str(s);
      XmStringFree (s);
    }


      // always set active data file

    bool file_is_active = data->window->set_selected_file_active();


      // create new graphical object

    OksClass * obj_class = data->window->parent->find_class(
      type ? const_cast<const char *>(type) : data->class_name->c_str()
    );

    try {
      OksObject * new_obj = file_is_active ? new OksObject(obj_class, value.get()) : 0;

      G_Object * obj = (
        new_obj
          ? G_Object::find_g_object(new_obj, &data->window->objs_set, true)
          : 0
      );

      if(obj) {
          // draw object in tabular form

        obj->set_table_state();


          // link object with parent

        if(data->parent && data->relationship_name) {
          data->parent->show_children();
          OksDataEditorMainDialog::link_rel_objects(
            data->relationship_name->c_str(), data->parent, obj->get_oks_object()
          );
        }


          // redraw window and make object visible

        data->window->draw(0);
        data->window->make_object_visible(obj);
      }
      else {
        ers::error(OksDataEditor::InternalProblem(ERS_HERE, "cannot create object"));
      }

      if(type) XtFree(type);
    }
    catch(oks::exception& ex)
    {
      ers::error(OksDataEditor::InternalProblem(ERS_HERE, ex.what()));
    }
  }
  else if(((XmSelectionBoxCallbackStruct *)call_data)->reason == XmCR_HELP && data) {
    mainDlg->show_help_window("new-object.html");
  }

  delete data;
}


void
G_Window::make_new_object_dlg(G_Object * o, const std::string * cn, const std::string * rn)
{
    // create dialog title

  std::string dialog_title = "New ";
  dialog_title += *cn;

  if(o) {
    dialog_title += " (child of ";
    dialog_title += G_Object::to_string(o->get_oks_object());
    dialog_title += " via \'";
    dialog_title += *rn;
    dialog_title += "\')";
  }

    // create prompt string

  std::string prompt_string = "Input object's ID for new \'";
  prompt_string += *cn;
  prompt_string += "\':\n(empty string means anonymous object)";


    // create dialog

  {
    Arg      args[5];
    XmString string = OksXm::create_string(prompt_string.c_str());
    XmString string2 = OksXm::create_string("(empty string means anonymous object)");
    Widget   dialog;

    XtSetArg(args[0], XmNallowShellResize, true);
    XtSetArg(args[1], XmNdialogStyle, XmDIALOG_APPLICATION_MODAL);
    XtSetArg(args[2], XmNselectionLabelString, string);
    XtSetArg(args[3], XmNchildPlacement, XmPLACE_BELOW_SELECTION);

    dialog = XmCreatePromptDialog(get_form_widget(), (char *)"prompt dialog", args, 4);

    XtVaSetValues(XtParent(dialog), XmNtitle, dialog_title.c_str(), NULL);


      // process object types if defined

    std::list<OksClass *> * types = get_new_object_types(get_parent()->find_class(*cn));
    Widget option_menu = 0;

    if(types) {
      XtSetArg(args[0], XmNmarginHeight, 0);
      XtSetArg(args[1], XmNmarginWidth, 0);

      Widget work_area = XmCreateRowColumn(dialog, (char *)"workarea", args, 2);

      XtManageChild(work_area);

      Widget pulldown = XmCreatePulldownMenu(work_area, (char *)"pulldown_menu", 0, 0);

      for(std::list<OksClass *>::iterator type = types->begin(); type != types->end(); ++type) {
        XmString btn_name = OksXm::create_string((*type)->get_name().c_str());

        Widget btn_w = XmCreatePushButtonGadget(pulldown, (char *)"option_menu_item", 0, 0);
        XtVaSetValues(btn_w, XmNlabelString, btn_name, NULL);
        XtManageChild(btn_w);

        XmStringFree(btn_name);
      }

      XmString menu_name = OksXm::create_string("Choose type:");

      XtSetArg(args[0], XmNlabelString, menu_name);
      XtSetArg(args[1], XmNsubMenuId, pulldown);

      option_menu = XmCreateOptionMenu(work_area, (char *)"simple", args, 2);
      XtManageChild(option_menu);

      OksXm::set_user_data(XmSelectionBoxGetChild(dialog, XmDIALOG_OK_BUTTON), (XtPointer)option_menu);

      XmStringFree(menu_name);

      delete types;
    }

    NewObjData * data = new NewObjData();
	
    if(data) {
      data->obj_id_w	  	= XmSelectionBoxGetChild(dialog, XmDIALOG_VALUE_TEXT);
      data->class_w		= option_menu;
      data->parent		= o;
      data->class_name		= cn;
      data->relationship_name	= rn;
      data->window		= this;
    }

      // add callback for standard buttons

    XtAddCallback(dialog, XmNokCallback, G_Window::newObjCB, (XtPointer)data);
    XtAddCallback(dialog, XmNcancelCallback, G_Window::newObjCB, (XtPointer)data);
    XtAddCallback(dialog, XmNhelpCallback, G_Window::newObjCB, (XtPointer)data);

    XtManageChild(dialog);

    XmStringFree(string);
    XmStringFree(string2);
  }
}


bool
G_Window::create_child_object(G_Object * o, const std::string& cn, const std::string& rn)
{
  for(std::list<G_Object *>::const_iterator i = objects.begin(); i != objects.end(); ++i) {
    if(*i == o) {
      make_new_object_dlg(o, &cn, &rn);
      return true;
    }
  }

  return false;
}
