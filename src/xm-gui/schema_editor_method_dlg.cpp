#include <stdlib.h>
#include <stdio.h>

#include <oks/method.h>
#include <oks/class.h>
#include <oks/xm_utils.h>
#include <oks/xm_popup.h>

#include <Xm/Xm.h>
#include <Xm/List.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/RowColumn.h>
#include <Xm/SelectioB.h>

#include "schema_editor_oks_method.xpm"

#include "schema_editor_method_dlg.h"
#include "schema_editor_main_dlg.h"
#include "schema_editor.h"


OksSchemaEditorMethodImplementationDialog::OksSchemaEditorMethodImplementationDialog(
    OksMethodImplementation * implementation,
    OksMethod * method,
    OksClass * cl,
    OksSchemaEditorMainDialog * p
) :
  OksSchemaEditorDialog	(p, cl),
  p_implementation	(implementation),
  p_parent_method       (method)
{
  static Pixmap	pixmap = 0;
 
  if(!pixmap)
    pixmap = OksXm::create_color_pixmap(get_form_widget(), schema_editor_oks_method_xpm);

  setIcon(pixmap);
 
  createOksDlgHeader(&pixmap, 0, 0, idLanguage, "Language", idDefinedInMethod, "Defined in method");
  set_value(idLanguage, p_implementation->get_language().c_str());
  set_value(idDefinedInMethod, p_parent_method->get_name().c_str());
  OksXm::TextField::set_not_editable(get_widget(idDefinedInMethod));

  add_text_field(idPrototype, "Prototype");
  attach_right(idPrototype);
  set_value(idPrototype, p_implementation->get_prototype().c_str());

  add_text(idBody, "Body");
  attach_right(idBody);
  set_value(idBody, p_implementation->get_body().c_str());
  

  add_separator();

  XtAddCallback(add_push_button(idApply, "Apply"), XmNactivateCallback, applyCB, (XtPointer)this);
  XtAddCallback(add_push_button(idClose, "Close"), XmNactivateCallback, closeCB, (XtPointer)this);
  attach_previous(idClose);
  OksXm::set_close_cb(XtParent(get_form_widget()), closeCB, (void *)this);

  attach_bottom(idBody);

  change_schema_file();
  show();

  setMinSize();
  setCascadePos();
}

OksSchemaEditorMethodImplementationDialog::~OksSchemaEditorMethodImplementationDialog()
{
  update();
  parent()->remove_dlg(this);
}

void
OksSchemaEditorMethodImplementationDialog::update()
{
  try {
    OksXm::AutoCstring language(XmTextGetString(get_widget(idLanguage)));
    OksXm::AutoCstring prototype(XmTextGetString(get_widget(idPrototype)));
    OksXm::AutoCstring body(XmTextGetString(get_widget(idBody)));

    p_implementation->set_language(language.get());
    p_implementation->set_prototype(prototype.get());
    p_implementation->set_body(body.get());
  }
  catch(oks::exception& ex) {
    Oks::error_msg("OksSchemaEditorMethodImplementationDialog::update()") << ex.what() << std::endl;
  }
}

void
OksSchemaEditorMethodImplementationDialog::change_schema_file()
{
    // set title

  {
    std::string s(p_parent_method->get_name());
    s += "() on ";
    s +=p_implementation->get_language();
    if(is_read_only()) s += " (read-only)";
    setTitle("Oks Method Implementation", get_class()->get_name().c_str(), s.c_str());
  }


    // set access to the method-implementation properties

  if(is_read_only()) {
    OksXm::TextField::set_not_editable(get_widget(idLanguage));
    OksXm::TextField::set_not_editable(get_widget(idPrototype));
    OksXm::TextField::set_not_editable(get_widget(idBody));
  }
  else {
    OksXm::TextField::set_editable(get_widget(idLanguage));
    OksXm::TextField::set_editable(get_widget(idPrototype));
    OksXm::TextField::set_editable(get_widget(idBody));
  }
}

