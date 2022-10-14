#include <oks/class.h>
#include <oks/attribute.h>
#include <oks/relationship.h>
#include <oks/method.h>
#include <oks/xm_utils.h>
#include <oks/xm_popup.h>

#include <Xm/Xm.h>
#include <Xm/List.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/RowColumn.h>
#include <Xm/SelectioB.h>

#include "schema_editor_oks_class.xpm"

#include "schema_editor_class_dlg.h"
#include "schema_editor_attribute_dlg.h"
#include "schema_editor_relationship_dlg.h"
#include "schema_editor_method_dlg.h"
#include "schema_editor_schema_dlg.h"
#include "schema_editor_main_dlg.h"
#include "schema_editor.h"


void
OksSchemaEditorClassDialog::addSuperclassCB(Widget w, XtPointer client_data, XtPointer)
{
  try {
    OksXm::AutoCstring value(OksXm::List::get_selected_value(w));
    ((OksSchemaEditorClassDialog *)client_data)->get_class()->add_super_class(value.get());
  }
  catch(oks::exception& ex) {
    Oks::error_msg("OksSchemaEditorClassDialog::addSuperclassCB()") << ex.what() << std::endl;
  }

  OksSchemaEditorMainDialog::destroy_select_class_dlg(w);
}


bool
get_names_for_swap(std::string& s1, std::string& s2, bool up, OksPanedWindow * paned_window, short id)
{
  Widget w = paned_window->get_form(id)->get_widget(id);
  OksXm::AutoCstring name1(OksXm::List::get_selected_value(w));
  OksXm::AutoCstring name2(OksXm::List::get_value(w, OksXm::List::get_selected_pos(w) + (up ? -1 : 1)));

  if(name1.get() != 0 && name2.get() != 0) {
    s1 = name1.get();
    s2 = name2.get();
    return true;
  }

  return false;
}


