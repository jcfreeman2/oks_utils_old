#include <boost/date_time/posix_time/time_formatters.hpp>

#include <oks/class.h>
#include <oks/xm_utils.h>
#include <oks/xm_popup.h>

#include <Xm/Xm.h>
#include <Xm/List.h>
#include <Xm/TextF.h>

#include "schema_editor_schema_dlg.h"
#include "schema_editor_class_dlg.h"
#include "schema_editor_main_dlg.h"
#include "schema_editor.h"


void
OksSchemaEditorSchemaDialog::default_clistCB(Widget, XtPointer client_data, XtPointer call_data)
{
  XmListCallbackStruct * cbs = (XmListCallbackStruct *)call_data;
  OksSchemaEditorSchemaDialog * dlg = (OksSchemaEditorSchemaDialog *)client_data;
  OksXm::AutoCstring className(OksXm::string_to_c_str(cbs->item));
  if(OksClass * cl = dlg->parent()->find_class(className.get())) dlg->parent()->create_class_dlg(cl);
}

static std::string
get_file_path(const OksKernel& kernel, const char * value, OksFile * file_h)
{
  try {
    return kernel.get_file_path(value, file_h);
  }
  catch(std::exception& ex) {
    static std::string empty;
    return empty;
  }
}


void
OksSchemaEditorSchemaDialog::default_ilistCB(Widget, XtPointer client_data, XtPointer call_data)
{
  XmListCallbackStruct * cbs = (XmListCallbackStruct *)call_data;
  OksSchemaEditorSchemaDialog * dlg = (OksSchemaEditorSchemaDialog *)client_data;
  OksXm::AutoCstring iname(OksXm::string_to_c_str(cbs->item));

  std::string s = get_file_path(*dlg->parent(), iname.get(), dlg->file_h);

  OksFile * f = (!s.empty() ? dlg->parent()->find_schema_file(s) : 0);

  if(f) {
    dlg->parent()->create_schema_dlg(f);
  }
  else {
    std::cerr << "ERROR: the file \'" << iname.get() << "\' is not loaded\n";
  }
}


void
OksSchemaEditorSchemaDialog::clistCB(Widget w, XtPointer d, XtPointer) 
{
  OksSchemaEditorSchemaDialog * dlg = (OksSchemaEditorSchemaDialog *)d;
  OksSchemaEditorMainDialog * parent = dlg->parent();

  long user_data = (long)OksXm::get_user_data(w);
  Widget list = XtParent(XtParent(XtParent(w)));
  OksPopupMenu::destroy(w);

  try {
    switch (user_data) {
      case idAcSchemaClassNew: {
        OksFile * old_active = parent->get_active_schema();
        if(old_active != dlg->file_h) parent->set_active_schema(dlg->file_h);
        parent->create_new_class_dlg();
        if(old_active != dlg->file_h) parent->set_active_schema(old_active);
        parent->update_schema_row(dlg->file_h);
        break; }

      case idAcSchemaClassShow:
      case idAcSchemaClassDelete: {
        OksXm::AutoCstring className(OksXm::List::get_selected_value(list));
        if(OksClass * cl = parent->find_class(className.get())) {
          if(user_data == idAcSchemaClassShow)
            parent->create_class_dlg(cl);
          else {
            OksClass::destroy(cl);
            dlg->refresh();
          }
        }
        break; }
    }
  }
  catch(oks::exception& ex) {
    std::cerr << "ERROR: " << ex.what() << std::endl;
  }
}