void
OksSchemaEditorMethodImplementationDialog::applyCB(Widget, XtPointer client_data, XtPointer)
{
  OksSchemaEditorMethodImplementationDialog * dlg = (OksSchemaEditorMethodImplementationDialog *)client_data;
  dlg->update();
}


////////////////////////////////////////////////////////////////////////////////

void
OksSchemaEditorMethodDialog::implementationCB(Widget w, XtPointer client_data, XtPointer)
{
  OksSchemaEditorMethodDialog * dlg = (OksSchemaEditorMethodDialog *)client_data;
  OksXm::AutoCstring item(OksXm::List::get_selected_value(dlg->get_widget(idImplementations)));
  long user_data = (long)OksXm::get_user_data(w);
  OksPopupMenu::destroy(w);

  switch (user_data) {
    case idAcMethodAddImplementation: {
      std::string s = OksXm::ask_name(
        dlg->get_form_widget(), "Input implementation language", "Add Method Implementation"
      );
    
      if(s.length()) {
        try {
          dlg->get_method()->add_implementation(s, " ", "");
          dlg->show_implementations();
        }
        catch(oks::exception& ex) {
          Oks::error_msg("OksSchemaEditorMethodDialog::implementationCB()") << ex.what() << std::endl;
        }

      }

      break; }

    case idAcMethodModifyImplementation:
      if(item.is_non_empty()) {
        OksMethodImplementation * i = dlg->get_method()->find_implementation(item.get());
	if(i) {
          dlg->parent()->create_method_implementation_dlg(i, dlg->get_method(), dlg->get_class());
	}
      }

      break;

    case idAcMethodRemoveImplementation: {
      if(item.is_non_empty()) {
        try {
	  dlg->get_method()->remove_implementation(item.get());
          dlg->show_implementations();
	}
        catch(oks::exception& ex) {
          Oks::error_msg("OksSchemaEditorMethodDialog::implementationCB()") << ex.what() << std::endl;
        }

      }

      break;
    }
  }
}


void
OksSchemaEditorMethodDialog::implementationAC(Widget w, XtPointer client_data, XEvent *event, Boolean *)
{
  if(((XButtonEvent *)event)->button != Button3) return;

  OksSchemaEditorMethodDialog * dlg = (OksSchemaEditorMethodDialog *)client_data;

  OksPopupMenu popup(w);

  if(dlg->is_read_only())
    popup.addDisableItem("Add Implementation");
  else
    popup.addItem("Add Implementation", idAcMethodAddImplementation, implementationCB, client_data);

  if(OksXm::List::get_selected_pos(w) && !dlg->is_read_only()) {
    OksXm::AutoCstring actionName(OksXm::List::get_selected_value(w));
    popup.addItem("Modify Implementation", actionName.get(), idAcMethodModifyImplementation, implementationCB, client_data);
    popup.add_separator();
    popup.addItem("Remove Implementation", actionName.get(), idAcMethodRemoveImplementation, implementationCB, client_data);
  }
  else {
    popup.addDisableItem("Modify Implementation");
    popup.add_separator();
    popup.addDisableItem("Remove Implementation");
  }

  popup.show((XButtonPressedEvent *)event);
}

//////

void
OksSchemaEditorMethodDialog::showCB(Widget, XtPointer client_data, XtPointer call_data)
{
  OksSchemaEditorMethodDialog * dlg = (OksSchemaEditorMethodDialog *)client_data;
  OksXm::AutoCstring item(OksXm::string_to_c_str(((XmListCallbackStruct *)call_data)->item));  

  if(OksMethodImplementation * i = dlg->get_method()->find_implementation(item.get())) {
    dlg->parent()->create_method_implementation_dlg(i, dlg->get_method(), dlg->get_class());
  }
}