void
OksSchemaEditorClassDialog::actionCB(Widget w, XtPointer client_data, XtPointer)
{
  long user_choice = (long)OksXm::get_user_data(w);
  OksPopupMenu::destroy(w);

  OksSchemaEditorClassDialog * dlg = (OksSchemaEditorClassDialog *)client_data;
  OksSchemaEditorMainDialog * dlg_parent = dlg->parent();
  OksClass * dlg_class = dlg->get_class();
  OksPanedWindow * paned_window = dlg->get_paned_window(idClassPanedWindow);

  try {
    switch (user_choice) {
      case idAcClassDelete:
        OksClass::destroy(dlg_class);
        break;

      case idAcClassChangeSchemaFile: {
        OksFile * old_file_h = dlg_class->get_file();
        OksFile * new_file_h = dlg_parent->get_active_schema();
        if(new_file_h && old_file_h != new_file_h) {
          dlg_class->set_file(new_file_h);
          dlg->set_value(idSchema, new_file_h->get_full_file_name().c_str());
	  dlg_parent->add_class_to_schema_dlg(new_file_h);
	  dlg_parent->remove_class_from_schema_dlg(dlg_class, old_file_h);
        }
        break;
      }

      case idAcClassSuperClassShowDirect:
        dlg->showAllSuperClasses = false;
        dlg->show_superclasses();
        break;

      case idAcClassSuperClassShowAll:
        dlg->showAllSuperClasses = true;
        dlg->show_superclasses();
        break;

      case idAcClassSuperClassAdd: {

          // prepare exclude list containg class superclasses and subclasses

        std::list<OksClass *> e_list;
        e_list.push_back(dlg_class);

        if(dlg_class->all_super_classes()) {
          e_list.insert(e_list.begin(), dlg_class->all_super_classes()->begin(), dlg_class->all_super_classes()->end());
        }

        if(dlg_class->all_sub_classes()) {
          e_list.insert(e_list.begin(), dlg_class->all_sub_classes()->begin(), dlg_class->all_sub_classes()->end());
        }

        dlg_parent->create_select_class_dlg("Choose Superclass", &e_list, addSuperclassCB, client_data);
        break; }

        case idAcClassSuperClassShow: {
          OksXm::AutoCstring className(OksXm::List::get_selected_value(paned_window->get_form(idSuperClasses)->get_widget(idSuperClasses)));
          if(OksClass * c = dlg_parent->find_class(className.get())) dlg_parent->create_class_dlg(c);
          break;
        }

      case idAcClassSuperClassRemove: {
        OksXm::AutoCstring className(OksXm::List::get_selected_value(paned_window->get_form(idSuperClasses)->get_widget(idSuperClasses)));
        dlg_class->remove_super_class(className.get());
        break; }

      case idAcClassSuperClassMoveUp:
      case idAcClassSuperClassMoveDown: {
        std::string name1, name2;
        if(
          get_names_for_swap(
            name1, name2,
            user_choice == idAcClassSuperClassMoveUp, 
            paned_window, idSuperClasses
          )
        ) {
          dlg_class->swap_super_classes(name1, name2);
        }
        break; }

      case idAcClassAttributeShowDirect:
        dlg->showAllAttributes = false;
        dlg->show_attributes();
        break;

      case idAcClassAttributeShowAll:
        dlg->showAllAttributes = true;
        dlg->show_attributes();
        break;

      case idAcClassAttributeAdd: {
        std::string s = OksXm::ask_name(
          dlg->get_form_widget(),
          "Input name of new attribute",
          "New OKS Attribute"
        );

        if(s.length())
          {
            try
              {
                OksAttribute *a = new OksAttribute(s.c_str());
                dlg_class->add(a);
                dlg_parent->create_attribute_dlg(a, dlg_class);
              }
            catch(std::exception& ex)
              {
                Oks::error_msg("Add new attribute") << ex.what() << std::endl;
              }
          }

        break; }

      case idAcClassAttributeShow: {
        OksXm::AutoCstring attributeName(OksXm::List::get_selected_value(paned_window->get_form(idAttributes)->get_widget(idAttributes)));
        if(OksAttribute * a = dlg_class->find_attribute(attributeName.get())) dlg_parent->create_attribute_dlg(a, dlg_class->source_class(a));
        break; }

      case idAcClassAttributeRemove: {
        OksXm::AutoCstring attributeName(OksXm::List::get_selected_value(paned_window->get_form(idAttributes)->get_widget(idAttributes)));
        OksAttribute * a = dlg_class->find_attribute(attributeName.get());
        if(OksSchemaEditorAttributeDialog * osead = dlg_parent->find_attribute_dlg(a, dlg_class)) delete osead;
        dlg_class->remove(a);
        break; }

      case idAcClassAttributeMoveUp:
      case idAcClassAttributeMoveDown: {
        std::string name1, name2;
        if(
          get_names_for_swap(
            name1, name2,
            user_choice == idAcClassAttributeMoveUp, 
            paned_window, idAttributes
          )
        ) {
          dlg_class->swap(
            dlg_class->find_direct_attribute(name1),
            dlg_class->find_direct_attribute(name2)
          );
        }
        break; }

      case idAcClassRelationshipShowDirect:
        dlg->showAllRelationships = false;
        dlg->show_relationships();
        break;

      case idAcClassRelationshipShowAll:
        dlg->showAllRelationships = true;
        dlg->show_relationships();
        break;

      case idAcClassRelationshipAdd: {
        std::string s = OksXm::ask_name(
          dlg->get_form_widget(),
          "Input name of new relationship",
          "New OKS Relationship"
        );

        if(s.length())
          {
            try
              {
                OksRelationship *r = new OksRelationship(s.c_str());
                dlg_class->add(r);
                dlg_parent->create_relationship_dlg(r, dlg_class);
              }
            catch(std::exception& ex)
              {
                Oks::error_msg("Add new relationship") << ex.what() << std::endl;
              }
          }

        break; }

      case idAcClassRelationshipShow: {
        OksXm::AutoCstring relationshipName(OksXm::List::get_selected_value(paned_window->get_form(idRelationships)->get_widget(idRelationships)));
        if(OksRelationship * r = dlg_class->find_relationship(relationshipName.get())) dlg_parent->create_relationship_dlg(r, dlg_class->source_class(r));
        break; }

      case idAcClassRelationshipRemove: {
        OksXm::AutoCstring relationshipName(OksXm::List::get_selected_value(paned_window->get_form(idRelationships)->get_widget(idRelationships)));
        OksRelationship * r = dlg_class->find_relationship(relationshipName.get());
        if(OksSchemaEditorRelationshipDialog * osead = dlg_parent->find_relationship_dlg(r, dlg_class)) delete osead;
        dlg_class->remove(r);
        break; }

      case idAcClassRelationshipMoveUp:
      case idAcClassRelationshipMoveDown: {
        std::string name1, name2;
        if(
          get_names_for_swap(
	    name1, name2,
	    user_choice == idAcClassRelationshipMoveUp, 
	    paned_window, idRelationships
	  )
        ) {
          dlg_class->swap(
            dlg_class->find_direct_relationship(name1),
            dlg_class->find_direct_relationship(name2)
          );
        }
      break; }

      case idAcClassMethodShowDirect:
        dlg->showAllMethods = false;
        dlg->show_methods();
        break;

      case idAcClassMethodShowAll:
        dlg->showAllMethods = true;
        dlg->show_methods();
        break;

      case idAcClassMethodAdd: {
        std::string s = OksXm::ask_name(
          dlg->get_form_widget(),
          "Input name of new method",
          "New OKS Method"
        );

        if(s.length())
          {
            try
              {
                OksMethod * m = new OksMethod(s.c_str());
                dlg_class->add(m);
                dlg_parent->create_method_dlg(m, dlg_class);
              }
            catch(std::exception& ex)
              {
                Oks::error_msg("Add new method") << ex.what() << std::endl;
              }
          }

        break; }

      case idAcClassMethodShow: {
        OksXm::AutoCstring methodName(OksXm::List::get_selected_value(paned_window->get_form(idMethods)->get_widget(idMethods)));
        if(OksMethod * m = dlg_class->find_method(methodName.get())) dlg_parent->create_method_dlg(m, dlg_class->source_class(m));
        break;
      }

      case idAcClassMethodRemove: {
        OksXm::AutoCstring methodName(OksXm::List::get_selected_value(paned_window->get_form(idMethods)->get_widget(idMethods)));
        OksMethod * m = dlg_class->find_method(methodName.get());
        if(OksSchemaEditorMethodDialog * osead = dlg_parent->find_method_dlg(m, dlg_class)) delete osead;
        dlg_class->remove(m);
        break; }

      case idAcClassMethodMoveUp:
      case idAcClassMethodMoveDown: {
        std::string name1, name2;
        if(
          get_names_for_swap(
  	    name1, name2,
	    user_choice == idAcClassMethodMoveUp, 
	    paned_window, idMethods
  	  )
        ) {
          dlg_class->swap(
            dlg_class->find_direct_method(name1),
            dlg_class->find_direct_method(name2)
          );
        }
        break; }
    }
  }
  catch(oks::exception& ex) {
    Oks::error_msg("OksSchemaEditorClassDialog::actionCB()") << ex.what() << std::endl;
  }
}