void
OksSchemaEditorSchemaDialog::ilistCB(Widget w, XtPointer d, XtPointer) 
{
  OksSchemaEditorSchemaDialog * dlg = (OksSchemaEditorSchemaDialog *)d;
  OksXm::AutoCstring name(OksXm::List::get_selected_value(XtParent(XtParent(XtParent(w)))));

  long user_data = (long)OksXm::get_user_data(w);
  OksPopupMenu::destroy(w);

  switch (user_data) {
    case idAcShowFile:
      if(name.get()) {
        std::string s = get_file_path(*dlg->parent(), name.get(), dlg->file_h);
	OksFile * f = (!s.empty() ? dlg->parent()->find_schema_file(s) : 0);
	if(f) {
	  dlg->parent()->create_schema_dlg(f);
	}
      }
      break;

    case idAcAddFile: {
      std::string s = OksXm::ask_file(
        dlg->get_form_widget(),
	"Input name of file to be included",
	"Add include file",
	(
	  !dlg->parent()->get_user_repository_root().empty() ? dlg->parent()->get_user_repository_root().c_str() :
	  !dlg->parent()->get_repository_dirs().empty() ? dlg->parent()->get_repository_dirs().front().c_str() :
	  nullptr
	), "*.schema.xml"
      );

      if(s.length()) {
        try {
          dlg->file_h->add_include_file(s);
	  dlg->refresh();
	  dlg->parent()->update_schema_row(dlg->get_file());
        }
        catch (oks::exception & ex) {
          std::cerr << "ERROR: " << ex << std::endl;
        }
      }
      break; }

    case idAcRemoveFile:
    case idAcRenameFile:
      if(user_data == idAcRenameFile) {
        const char * dir_s = 0;
        std::string dir_name(name.get());
	std::string::size_type pos = dir_name.find_last_of('/');

        if(pos != std::string::npos) {
          dir_name.erase(pos);
	  dir_s = dir_name.c_str();
        }

        std::string s = OksXm::ask_file(
          dlg->get_form_widget(),
	  "Input new name of file",
	  "Rename include file",
	  dir_s, "*.schema.xml"
        );

        if(s.length()) {
          try {
            dlg->file_h->rename_include_file(name.get(), s);
            dlg->refresh();
            dlg->parent()->update_schema_row(dlg->get_file());
          }
          catch (oks::exception & ex) {
            std::cerr << "ERROR: " << ex << std::endl;
          }
        }
      }
      else {
        try {
          dlg->file_h->remove_include_file(name.get());
          dlg->refresh();
          dlg->parent()->update_schema_row(dlg->get_file());
        }
        catch (oks::exception & ex) {
          std::cerr << "ERROR: " << ex << std::endl;
        }
      }

      break;
  }
}


void
OksSchemaEditorSchemaDialog::clistAC(Widget w, XtPointer client_data, XEvent *event, Boolean *)
{
  if (((XButtonEvent *)event)->button != Button3) return;
  
  OksPopupMenu popup(w);

  OksSchemaEditorSchemaDialog *dlg = (OksSchemaEditorSchemaDialog *)client_data;
  bool is_file_read_only = OksKernel::check_read_only(dlg->file_h);
  int pos = OksXm::List::get_selected_pos(w);

  if(is_file_read_only)
    popup.addDisableItem(szNew);
  else
    popup.addItem(szNew, idAcSchemaClassNew, clistCB, client_data);

  if(pos)
    popup.addItem(szShow, idAcSchemaClassShow, clistCB, client_data);
  else
    popup.addDisableItem(szShow);

  if(pos && (is_file_read_only == false))
    popup.addItem(szDelete, idAcSchemaClassDelete, clistCB, client_data);
  else
    popup.addDisableItem(szDelete);

  popup.add_separator();


  if(OksXm::List::size(w) > 1)
    popup.addItem(szSort, 0, (XtCallbackProc)OksXm::List::sortCB, (XtPointer)w);
  else
    popup.addDisableItem(szSort);

  popup.show((XButtonPressedEvent *)event);
}

