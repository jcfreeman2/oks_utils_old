#include <oks/relationship.h>
#include <oks/class.h>
#include <oks/xm_utils.h>
#include <oks/xm_popup.h>

#include <Xm/Xm.h>
#include <Xm/List.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/RowColumn.h>
#include <Xm/SelectioB.h>

#include "schema_editor_oks_relationship.xpm"

#include "schema_editor_relationship_dlg.h"
#include "schema_editor_class_dlg.h"
#include "schema_editor_main_dlg.h"
#include "schema_editor.h"


static const char * szZeroToOne			= "ZERO or ONE";
static const char * szZeroToMany		= "ZERO or MANY";
static const char * szOneToOne			= "ONE";
static const char * szOneToMany			= "ONE or MANY";

static const char * szWeak			= "Weak (non composite)";
static const char * szDependentExclusive	= "Dependent Exclusive";
static const char * szIndependentExclusive	= "Independent Exclusive";
static const char * szDependentShared		= "Dependent Shared";
static const char * szIndependentShared		= "Independent Shared";



void
OksSchemaEditorRelationshipDialog::iconCB(Widget w, XtPointer client_data, XtPointer)
{
  long user_choice = (long)OksXm::get_user_data(w);

  OksPopupMenu::destroy(w);

  OksSchemaEditorRelationshipDialog * dlg = (OksSchemaEditorRelationshipDialog *)client_data;
  OksClass * c = dlg->get_class();
  OksRelationship * r = dlg->get_relationship();

  try {
    switch(user_choice) {
      case idUpdate:
        dlg->update();
        break;

      case idRename:
        r->set_name(
          OksXm::ask_name(
            dlg->parent()->get_form_widget(),
            "Input new name of the relationship",
            "Rename OKS Relationship",
	    r->get_name().c_str()
          )
        );
        dlg->set_value(idName, r->get_name().c_str());
        break;

      case idDelete:
        delete dlg;
        c->remove(r);
        break;
    }
  }
  catch(oks::exception& ex) {
    Oks::error_msg("OksSchemaEditorRelationshipDialog::iconCB") << ex.what() << std::endl;
  }
}

void
OksSchemaEditorRelationshipDialog::iconAC(Widget w, XtPointer client_data, XEvent *event, Boolean *)
{
  make_popup(
    w, client_data, event,
    (((OksSchemaEditorRelationshipDialog *)client_data)->is_read_only()),
    iconCB
  );
}


void
OksSchemaEditorRelationshipDialog::setCB(Widget w, XtPointer data, XtPointer)
{
  OksXm::AutoCstring value(OksXm::List::get_selected_value(w));

  ((OksSchemaEditorRelationshipDialog *)data)->set_value(idClassType, value.get());

  OksSchemaEditorMainDialog::destroy_select_class_dlg(w);
/*  Widget top = XtParent(XtParent(XtParent(w)));
  XtUnmanageChild(top);
  XtDestroyWidget(top);*/
}


void
OksSchemaEditorRelationshipDialog::chooseCB(Widget, XtPointer data, XtPointer)
{
  ((OksSchemaEditorRelationshipDialog *)data)->parent()->create_select_class_dlg("Choose Class", 0, setCB, data);
}


OksSchemaEditorRelationshipDialog::OksSchemaEditorRelationshipDialog(OksRelationship *rl, OksClass *cl, OksSchemaEditorMainDialog *p) :
  OksSchemaEditorDialog	(p, cl),
  p_rel			(rl)
{
  static Pixmap	pixmap = 0;
 
  if(!pixmap)
    pixmap = OksXm::create_color_pixmap(get_form_widget(), schema_editor_oks_relationship_xpm);
  
  createOksDlgHeader(&pixmap, iconAC, (void *)this, idName, szName, idDefinedInClass, szDefinedInClass);

  setIcon(pixmap);

  OksXm::TextField::set_not_editable(get_widget(idDefinedInClass));

  add_label(OksDlgEntity::idLabel, "Relationship Class Type:");
 
  OksXm::TextField::set_not_editable(add_text_field(idClassType, szClassType));

  XtAddCallback(add_push_button(idBrowse, szBrowse), XmNactivateCallback, chooseCB, (XtPointer)this);
  attach_previous(idBrowse);

  add_separator();


  add_label(OksDlgEntity::idLabel, "Relationship Constarints:");

  std::list<const char *> cardinalityConstraintList;
  cardinalityConstraintList.push_back(szZeroToOne);
  cardinalityConstraintList.push_back(szZeroToMany);
  cardinalityConstraintList.push_back(szOneToOne);
  cardinalityConstraintList.push_back(szOneToMany);
  cardinalityConstraintList.push_back(szCardinalityConstraint);
  
  add_option_menu(idCardinalityConstraint, &cardinalityConstraintList);


  std::list<const char *> referenceTypeList;
  referenceTypeList.push_back(szWeak);
  referenceTypeList.push_back(szDependentExclusive);
  referenceTypeList.push_back(szIndependentExclusive);
  referenceTypeList.push_back(szDependentShared);
  referenceTypeList.push_back(szIndependentShared);
  referenceTypeList.push_back(szReferenceType);
  
  add_option_menu(idReferenceType, &referenceTypeList);

  add_separator();

  add_text(idDescription, szDescription);
  attach_right(idDescription);

  add_separator();
  
  XtAddCallback(addCloseButton("OK"), XmNactivateCallback, closeCB, (XtPointer)this);
  OksXm::set_close_cb(XtParent(get_form_widget()), closeCB, (void *)this);

  attach_bottom(idDescription);

  change_schema_file();
  refresh();
  show();

  setMinSize();
  setCascadePos();
}