void
OksSchemaEditorClassDialog::dlgAC(Widget w, XtPointer client_data, XEvent *event, Boolean *)
{
  if (((XButtonEvent *)event)->button != Button3) return;

  OksSchemaEditorClassDialog *dlg = (OksSchemaEditorClassDialog *)client_data;

  OksPopupMenu popup(w);

  OksFile * f1 = dlg->parent()->get_active_schema();
  OksFile * f2 = dlg->get_class()->get_file();
  const char * btn_txt = (f1 ? f1->get_full_file_name().c_str() : "");
  Widget btn = popup.addItem("Move to: ", btn_txt, idAcClassChangeSchemaFile, actionCB, client_data);
  bool is_read_only = OksKernel::check_read_only(f2);

  if(f1 == f2 || is_read_only == true || !OksSchemaEditorMainDialog::is_valid_active_schema()) XtSetSensitive(btn, false);

  popup.add_separator();

  if(is_read_only == false)
    popup.addItem(szDelete, idAcClassDelete, actionCB, client_data);
  else
    popup.addDisableItem(szDelete);

  popup.show((XButtonPressedEvent *)event);
}


bool
OksSchemaEditorClassDialog::is_read_only() const
{
  return get_class()->get_file()->is_read_only();
}


void
OksSchemaEditorClassDialog::create_popup(
  Widget w, XEvent *event,
  bool show_all_params, const char * param_type,
  int show_direct_id, int show_all_id,
  int add_id, int show_id, int remove_id,
  int move_up_id, int move_down_id
)
{
  if (((XButtonEvent *)event)->button != Button3) return;

  std::string show_what("Show ");
  show_what += (show_all_params ? "direct " : "all ");
  show_what += param_type;
  if(show_what[show_what.length()-1] == 's') show_what += 'e';
  show_what += 's';

  std::string add("Add ");
  add += param_type;

  std::string show("Show ");
  show += param_type;
  show += ' ';

  std::string remove("Remove ");  
  remove += param_type;
  remove += ' ';
  
  OksPopupMenu popup(w);

  if(show_all_params) popup.addItem(show_what.c_str(), show_direct_id, actionCB, (XtPointer)this);
  else popup.addItem(show_what.c_str(), show_all_id, actionCB, (XtPointer)this);
  
  if(is_read_only()) popup.addDisableItem(add.c_str());
  else popup.addItem(add.c_str(), add_id, actionCB, (XtPointer)this);

  unsigned int pos = OksXm::List::get_selected_pos(w);

  if(pos) {
    unsigned int total = OksXm::List::size(w);
    OksXm::AutoCstring name(OksXm::List::get_selected_value(w));

    w = popup.addItem(show.c_str(), name.get(), show_id, actionCB, (XtPointer)this);
    if(!strcmp(param_type, "superclass") && !parent()->find_class(name.get())) XtSetSensitive(w, false);

    popup.add_separator();

    OksClass *c = get_class();

    bool is_direct = false;
    unsigned int num_of_direct = 0;

    if(!strcmp(param_type, "attribute")) {
      is_direct = (c->source_class(c->find_attribute(name.get())) == c);
      num_of_direct = (c->direct_attributes() ? c->direct_attributes()->size() : 0);
    }
    else if(!strcmp(param_type, "relationship")){
      is_direct = (c->source_class(c->find_relationship(name.get())) == c);
      num_of_direct = (c->direct_relationships() ? c->direct_relationships()->size() : 0);
    }
    else if(!strcmp(param_type, "method")){
      is_direct = (c->source_class(c->find_method(name.get())) == c);
      num_of_direct = (c->direct_methods() ? c->direct_methods()->size() : 0);
    }
    else if(!strcmp(param_type, "superclass")) {
      is_direct = c->has_direct_super_class(name.get());
      num_of_direct = (c->direct_super_classes() ? c->direct_super_classes()->size() : 0);
    }

    if(is_direct && !is_read_only())
      popup.addItem(remove.c_str(), name.get(), remove_id, actionCB, (XtPointer)this);
    else
      popup.addDisableItem(remove.c_str());

    popup.add_separator();

    if(is_direct && !is_read_only() && total > 1 && pos > (total - num_of_direct + 1))
      popup.addItem("Move Up", move_up_id, actionCB, (XtPointer)this);
    else
      popup.addDisableItem("Move Up");

    if(is_direct && !is_read_only() && total > 1 && pos != total)
      popup.addItem("Move Down", move_down_id, actionCB, (XtPointer)this);
    else
      popup.addDisableItem("Move Down");
 }
  else {
    popup.addDisableItem(show.c_str());
    popup.add_separator();
    popup.addDisableItem(remove.c_str());
    popup.add_separator();
    popup.addDisableItem("Move Up");
    popup.addDisableItem("Move Down");
  }

  popup.show((XButtonPressedEvent *)event);
}