void
OksSchemaEditorSchemaDialog::ilistAC(Widget w, XtPointer client_data, XEvent *event, Boolean *)
{
  if (((XButtonEvent *)event)->button != Button3) return;

  OksPopupMenu popup(w);
  OksSchemaEditorSchemaDialog * dlg = (OksSchemaEditorSchemaDialog *)client_data;
  bool is_file_read_only = OksKernel::check_read_only(dlg->file_h);
  OksXm::AutoCstring value(OksXm::List::get_selected_value(w));

  {
    std::string s;
    if(value.get()) s = get_file_path(*dlg->parent(), value.get(), dlg->file_h);

    if(!s.empty() && dlg->parent()->find_schema_file(s))
      popup.addItem("Show", idAcShowFile, ilistCB, client_data);
    else
      popup.addDisableItem("Show");
  }

  popup.add_separator();

  if(is_file_read_only)
    popup.addDisableItem("Add");
  else
    popup.addItem("Add", idAcAddFile, ilistCB, client_data);

  if(value.get())
    popup.addItem("Remove", idAcRemoveFile, ilistCB, client_data);
  else
    popup.addDisableItem("Remove");

  if(value.get() && (is_file_read_only == false))
    popup.addItem("Rename", idAcRenameFile, ilistCB, client_data);
  else
    popup.addDisableItem("Rename");

  popup.show((XButtonPressedEvent *)event);
}


void
OksSchemaEditorSchemaDialog::applyCB(Widget, XtPointer client_data, XtPointer)
{
  OksSchemaEditorSchemaDialog * dlg = (OksSchemaEditorSchemaDialog *)client_data;

  OksXm::AutoCstring new_logical_name(XmTextFieldGetString(dlg->get_widget(idLogicalName)));
  OksXm::AutoCstring new_type(XmTextFieldGetString(dlg->get_widget(idType)));

  try {
    dlg->file_h->set_logical_name(new_logical_name.get());
    dlg->file_h->set_type(new_type.get());
  }
  catch(oks::exception& ex) {
    std::cerr << "ERROR [schema dialog]: " << ex.what() << std::endl;
  }

  dlg->refresh();
  dlg->parent()->update_schema_row(dlg->file_h);
}


OksSchemaEditorSchemaDialog::OksSchemaEditorSchemaDialog(OksFile * file, OksSchemaEditorMainDialog *p) :
  OksSchemaEditorDialog	(p),
  file_h		(file)
{
  setIcon((OksDialog *)p);

  add_label(idSchema, "full name");
  add_label(idCreated, "created");
  add_label(idLastModified, "last modified");

  add_separator();

  add_text_field(idLogicalName, "Logical Name");
  add_text_field(idType, "  Type");
  attach_previous(idType);

  add_paned_window(idPanedWindow);
  attach_right(idPanedWindow);

  {
    OksForm * f = new OksForm(get_widget(idPanedWindow));

    Widget w = f->add_list(idIncludes, "Include files");
    XtAddCallback(w, XmNdefaultActionCallback, default_ilistCB, (XtPointer)this);
    XtAddEventHandler(w, ButtonPressMask, False, ilistAC, (XtPointer)this);

    f->attach_right(idIncludes);
    f->attach_bottom(idIncludes);

    get_paned_window(idPanedWindow)->add_form(f, idIncludes);
  }

  {
    OksForm * f = new OksForm(get_widget(idPanedWindow));
    Widget w = f->add_list(idClasses, szClasses);
    XtAddCallback(w, XmNdefaultActionCallback, default_clistCB, (XtPointer)this);
    XtAddEventHandler(w, ButtonPressMask, False, clistAC, (XtPointer)this);
    
    f->attach_right(idClasses);
    f->attach_bottom(idClasses);

    get_paned_window(idPanedWindow)->add_form(f, idClasses);
  }

  {
    Widget w = add_push_button(idOK, "OK");
    XtAddCallback(w, XmNactivateCallback, applyCB, (XtPointer)this);
    XtAddCallback(w, XmNactivateCallback, closeCB, (XtPointer)this);
  }

  XtAddCallback(add_push_button(idApply, "Apply"), XmNactivateCallback, applyCB, (XtPointer)this);
  attach_previous(idApply);

  XtAddCallback(add_push_button(idCancel, "Cancel"), XmNactivateCallback, closeCB, (XtPointer)this);
  attach_previous(idCancel);

  OksXm::set_close_cb(XtParent(get_form_widget()), closeCB, (void *)this);

  attach_bottom(idPanedWindow);

  change_schema_file();
  refresh();
  show();
  
  setMinSize();
  setCascadePos();
}