void
OksSchemaEditorMethodDialog::iconCB(Widget w, XtPointer client_data, XtPointer)
{
  long user_choice = (long)OksXm::get_user_data(w);

  OksPopupMenu::destroy(w);

  OksSchemaEditorMethodDialog * dlg = (OksSchemaEditorMethodDialog *)client_data;
  OksClass * c = dlg->get_class();
  OksMethod * m = dlg->get_method();

  try {
    switch(user_choice) {
      case idUpdate:
        dlg->update();
        break;

      case idRename:
        m->set_name(
          OksXm::ask_name(
            dlg->parent()->get_form_widget(), "Input new name of the method", "Rename OKS Method", m->get_name().c_str()
          )
        );
        dlg->set_value(idName, m->get_name().c_str());
        break;

      case idDelete:
        delete dlg;
        c->remove(m);
        break;
    }
  }
  catch(oks::exception& ex) {
    Oks::error_msg("OksSchemaEditorMethodDialog::iconCB()") << ex.what() << std::endl;
  }
}


void
OksSchemaEditorMethodDialog::iconAC(Widget w, XtPointer client_data, XEvent *event, Boolean *)
{
  make_popup(
    w, client_data, event,
    (((OksSchemaEditorMethodDialog *)client_data)->is_read_only()),
    iconCB
  );
}


OksSchemaEditorMethodDialog::OksSchemaEditorMethodDialog(OksMethod *mt, OksClass *cl, OksSchemaEditorMainDialog *p) :
  OksSchemaEditorDialog	(p, cl),
  m			(mt)
{
  static Pixmap	pixmap = 0;
 
  if(!pixmap)
    pixmap = OksXm::create_color_pixmap(get_form_widget(), schema_editor_oks_method_xpm);

  setIcon(pixmap);
 
  createOksDlgHeader(&pixmap, iconAC, (void *)this, idName, szName, idDefinedInClass, szDefinedInClass);
  set_value(idName, m->get_name().c_str());
  set_value(idDefinedInClass, get_class()->get_name().c_str());
  OksXm::TextField::set_not_editable(get_widget(idName));
  OksXm::TextField::set_not_editable(get_widget(idDefinedInClass));

  Widget w = add_list(idImplementations, "Implementations");
  XtAddEventHandler(w, ButtonPressMask, False, implementationAC, (XtPointer)this);
  XtAddCallback(w, XmNdefaultActionCallback, showCB, (XtPointer)this);
  attach_right(idImplementations);
  show_implementations();

  add_separator();

  add_text(idDescription, szDescription);
  attach_right(idDescription);
  set_value(idDescription, m->get_description().c_str());

  add_separator();

  XtAddCallback(addCloseButton("OK"), XmNactivateCallback, closeCB, (XtPointer)this);
  OksXm::set_close_cb(XtParent(get_form_widget()), closeCB, (void *)this);

  attach_bottom(idImplementations);

  change_schema_file();
  show();

  setMinSize();
  setCascadePos();
}


OksSchemaEditorMethodDialog::~OksSchemaEditorMethodDialog()
{
  update();
  parent()->remove_dlg(this);
}


void
OksSchemaEditorMethodDialog::show_implementations()
{
  Widget w = get_widget(idImplementations);
  OksXm::AutoCstring item(OksXm::List::get_selected_value(w));

  XmListDeleteAllItems(w);

  if(m->implementations()) {
    for(std::list<OksMethodImplementation *>::const_iterator i = m->implementations()->begin(); i != m->implementations()->end(); ++i) {
      set_value(idImplementations, (*i)->get_language().c_str(), 0, false);
    }
  }

  if(item.get()) {
    OksXm::List::select_row(w, item.get());
  }
}

void
OksSchemaEditorMethodDialog::update()
{
  try {
    OksXm::AutoCstring description(XmTextGetString(get_widget(idDescription)));
    get_method()->set_description(description.get());
  }
  catch(oks::exception& ex) {
    Oks::error_msg("OksSchemaEditorMethodDialog::update()") << ex.what() << std::endl;
  }
}


void
OksSchemaEditorMethodDialog::change_schema_file()
{
    // set title

  {
    std::string s(m->get_name());
    if(is_read_only()) s += " (read-only)";
    setTitle("Oks Method", get_class()->get_name().c_str(), s.c_str());
  }


    // set access to the method properties

  if(is_read_only()) {
    OksXm::TextField::set_not_editable(get_widget(idDescription));
  }
  else {
    OksXm::TextField::set_editable(get_widget(idDescription));
  }
}