void
OksSchemaEditorClassDialog::superclassesAC(Widget w, XtPointer client_data, XEvent *event, Boolean *)
{
  OksSchemaEditorClassDialog * dlg = (OksSchemaEditorClassDialog *)client_data;

  dlg->create_popup(
    w, event, dlg->showAllSuperClasses, "superclass",
    idAcClassSuperClassShowDirect,
    idAcClassSuperClassShowAll,
    idAcClassSuperClassAdd,
    idAcClassSuperClassShow,
    idAcClassSuperClassRemove,
    idAcClassSuperClassMoveUp,
    idAcClassSuperClassMoveDown
  );
}


void
OksSchemaEditorClassDialog::attributesAC(Widget w, XtPointer client_data, XEvent *event, Boolean *)
{
  OksSchemaEditorClassDialog * dlg = (OksSchemaEditorClassDialog *)client_data;

  dlg->create_popup(
    w, event, dlg->showAllAttributes, "attribute",
    idAcClassAttributeShowDirect,
    idAcClassAttributeShowAll,
    idAcClassAttributeAdd,
    idAcClassAttributeShow,
    idAcClassAttributeRemove,
    idAcClassAttributeMoveUp,
    idAcClassAttributeMoveDown
  );
}


void
OksSchemaEditorClassDialog::relationshipsAC(Widget w, XtPointer client_data, XEvent *event, Boolean *)
{
  OksSchemaEditorClassDialog * dlg = (OksSchemaEditorClassDialog *)client_data;

  dlg->create_popup(
    w, event, dlg->showAllRelationships, "relationship",
    idAcClassRelationshipShowDirect,
    idAcClassRelationshipShowAll,
    idAcClassRelationshipAdd,
    idAcClassRelationshipShow,
    idAcClassRelationshipRemove,
    idAcClassRelationshipMoveUp,
    idAcClassRelationshipMoveDown
  );
}