OksSchemaEditorSchemaDialog::~OksSchemaEditorSchemaDialog()
{
  parent()->remove_dlg(this);
}


void
OksSchemaEditorSchemaDialog::refresh()
{
    // put logical name, type and creation/modification info

  {
    size_t logical_name_width = file_h->get_logical_name().size();
    size_t type_width = file_h->get_type().size();
  
    if(logical_name_width < 5) logical_name_width = 5;
    if(type_width < 5) type_width = 5;

    OksXm::TextField::set_width(set_value(idLogicalName, file_h->get_logical_name().c_str()), logical_name_width);
    OksXm::TextField::set_width(set_value(idType, file_h->get_type().c_str()), type_width);

    set_value(idLogicalName, file_h->get_logical_name().c_str());
    set_value(idType, file_h->get_type().c_str());
  }

  {
    std::string created_at = boost::posix_time::to_simple_string(file_h->get_creation_time());
    std::string s("Created by \"");
    s += file_h->get_created_by();
    s += "\" on \"";
    s += file_h->get_created_on();
    s += "\" at \"";
    s += created_at;
    s += '\"';

    set_value(idCreated, s.c_str());
  }

  {
    std::string last_modified_at = boost::posix_time::to_simple_string(file_h->get_last_modification_time());
    std::string s("Last modified by \"");
    s += file_h->get_last_modified_by();
    s += "\" on \"";
    s += file_h->get_last_modified_on();
    s += "\" at \"";
    s += last_modified_at;
    s += '\"';

    set_value(idLastModified, s.c_str());
  }


    // refresh list of includes

  {
    OksForm * f = get_paned_window(idPanedWindow)->get_form(idIncludes);

    Widget listw = f->get_widget(idIncludes);
    OksXm::AutoCstring selected(OksXm::List::get_selected_value(listw));

    XmListDeleteAllItems(listw);

    const std::list<std::string> & ilist = file_h->get_include_files();
    for(std::list<std::string>::const_iterator i = ilist.begin(); i != ilist.end(); ++i) {
      f->set_value(idIncludes, (*i).c_str(), 0, false);
    }

    if(selected.get()) {
      OksXm::List::select_row(listw, selected.get());
    }
  }


    // refresh list of classes

  {
    OksForm * f = get_paned_window(idPanedWindow)->get_form(idClasses);

    Widget listw = f->get_widget(idClasses);
    OksXm::AutoCstring selectedStr(OksXm::List::get_selected_value(listw));

    XmListDeleteAllItems(listw);

    if(std::list<OksClass *> * classes = parent()->create_list_of_schema_classes(file_h)) {
      for(std::list<OksClass *>::iterator i = classes->begin(); i != classes->end(); ++i)
        f->set_value(idClasses, (*i)->get_name().c_str(), 0, false);

      delete classes;
    }

    if(selectedStr.get()) {
      OksXm::List::select_row(listw, selectedStr.get());
    }
  }
}

void
OksSchemaEditorSchemaDialog::remove_class(const OksClass * c)
{
  Widget w = get_paned_window(idPanedWindow)->get_form(idClasses)->get_widget(idClasses);
  OksXm::List::delete_row(w, (char *)(c->get_name().c_str()));
}

bool
OksSchemaEditorSchemaDialog::is_read_only() const
{
  return file_h->is_read_only();
}


void
OksSchemaEditorSchemaDialog::change_schema_file()
{
    // set title

  {
    std::string s(file_h->get_full_file_name());
    if(is_read_only()) s += " (read-only)";
    setTitle("Oks Schema", s.c_str());
  }

    // set name in the header

  {
    std::string s("Full name: \"");
    s += file_h->get_full_file_name();
    s += '\"';
    set_value(idSchema, s.c_str());
  }
}