OksSchemaEditorRelationshipDialog::~OksSchemaEditorRelationshipDialog()
{
  update();
  parent()->remove_dlg(this);
}

void
OksSchemaEditorRelationshipDialog::refresh()
{
  set_value(idName, p_rel->get_name().c_str());
  OksXm::TextField::set_not_editable(get_widget(idName));
 
  set_value(idDefinedInClass, get_class()->get_name().c_str());

  
  set_value(idClassType, p_rel->get_type().c_str());

  set_value(idCardinalityConstraint,
      p_rel->get_low_cardinality_constraint() == OksRelationship::Zero &&
      p_rel->get_high_cardinality_constraint() == OksRelationship::One
      ? szZeroToOne
    :
      p_rel->get_low_cardinality_constraint() == OksRelationship::Zero &&
      p_rel->get_high_cardinality_constraint() == OksRelationship::Many
      ? szZeroToMany
    :
      p_rel->get_low_cardinality_constraint() == OksRelationship::One &&
      p_rel->get_high_cardinality_constraint() == OksRelationship::One
      ? szOneToOne : szOneToMany
  );  

  set_value(idReferenceType,
    !p_rel->get_is_composite() ? szWeak :
    !p_rel->get_is_exclusive() && !p_rel->get_is_dependent() ? szIndependentShared :
    p_rel->get_is_exclusive() && !p_rel->get_is_dependent() ? szIndependentExclusive :
    !p_rel->get_is_exclusive() && p_rel->get_is_dependent() ? szDependentShared :
    szDependentExclusive
  );  

  set_value(idDescription, p_rel->get_description().c_str());
}

void
OksSchemaEditorRelationshipDialog::update()
{
  try {

      // set description and class type

    {
      OksXm::AutoCstring description(XmTextGetString(get_widget(idDescription)));
      OksXm::AutoCstring class_type(XmTextFieldGetString(get_widget(idClassType)));

      get_relationship()->set_type(class_type.get());
      get_relationship()->set_description(description.get());
    }


      // set cardinality constraint

    {
      char * cardinalityConstraint = get_value(idCardinalityConstraint);

      get_relationship()->set_low_cardinality_constraint(
        !strcmp(cardinalityConstraint, szZeroToOne) || !strcmp(cardinalityConstraint, szZeroToMany) ? OksRelationship::Zero :
        !strcmp(cardinalityConstraint, szOneToOne)  || !strcmp(cardinalityConstraint, szOneToMany)  ? OksRelationship::One :
        OksRelationship::Many
      );

      get_relationship()->set_high_cardinality_constraint(
        !strcmp(cardinalityConstraint, szZeroToOne) || !strcmp(cardinalityConstraint, szOneToOne) ? OksRelationship::One :
        OksRelationship::Many
      );
    }


      // set reference type

    {
      char * referenceType = get_value(idReferenceType);

      int isComposite, isExclusive, isDependent;

      if(!strcmp(referenceType, szWeak))
        isComposite = isExclusive = isDependent = false;
      else {
        isComposite = true;
        isExclusive = !strcmp(referenceType, szDependentExclusive) || !strcmp(referenceType, szIndependentExclusive) ? true : false;
        isDependent = !strcmp(referenceType, szDependentExclusive) || !strcmp(referenceType, szDependentShared) ? true : false;
      }

      get_relationship()->set_is_composite(isComposite);
      get_relationship()->set_is_exclusive(isExclusive);
      get_relationship()->set_is_dependent(isDependent);
    }
  }
  catch(oks::exception& ex) {
    Oks::error_msg("OksSchemaEditorRelationshipDialog::update()") << ex.what() << std::endl;
  }

}


void
OksSchemaEditorRelationshipDialog::change_schema_file()
{
    // set title

  {
    std::string s(p_rel->get_name());
    if(is_read_only()) s += " (read-only)";
    setTitle("Oks Relationship", get_class()->get_name().c_str(), s.c_str());
  }


    // set access to the attribute properties

  {
    bool state = is_read_only() ? false : true;

    XtSetSensitive(get_widget(idBrowse), state);

    XtSetSensitive(XmOptionButtonGadget(get_widget(idCardinalityConstraint)), state);
    XtSetSensitive(XmOptionButtonGadget(get_widget(idReferenceType)), state);

    if(is_read_only()) {
      OksXm::TextField::set_not_editable(get_widget(idClassType));
      OksXm::TextField::set_not_editable(get_widget(idDescription));
    }
    else {
      OksXm::TextField::set_editable(get_widget(idClassType));
      OksXm::TextField::set_editable(get_widget(idDescription));
    }
  }
}