void
OksSchemaEditorClassDialog::methodsAC(Widget w, XtPointer client_data, XEvent *event, Boolean *)
{
  OksSchemaEditorClassDialog * dlg = (OksSchemaEditorClassDialog *)client_data;

  dlg->create_popup(
    w, event, dlg->showAllMethods, "method",
    idAcClassMethodShowDirect,
    idAcClassMethodShowAll,
    idAcClassMethodAdd,
    idAcClassMethodShow,
    idAcClassMethodRemove,
    idAcClassMethodMoveUp,
    idAcClassMethodMoveDown
  );
}


void
OksSchemaEditorClassDialog::classesCB(Widget, XtPointer client_data, XtPointer call_data)
{
  OksSchemaEditorClassDialog * dlg = (OksSchemaEditorClassDialog *)client_data;
  OksSchemaEditorMainDialog * dlg_parent = dlg->parent();
  OksXm::AutoCstring name(OksXm::string_to_c_str(((XmListCallbackStruct *)call_data)->item));
  if(OksClass * c = dlg_parent->find_class(name.get())) dlg_parent->create_class_dlg(c);
}


void
OksSchemaEditorClassDialog::attributesCB(Widget, XtPointer client_data, XtPointer call_data)
{
  OksSchemaEditorClassDialog * dlg = (OksSchemaEditorClassDialog *)client_data;
  OksClass * c = dlg->get_class();
  OksXm::AutoCstring item(OksXm::string_to_c_str(((XmListCallbackStruct *)call_data)->item));  
  if(OksAttribute * a = c->find_attribute(item.get())) dlg->parent()->create_attribute_dlg(a, c->source_class(a));
}


void
OksSchemaEditorClassDialog::relationshipsCB(Widget, XtPointer client_data, XtPointer call_data)
{
  OksSchemaEditorClassDialog * dlg = (OksSchemaEditorClassDialog *)client_data;
  OksClass * c = dlg->get_class();
  OksXm::AutoCstring item(OksXm::string_to_c_str(((XmListCallbackStruct *)call_data)->item));
  if(OksRelationship * r = c->find_relationship(item.get())) dlg->parent()->create_relationship_dlg(r, c->source_class(r));
}


void
OksSchemaEditorClassDialog::methodsCB(Widget, XtPointer client_data, XtPointer call_data)
{
  OksSchemaEditorClassDialog * dlg = (OksSchemaEditorClassDialog *)client_data;
  OksClass * c = dlg->get_class();
  OksXm::AutoCstring item(OksXm::string_to_c_str(((XmListCallbackStruct *)call_data)->item));
  if(OksMethod * m = c->find_method(item.get())) dlg->parent()->create_method_dlg(m, c->source_class(m));
}


void
OksSchemaEditorClassDialog::add_paned_list(short id, const char *name, XtCallbackProc cb, XtEventHandler ac)
{
  OksForm * f = new OksForm(get_widget(idClassPanedWindow));
  Widget w = f->add_list(id, name);

  f->attach_right(id);
  f->attach_bottom(id);

  get_paned_window(idClassPanedWindow)->add_form(f, id);

  if(cb) XtAddCallback(w, XmNdefaultActionCallback, cb, (XtPointer)this);
  if(ac) XtAddEventHandler(w, ButtonPressMask, False, ac, (XtPointer)this);
}


OksSchemaEditorClassDialog::OksSchemaEditorClassDialog(OksClass *cl, OksSchemaEditorMainDialog *p) :
  OksSchemaEditorDialog	(p, cl),
  showAllSuperClasses	(true),
  showAllAttributes	(true),
  showAllRelationships	(true),
  showAllMethods	(true)
{
  static Pixmap	pixmap = 0;

  if(!pixmap)
    pixmap = OksXm::create_color_pixmap(get_form_widget(), schema_editor_oks_class_xpm);

  createOksDlgHeader(&pixmap, dlgAC, (void *)this, idClass, szClass, idSchema, szSchema);

  setIcon(pixmap);

  OksXm::TextField::set_not_editable(get_widget(idClass));
  OksXm::TextField::set_not_editable(get_widget(idSchema));

  add_paned_window(idClassPanedWindow);
  attach_right(idClassPanedWindow);

  add_paned_list(idSuperClasses, szSuperClasses, classesCB, superclassesAC);
  add_paned_list(idSubClasses, szSubClasses, classesCB, 0);
  add_paned_list(idAttributes, szAttributes, attributesCB, attributesAC);
  add_paned_list(idRelationships, szRelationships, relationshipsCB, relationshipsAC);
  add_paned_list(idMethods, szMethods, methodsCB, methodsAC);


    // make panel with class properties

  {
    OksForm *f = new OksForm(get_widget(idClassPanedWindow));


      // add abstract menu

    std::list<const char *> isAbstratctList;
    isAbstratctList.push_back(szYes);
    isAbstratctList.push_back(szNo);
    isAbstratctList.push_back(szIsAbstract);

    f->add_option_menu(idIsAbstract, &isAbstratctList);


      // add description text

    f->add_text(idDescription, szDescription);
    f->attach_right(idDescription);
    f->attach_bottom(idDescription);


      // add panel to the class window

    get_paned_window(idClassPanedWindow)->add_form(f, idAcClassProperties);
  }


    // add "Close" button

  add_separator();

  XtAddCallback(addCloseButton(), XmNactivateCallback, closeCB, (XtPointer)this);
  OksXm::set_close_cb(XtParent(get_form_widget()), closeCB, (void *)this);
  
  attach_bottom(idClassPanedWindow);

  refresh();
  show();

  setCascadePos();
}


OksSchemaEditorClassDialog::~OksSchemaEditorClassDialog()
{
  update();
  parent()->remove_dlg(this);
}


    //
    // The constructor of the UpdatePanedList struct does the following:
    //  - finds paned window and it's list
    //  - remembers list selectet item
    //  - clean the list
    //  - set label for new items shown in list
    //
    // The destructor select the list item
    //

struct UpdatePanedList
{
  UpdatePanedList(OksForm *, const char *, bool, int, int);
  ~UpdatePanedList();

  OksForm * form;
  Widget list_w;
  OksXm::AutoCstring item;
};


UpdatePanedList::UpdatePanedList(
  OksForm * pf, const char * what, bool show_all,
  int paned_window_id, int list_id
) :
  form   (pf->get_paned_window(paned_window_id)->get_form(list_id)),
  list_w (form->get_widget(list_id)),
  item   (OksXm::List::get_selected_value(list_w))
{
  if(what != 0) {
    std::string s = (show_all ? "All " : "Direct ");
    s += what;
    s += ':';
    OksXm::Label::set(form->get_previous(list_id), s.c_str());
  }
  
  XmListDeleteAllItems(list_w);
}

UpdatePanedList::~UpdatePanedList()
{
  if(item.get()) {
    OksXm::List::select_row(list_w, item.get());
  }
}


void
OksSchemaEditorClassDialog::show_superclasses()
{
  UpdatePanedList ul(
    this, "Super Classes", showAllSuperClasses,
    idClassPanedWindow, idSuperClasses
  );
  
  if(showAllSuperClasses) {
    if(const OksClass::FList * slist = get_class()->all_super_classes()) {
      for(OksClass::FList::const_iterator i = slist->begin(); i != slist->end(); ++i)
        OksXm::List::add_row(ul.list_w, (*i)->get_name().c_str());
    }
  }
  else {
    const std::list<std::string *> * slist = get_class()->direct_super_classes();
    if(slist) {
      for(std::list<std::string *>::const_iterator i = slist->begin(); i != slist->end(); ++i)
        OksXm::List::add_row(ul.list_w, (*i)->c_str());
    }
  }
}

void
OksSchemaEditorClassDialog::show_subclasses()
{
  UpdatePanedList ul(
    this, 0, true,
    idClassPanedWindow, idSubClasses
  );

  if(const OksClass::FList * slist = get_class()->all_sub_classes()) {
    for(OksClass::FList::const_iterator i = slist->begin(); i != slist->end(); ++i)
      OksXm::List::add_row(ul.list_w, (*i)->get_name().c_str());
  }
}

void
OksSchemaEditorClassDialog::show_attributes()
{
  UpdatePanedList ul(
    this, "Attributes", showAllAttributes,
    idClassPanedWindow, idAttributes
  );
  
  const std::list<OksAttribute *> * alist = (
    showAllAttributes
      ? get_class()->all_attributes()
      : get_class()->direct_attributes()
  );

  if(alist) {
    for(std::list<OksAttribute *>::const_iterator i = alist->begin(); i != alist->end(); ++i)
      OksXm::List::add_row(ul.list_w, (*i)->get_name().c_str());
  }
}

void
OksSchemaEditorClassDialog::show_relationships()
{
  UpdatePanedList ul(
    this, "Relationships", showAllRelationships,
    idClassPanedWindow, idRelationships
  );
  
  const std::list<OksRelationship *> * rlist = (
    showAllRelationships
      ? get_class()->all_relationships()
      : get_class()->direct_relationships()
  );

  if(rlist) {
    for(std::list<OksRelationship *>::const_iterator i = rlist->begin(); i != rlist->end(); ++i)
      OksXm::List::add_row(ul.list_w, (*i)->get_name().c_str());
  }
}

void
OksSchemaEditorClassDialog::show_methods()
{
  UpdatePanedList ul(
    this, "Methods", showAllMethods,
    idClassPanedWindow, idMethods
  );

  const std::list<OksMethod *> * mlist = (
    showAllMethods
      ? get_class()->all_methods()
      : get_class()->direct_methods()
  );

  if(mlist) {
    for(std::list<OksMethod *>::const_iterator i = mlist->begin(); i != mlist->end(); ++i)
      OksXm::List::add_row(ul.list_w, (*i)->get_name().c_str());
  }
}


void
OksSchemaEditorClassDialog::refresh()
{
  set_value(idClass, get_class()->get_name().c_str());
  change_schema_file();

  OksFile * fp = get_class()->get_file();
  set_value(idSchema, fp->get_full_file_name().c_str());

  show_superclasses();
  show_subclasses();
  show_attributes();
  show_relationships();
  show_methods();

  OksForm * f = get_paned_window(idClassPanedWindow)->get_form(idAcClassProperties);

  f->set_value(idIsAbstract, (get_class()->get_is_abstract() ? szYes : szNo));
  f->set_value(idDescription, get_class()->get_description().c_str());
}


void
OksSchemaEditorClassDialog::update()
{
  OksForm *f = get_paned_window(idClassPanedWindow)->get_form(idAcClassProperties);

  try {
    OksXm::AutoCstring description(XmTextGetString(f->get_widget(idDescription)));
    get_class()->set_description(description.get());
    get_class()->set_is_abstract(strcmp(f->get_value(idIsAbstract), szYes) ? false : true);
  }
  catch(oks::exception& ex) {
    Oks::error_msg("OksSchemaEditorClassDialog::update()") << ex.what() << std::endl;
  }
}

void
OksSchemaEditorClassDialog::change_schema_file()
{
    // set schema file name

  OksFile * fp = get_class()->get_file();
  
  set_value(idSchema, fp->get_full_file_name().c_str());


    // set title

  {
    std::string s2(get_class()->get_name().c_str());
    if(is_read_only()) s2 += " (read-only)";
    setTitle("Oks Class", s2.c_str());
  }


    // set access to the class properties

  {
    OksForm *f = get_paned_window(idClassPanedWindow)->get_form(idAcClassProperties);

    if(is_read_only())
      OksXm::TextField::set_not_editable(f->get_widget(idDescription));
    else
      OksXm::TextField::set_editable(f->get_widget(idDescription));

    XtSetSensitive(XmOptionButtonGadget(f->get_widget(idIsAbstract)), (is_read_only() ? false : true));
  }
}

void
OksSchemaEditorClassDialog::select_super_class(const std::string& s)
{
  OksXm::List::select_row(
    get_paned_window(idClassPanedWindow)->get_form(idSuperClasses)->get_widget(idSuperClasses),
    s.c_str()
  );
}
