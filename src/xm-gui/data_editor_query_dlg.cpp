#include <stdlib.h>

#include <fstream>
#include <sstream>

#include <oks/attribute.h>
#include <oks/relationship.h>
#include <oks/class.h>
#include <oks/object.h>
#include <oks/xm_utils.h>
#include <oks/xm_popup.h>

#include <Xm/Tree.h>
#include <Xm/Xm.h>
#include <Xm/List.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/ToggleB.h>
#include <Xm/RowColumn.h>
#include <Xm/SelectioB.h>


#include "data_editor_class_dlg.h"
#include "data_editor_query_dlg.h"
#include "data_editor_main_dlg.h"
#include "data_editor_exceptions.h"
#include "data_editor.h"


const char * szAddAttributeExpression	= "Add Attribute Expression";
const char * szAddRelationshipExpression= "Add Relationship Expression";
const char * szAddNotExpression		= "Add \'Not\' Expression";
const char * szAddAndExpression		= "Add \'And\' Expression";
const char * szAddOrExpression		= "Add \'Or\' Expression";
const char * szExecuteQuery		= "Execute Query";
const char * szSelectQuery		= "Select Query";
const char * szSaveQuery		= "Save Query";

const char * szQueryForm		= "Query Form:";
const char * szQueryResult		= "Query Result:";
const char * szClearQueryResult		= "Clear query result before new search";
const char * szSearchInSubClasses	= "Search in subclasses";
const char * szAllRelsObjects		= "All Objects Match Query";
const char * szSomeRelsObjects		= "Some Objects Match Query";


OksQueryExpression *
OksDataEditorQueryDialog::create_expression(Widget treeWidget, OksClass *c, OksQuery::QueryType type, Widget superNode, void *param)
{
  Widget		frameWindow = XmCreateFrame(treeWidget, (char *)"frame", 0, 0);
  OksQueryExpression	*form;

  XtVaSetValues(frameWindow,
    XmNshadowType, XmSHADOW_IN,
    XmNshadowThickness, 2,
    XmNmarginWidth, 2,
    XmNmarginHeight, 2,
    XmNsuperNode, superNode,
    NULL
  );
  		
  XtManageChild(frameWindow);
	
  if(type == OksQuery::comparator_type)
	form = (OksQueryExpression *)new OksQueryAttributeExpressionForm(c, frameWindow, (OksComparator *)param);
  else if(type == OksQuery::relationship_type)
	form = (OksQueryExpression *)new OksQueryRelationshipExpressionForm(c, frameWindow, (OksRelationshipExpression *)param);
  else if(type == OksQuery::not_type)
	form = (OksQueryExpression *)new OksQueryNotExpressionForm(frameWindow, c, treeWidget);
  else if(type == OksQuery::and_type)
	form = (OksQueryExpression *)new OksQueryAndExpressionForm(frameWindow, c, treeWidget);
  else if(type == OksQuery::or_type)
	form = (OksQueryExpression *)new OksQueryOrExpressionForm(frameWindow, c, treeWidget);
  else
	form = 0;

  if(type == OksQuery::not_type && param) {
	OksQueryExpression *q = ((OksNotExpression *)param)->get();
	
	if(q)
		((OksQueryNotExpressionForm *)form)->set(
			create_expression(
				treeWidget,
				c,
				q->type(),
				((OksQueryNotExpressionForm *)form)->getLabelFrame(),
				(void *)q
			)
		);
  }

  if(type == OksQuery::and_type && param) {
    const std::list<OksQueryExpression *> & exprs = ((OksAndExpression *)param)->expressions();

    if(!exprs.empty()) {
      for(std::list<OksQueryExpression *>::const_iterator i = exprs.begin(); i != exprs.end(); ++i) {
        ((OksQueryAndExpressionForm *)form)->add(
          create_expression(
            treeWidget,
            c,
            (*i)->type(),
            ((OksQueryAndExpressionForm *)form)->getLabelFrame(),
            (void *)(*i)
          )
        );
      }
    }
  }


  if(type == OksQuery::or_type && param) {
    const std::list<OksQueryExpression *> & exprs = ((OksOrExpression *)param)->expressions();

    if(!exprs.empty()) {
      for (std::list<OksQueryExpression *>::const_iterator i = exprs.begin(); i != exprs.end(); ++i) {
        ((OksQueryOrExpressionForm *)form)->add(
          create_expression(
            treeWidget,
            c,
            (*i)->type(),
            ((OksQueryOrExpressionForm *)form)->getLabelFrame(),
            (void *)(*i)
          )
        );
      }
    }
  }
  
  return form;
}


void
OksDataEditorQueryDialog::closeCB(Widget, XtPointer d, XtPointer)
{
  delete (OksDataEditorQueryDialog *)d;
}


void
OksDataEditorQueryDialog::objectCB(Widget, XtPointer client_data, XtPointer call_data)
{
  OksXm::AutoCstring name(OksXm::string_to_c_str(((XmListCallbackStruct *)call_data)->item));
  if(OksObject * o = findOksObject(name.get())) {
    ((OksDataEditorQueryDialog *)client_data)->parent()->create_object_dlg(o);
  }
}


void
OksDataEditorQueryDialog::formCB(Widget w, XtPointer client_data, XtPointer) 
{
  short user_data = (short)((long)OksXm::get_user_data(w));
  OksPopupMenu::destroy(w);

  OksDataEditorQueryDialog * dlg = (OksDataEditorQueryDialog *)client_data;
  OksClass * c = dlg->get_class();

  XtVaSetValues(w, XmNuserData, 0, NULL);

  if( user_data != idExecute
   && user_data != idSelectQuery
   && user_data != idSave ) {
    OksQueryExpression *form = create_expression(
      dlg->tree_widget(),
      c,
      (
        (user_data == idAddAttribute) ? OksQuery::comparator_type :
        (user_data == idAddNot) ? OksQuery::not_type :
        (user_data == idAddAnd) ? OksQuery::and_type :
        (user_data == idAddOr) ? OksQuery::or_type :
        OksQuery::relationship_type
      )
    );

    dlg->set(form);
  }
  else if(user_data == idExecute) {
    OksForm * f = dlg->get_paned_window(idQueryPanedWindow)->get_form(idQueryResult);
    Widget queryResult = f->get_widget(idQueryResult);

    try {
      OksObject::List * objs = c->execute_query((OksQuery *)dlg);

      std::list<ObjectRow *> result;
      size_t sizeOfMatrix(dlg->p_class->number_of_all_attributes() + dlg->p_class->number_of_all_relationships());

      if(objs) {
        for(OksObject::List::iterator i = objs->begin(); i != objs->end() ; ++i) {
          ObjectTable::add_row(*i, dlg->p_class, sizeOfMatrix, result);
        }

        delete objs;
      }

      ObjectTable::show_rows(queryResult, sizeOfMatrix, dlg->treeWidget, result);
    }
    catch(oks::exception& ex) {
      OksDataEditorMainDialog::report_exception("Execute Query", ex, dlg->get_form_widget());
    }
  }
  else {
    if(((OksQuery *)dlg)->get()->CheckSyntax() == false) {
      std::ostringstream text;
      text << "cannot save query \"" << *(OksQuery *)dlg << "\" because it is incomplete";
      OksDataEditorMainDialog::report_error("Save Query", text.str().c_str(), dlg->get_form_widget());
      return;
    }

    if(user_data == idSelectQuery) {
      std::ostringstream qs;
      qs << *(OksQuery *)dlg;
      dlg->parent()->p_selected_query = qs.str();
      return;
    }

    std::ostringstream st;
    st << "Save query\n" << *(OksQuery *)dlg << "\nto file";

    std::string buf = st.str();
    std::string s = OksXm::ask_name(dlg->get_form_widget(), buf.c_str(), "Save Query As");

    if(s.length() && OksXm::ask_overwrite_file(dlg->get_form_widget(), s.c_str())) {
      std::ofstream f(s.c_str());

      if(!f.good()) {
        std::ostringstream text;
        text << "cannot create file \"" << s << "\" to save query";
        OksDataEditorMainDialog::report_error("Save Query As", text.str().c_str(), dlg->get_form_widget());
      }
      else {
        f << *(OksQuery *)dlg;

        if(f.fail()) {
          std::ostringstream text;
          text << "cannot write to file \"" << s << '\"';
          OksDataEditorMainDialog::report_error("Save Query As", text.str().c_str(), dlg->get_form_widget());
        }
      }
    }
  }
}


void
OksQueryRelationshipExpressionForm::formCB(Widget w, XtPointer d, XtPointer) 
{
  OksQueryRelationshipExpressionForm	*dlg = (OksQueryRelationshipExpressionForm *)d;
  short					user_data = (short)((long)OksXm::get_user_data(w));

  XtVaSetValues(w, XmNuserData, 0, NULL);

  OksQueryExpression *form = OksDataEditorQueryDialog::create_expression(
	dlg->tree_widget(),
	findOksClass(dlg->GetRelationship()->get_type().c_str()),
	(
		(user_data == idAcQueryRelationshipFormAddAttribute) ? OksQuery::comparator_type :
		(user_data == idAcQueryRelationshipFormAddNot) ? OksQuery::not_type :
		(user_data == idAcQueryRelationshipFormAddAnd) ? OksQuery::and_type :
		(user_data == idAcQueryRelationshipFormAddOr) ? OksQuery::or_type :
		OksQuery::relationship_type
	)
  );

  dlg->set(form);

  XtSetSensitive(XmOptionButtonGadget(dlg->get_widget(idQueryRelationshipName)), false);
}


void
OksDataEditorQueryDialog::formAC(Widget w, XtPointer client_data, XEvent *event, Boolean *)
{
  if(((XButtonEvent *)event)->button != Button3) return;

  OksDataEditorQueryDialog *dlg = (OksDataEditorQueryDialog *)client_data;

  OksPopupMenu popup(w);

  if(!dlg->get()) {
    popup.addItem(szAddAttributeExpression, idAddAttribute, formCB, client_data);

    if(dlg->get_class()->number_of_all_relationships())
      popup.addItem(szAddRelationshipExpression, idAddRelationship, formCB, client_data);
    else
      popup.addDisableItem(szAddRelationshipExpression);

    popup.addItem(szAddNotExpression, idAddNot, formCB, client_data);
    popup.addItem(szAddAndExpression, idAddAnd, formCB, client_data);
    popup.addItem(szAddOrExpression, idAddOr, formCB, client_data);
    popup.add_separator();
    popup.addDisableItem(szExecuteQuery);
    popup.addDisableItem(szSelectQuery);
    popup.addDisableItem(szSaveQuery);
  }
  else {
    popup.addDisableItem(szAddAttributeExpression);
    popup.addDisableItem(szAddRelationshipExpression);
    popup.addDisableItem(szAddNotExpression);
    popup.addDisableItem(szAddAndExpression);
    popup.addDisableItem(szAddOrExpression);
    popup.add_separator();
    popup.addItem(szExecuteQuery, idExecute, formCB, client_data);
    popup.addItem(szSelectQuery, idSelectQuery, formCB, client_data);
    popup.addItem(szSaveQuery, idSave, formCB, client_data);
  }

  popup.show((XButtonPressedEvent *)event);
}


void
OksQueryRelationshipExpressionForm::formAC(Widget w, XtPointer client_data, XEvent *event, Boolean *)
{
#if !defined(__linux__) || (XmVersion > 2000)
  if (((XButtonEvent *)event)->button != Button3) return;
#endif

  OksQueryRelationshipExpressionForm *dlg = (OksQueryRelationshipExpressionForm *)client_data;

  OksPopupMenu popup(w);

  if(!dlg->get()) {
    OksClass *c = findOksClass(dlg->GetRelationship()->get_type().c_str());

    popup.addItem(szAddAttributeExpression, idAcQueryRelationshipFormAddAttribute, formCB, client_data);

    if(c->number_of_all_relationships())
      popup.addItem(szAddRelationshipExpression, idAcQueryRelationshipFormAddRelationship, formCB, client_data);
    else
      popup.addDisableItem(szAddRelationshipExpression);

    popup.addItem(szAddNotExpression, idAcQueryRelationshipFormAddNot, formCB, client_data);
    popup.addItem(szAddAndExpression, idAcQueryRelationshipFormAddAnd, formCB, client_data);
    popup.addItem(szAddOrExpression, idAcQueryRelationshipFormAddOr, formCB, client_data);
  }
  else {
    popup.addDisableItem(szAddAttributeExpression);
    popup.addDisableItem(szAddRelationshipExpression);
    popup.addDisableItem(szAddNotExpression);
    popup.addDisableItem(szAddAndExpression);
    popup.addDisableItem(szAddOrExpression);
  }

  popup.show((XButtonPressedEvent *)event);
}


void
OksDataEditorQueryDialog::searchSubClassesCB(Widget, XtPointer client_data, XtPointer call_data) 
{
  ((OksDataEditorQueryDialog *)client_data)->search_in_subclasses(((XmToggleButtonCallbackStruct *)call_data)->set);
}


OksDataEditorQueryDialog::OksDataEditorQueryDialog(OksClass *cl, OksDataEditorMainDialog *p, OksQuery *qe) :
  OksDataEditorDialog	(p),
  OksQuery		(false),
  p_class		(cl)
{
  Widget w, tb1;

  setTitle("Oks Query", get_class()->get_name().c_str());
  setIcon((OksDialog *)p);

  w = add_text_field(idClass, "Class:");
  attach_right(idClass);
  set_value(idClass, get_class()->get_name().c_str());
  OksXm::TextField::set_not_editable(w);

  tb1 = add_toggle_button(idSearchInSubClasses, szSearchInSubClasses);
  XtAddCallback (tb1, XmNvalueChangedCallback, searchSubClassesCB, (XtPointer)this);

  Widget	 panedWidget = add_paned_window(idQueryPanedWindow);
  OksPanedWindow *panedWindow = get_paned_window(idQueryPanedWindow);

  attach_right(idQueryPanedWindow);

  OksForm *f = new OksForm(panedWidget);
  f->add_form(idQueryForm, szQueryForm);

  treeWidget = (Widget)XsCreateTree(f->get_form(idQueryForm)->get_form_widget(), (char *)"Tree", 0, 0);

  XtVaSetValues(
    treeWidget,
    XmNtopAttachment, XmATTACH_FORM,
    XmNtopOffset, 5,
    XmNbottomAttachment, XmATTACH_FORM,
    XmNbottomOffset, 5,
    XmNleftAttachment, XmATTACH_FORM,
    XmNleftOffset, 0,
    XmNrightAttachment, XmATTACH_FORM,
    XmNrightOffset, 5,
    XtNwidth, 300,
    XtNheight, 500,
    NULL
  );

  XtManageChild(treeWidget);

  f->attach_right(idQueryForm);
  f->attach_bottom(idQueryForm);
  panedWindow->add_form(f, idQueryForm);

  XtAddEventHandler(treeWidget, ButtonPressMask, False, formAC, (XtPointer)this);
  XtAddEventHandler(XtParent(XtParent(treeWidget)), ButtonPressMask, False, formAC, (XtPointer)this);

  f = new OksForm(panedWidget);

  Widget mw = ObjectTable::create_matrix(*f, szQueryResult, idQueryResult, p_class, parent());
  f->attach_bottom(idQueryResult);

    // refresh
  {
    std::list<ObjectRow *> result;
    ObjectTable::show_rows(mw, p_class->number_of_all_attributes() + p_class->number_of_all_relationships(), treeWidget, result);
    XtVaSetValues(mw, XtNwidth, 300, XtNheight, 200, NULL);
  }

  panedWindow->add_form(f, idQueryResult);

  attach_bottom(idQueryPanedWindow);

  OksXm::set_close_cb(XtParent(get_form_widget()), closeCB, (void *)this);

  show();

  setMinSize();
  setCascadePos();

  XtAddCallback(mw, XmNresizeCallback, OksXm::Matrix::resizeCB, 0);

  if(qe) {
    search_in_subclasses(qe->search_in_subclasses());
    XtVaSetValues(tb1, XmNset, search_in_subclasses(), NULL);

    if(qe->get())
      set(
        create_expression(
          treeWidget,
          cl,
          qe->get()->type(),
          0,
          (void *)qe->get()
        )
      );

    delete qe;
  }
}

OksDataEditorQueryDialog::~OksDataEditorQueryDialog()
{
  parent()->remove_dlg(this);
}


void
OksQueryAttributeExpressionForm::change_value(Widget w)
{
  OksXm::AutoCstring v(XmTextFieldGetString(w));
  OksData * d = GetValue();
  d->Clear();

  if(const OksAttribute * a = GetAttribute()) {
    std::string s(v.get());

    if(GetFunction() == OksQuery::reg_exp_cmp) {
      d->Set(v.get());
    }
    else if(a->get_data_type() != OksData::enum_type) {
      try {
        d->SetValues(strip_brackets(a, v.get()), a);
      }
      catch(oks::exception& ex) {
        d->SetValues(a->get_init_value().c_str(), a);
      }

      std::string s2 = d->str();

      if(s != s2) {
        XmTextFieldSetString(w, const_cast<char *>(s2.c_str()));
      }
    }
    else {
      d->type = OksData::enum_type;
      d->data.ENUMERATION = a->get_enum_value(v.get(), strlen(v.get())); //new OksString(v.get());
    }
  }
  else {
    d->Set(v.get());
  }

  clean_reg_exp();
}


void
OksQueryAttributeExpressionForm::nameCB(Widget w, XtPointer d, XtPointer)
{
  XmString s;
  XtVaGetValues(w, XmNlabelString, &s, NULL);

  OksXm::AutoCstring value(OksXm::string_to_c_str(s));

  if(value.get()) {
    OksQueryAttributeExpressionForm * dlg = (OksQueryAttributeExpressionForm *)d;
    dlg->p_saved_attribute = dlg->get_class()->find_attribute(value.get());
    dlg->SetAttribute(dlg->p_saved_attribute);
    dlg->change_value(dlg->p_value_widget);
  }

  XmStringFree(s);	// TO BE TESTED : Found by Purify
}


void
OksQueryRelationshipExpressionForm::nameCB(Widget w, XtPointer d, XtPointer)
{
  XmString s;
  XtVaGetValues(w, XmNlabelString, &s, NULL);

  OksXm::AutoCstring value(OksXm::string_to_c_str(s));

  if(value.get()) {
    OksQueryRelationshipExpressionForm *dlg = (OksQueryRelationshipExpressionForm *)d;
    dlg->SetRelationship(dlg->get_class()->find_relationship(value.get()));
  }
}


void
OksQueryAttributeExpressionForm::functionCB(Widget w, XtPointer d, XtPointer)
{
  XmString s;
  XtVaGetValues(w, XmNlabelString, &s, NULL);

  OksXm::AutoCstring value(OksXm::string_to_c_str(s));

  if(value.get()) {
    OksQueryAttributeExpressionForm * dlg = (OksQueryAttributeExpressionForm *)d;

    std::string comp(value.get());
    dlg->SetFunction(
      (comp == OksQuery::EQ) ? OksQuery::equal_cmp            :
      (comp == OksQuery::NE) ? OksQuery::not_equal_cmp        :
      (comp == OksQuery::RE) ? OksQuery::reg_exp_cmp          :
      (comp == OksQuery::GT) ? OksQuery::greater_cmp          :
      (comp == OksQuery::LS) ? OksQuery::less_cmp             :
      (comp == OksQuery::GE) ? OksQuery::greater_or_equal_cmp :
      (comp == OksQuery::LE) ? OksQuery::less_or_equal_cmp    :
      0
    );

    dlg->change_value(dlg->p_value_widget);
  }
}

void
OksQueryAttributeExpressionForm::valueCB(Widget w, XtPointer d, XtPointer)
{
  ((OksQueryAttributeExpressionForm *)d)->change_value(w);
}


void
OksQueryAttributeExpressionForm::switchTypeCB(Widget w, XtPointer client_data, XtPointer)
{
  OksQueryAttributeExpressionForm * dlg = (OksQueryAttributeExpressionForm *)client_data;

  if(dlg->get_widget(idUseAttributeValue) == w) {
    dlg->set_value(idUseAttributeValue, "On");
    dlg->set_value(idUseObjectID, "Off");
    dlg->SetAttribute(dlg->p_saved_attribute);

    XtSetSensitive(dlg->p_name_widget, true);
  }
  else {
    dlg->set_value(idUseAttributeValue, "Off");
    dlg->set_value(idUseObjectID, "On");
    dlg->SetAttribute(0);

    XtSetSensitive(dlg->p_name_widget, false);
  }

  dlg->change_value(dlg->p_value_widget);
}

OksQueryAttributeExpressionForm::OksQueryAttributeExpressionForm(OksClass *cl, Widget p, OksComparator *q) :
  OksComparator	    (0, new OksData(), OksQuery::equal_cmp),
  OksForm	    (p),
  p_class	    (cl),
  p_saved_attribute (0)
{
  show();
  GetValue()->type = OksData::unknown_type;

  p_use_attr_value_widget = add_radio_button(idUseAttributeValue, "Attribute Value");
  p_object_id_widget = add_radio_button(idUseObjectID, "Object ID");

  XtAddCallback(p_use_attr_value_widget, XmNvalueChangedCallback, switchTypeCB, (XtPointer)this);
  XtAddCallback(p_object_id_widget, XmNvalueChangedCallback, switchTypeCB, (XtPointer)this);

  attach_previous(idUseObjectID);

  std::list<const char *> attrList;
  const std::list<OksAttribute *> * attrs = get_class()->all_attributes();
  const char * attrDefaultValue = 0;

  if(attrs && !attrs->empty()) {
    for(std::list<OksAttribute *>::const_iterator i = attrs->begin(); i != attrs->end(); ++i) {
      if(!q && !GetAttribute()) {
        if(!p_saved_attribute) p_saved_attribute = *i;
        SetAttribute(*i);
        attrDefaultValue = (*i)->get_init_value().c_str();
      }

      attrList.push_back((*i)->get_name().c_str());
    }
  }
  else {
    attrList.push_back("(no attributes)");
  }

  attrList.push_back("Attribute:");

  p_name_widget = add_option_menu(idName, &attrList);
  set_option_menu_cb(idName, nameCB, (XtPointer)this);

  p_value_widget = add_text_field(idValue, "Value:");

  if(!q && attrDefaultValue) {
    set_value(idValue, attrDefaultValue);
    GetValue()->SetValues(attrDefaultValue, GetAttribute());
  }

  XtAddCallback (p_value_widget, XmNvalueChangedCallback, valueCB, (XtPointer)this);


  std::list<const char *> compFnList;
  
  compFnList.push_back(OksQuery::EQ);
  compFnList.push_back(OksQuery::NE);
  compFnList.push_back(OksQuery::RE);
  compFnList.push_back(OksQuery::LE);
  compFnList.push_back(OksQuery::GE);
  compFnList.push_back(OksQuery::LS);
  compFnList.push_back(OksQuery::GT);
  compFnList.push_back("Comparator:");

  add_option_menu(idComparator, &compFnList);
  set_option_menu_cb(idComparator, functionCB, (XtPointer)this);

  if(q) {
    const OksAttribute *a = q->GetAttribute();
    SetAttribute(a);
    p_saved_attribute = a;
    if(a) {
      set_value(idName, a->get_name().c_str());
    }

    SetFunction(q->GetFunction());
    set_value(idComparator,
      GetFunction() == OksQuery::equal_cmp            ? OksQuery::EQ :
      GetFunction() == OksQuery::not_equal_cmp        ? OksQuery::NE :
      GetFunction() == OksQuery::reg_exp_cmp          ? OksQuery::RE :
      GetFunction() == OksQuery::greater_cmp          ? OksQuery::GT :
      GetFunction() == OksQuery::less_cmp             ? OksQuery::LS :
      GetFunction() == OksQuery::greater_or_equal_cmp ? OksQuery::GE :
      OksQuery::LE
    );


    OksData * new_data = new OksData();
    *new_data = *q->GetValue();
    SetValue(new_data);

    {
      std::string cvts = GetValue()->str();
      set_value(idValue, cvts.c_str());
    }
  }
  else {
    SetFunction(OksQuery::equal_cmp);
  }

  attach_bottom(0);

  if(attrs && !attrs->empty()) {
    switchTypeCB(p_use_attr_value_widget, (XtPointer)this, 0);
  }
  else {
    XtSetSensitive(p_use_attr_value_widget, false);
    XtSetSensitive(p_name_widget, false);
    switchTypeCB(p_object_id_widget, (XtPointer)this, 0);
  }
}


void
OksQueryRelationshipExpressionForm::classesCB(Widget w, XtPointer client_data, XtPointer call_data) 
{
  ((OksQueryRelationshipExpressionForm *)client_data)->SetIsCheckAllObjects(((XmToggleButtonCallbackStruct *)call_data)->set);

  XmString string = OksXm::create_string(
    ((XmToggleButtonCallbackStruct *)call_data)->set ? szAllRelsObjects : szSomeRelsObjects
  );

  XtVaSetValues(w, XmNlabelString, string, NULL);
  XmStringFree(string);
}


OksQueryRelationshipExpressionForm::OksQueryRelationshipExpressionForm(OksClass *c, Widget p, OksRelationshipExpression *q) :
  OksRelationshipExpression	(0, 0),
  OksForm			(p),
  p_class			(c)
{
  Widget w;

  show();

  std::list<const char *> relsList;
  const std::list<OksRelationship *> * rels = get_class()->all_relationships();

  if(rels) {
    for(std::list<OksRelationship *>::const_iterator i = rels->begin(); i != rels->end(); ++i) {
      if(!q && !GetRelationship())
        SetRelationship(*i);

      relsList.push_back((*i)->get_name().c_str());
    }
  }

  relsList.push_back("Relationship:");

  add_option_menu(idQueryRelationshipName, &relsList);
  set_option_menu_cb(idQueryRelationshipName, nameCB, (XtPointer)this);

  if(q) {
    const OksRelationship *r = q->GetRelationship();
    SetRelationship(r);
    set_value(idQueryRelationshipName, r->get_name().c_str());
  }

  w = add_toggle_button(idAllRelsObjects, szSomeRelsObjects);
  XtAddCallback (w, XmNvalueChangedCallback, classesCB, (XtPointer)this);

  if(q) {
    XmToggleButtonCallbackStruct s;
    s.set = q->IsCheckAllObjects();
    classesCB(w, (XtPointer)this, &s);
    XtVaSetValues(w, XmNset, q->IsCheckAllObjects(), NULL);
  }

  add_simple_form(idRelationshipTreeForm);
  treeWidget = (Widget)XsCreateTree(get_form(idRelationshipTreeForm)->get_form_widget(), (char *)"Tree", 0, 0);

  XtManageChild(treeWidget);
  XtAddEventHandler(treeWidget, ButtonPressMask, False, formAC, (XtPointer)this);
  XtVaSetValues(
    treeWidget,
    XmNtopAttachment, XmATTACH_FORM,
    XmNtopOffset, 5,
    XmNbottomAttachment, XmATTACH_FORM,
    XmNbottomOffset, 5,
    XmNleftAttachment, XmATTACH_FORM,
    XmNleftOffset, 0,
    XmNrightAttachment, XmATTACH_FORM,
    XmNrightOffset, 5,
    XtNwidth, 20,
    XtNheight, 100,
    NULL
  );

  attach_right(idRelationshipTreeForm);
  attach_bottom(idRelationshipTreeForm);

  if(q && q->get()) {
    OksClass *ac = get_class()->get_kernel()->find_class(q->GetRelationship()->get_type());

    if(ac)
      set(
        OksDataEditorQueryDialog::create_expression(
          treeWidget,
          ac,
          q->get()->type(),
          0,
          (void *)(q->get())
        )
      );
  }
}

void
OksQuerySimpleExpressionForm::formCB(Widget w, XtPointer d, XtPointer) 
{
  long user_data = (long)OksXm::get_user_data(w);
  OksPopupMenu::destroy(w);

  OksQuerySimpleExpressionForm * dlg = (OksQuerySimpleExpressionForm *)d;
  OksQueryExpression *form = OksDataEditorQueryDialog::create_expression(
    dlg->tree_widget(),
    dlg->getSimpleClass(),
    (
      (
        user_data == idAcQueryAndAddAttribute	||
        user_data == idAcQueryOrAddAttribute	||
        user_data == idAcQueryNotAddAttribute 
      ) ? OksQuery::comparator_type :
      (
        user_data == idAcQueryAndAddNot		||
        user_data == idAcQueryOrAddNot		||
        user_data == idAcQueryNotAddNot 
      ) ? OksQuery::not_type :
      (
        user_data == idAcQueryAndAddAnd		||
        user_data == idAcQueryOrAddAnd		||
        user_data == idAcQueryNotAddAnd 
      ) ? OksQuery::and_type :
      (
        user_data == idAcQueryAndAddOr		||
        user_data == idAcQueryOrAddOr		||
        user_data == idAcQueryNotAddOr 
      ) ? OksQuery::or_type :
      OksQuery::relationship_type
    ),
    dlg->getLabelFrame()
  );

  if(
    user_data == idAcQueryNotAddAttribute	||
    user_data == idAcQueryNotAddRelationship	||
    user_data == idAcQueryNotAddNot		||
    user_data == idAcQueryNotAddAnd		||
    user_data == idAcQueryNotAddOr
  )
    ((OksQueryNotExpressionForm *)dlg)->set(form);
  else if(
    user_data == idAcQueryOrAddAttribute	||
    user_data == idAcQueryOrAddRelationship	||
    user_data == idAcQueryOrAddNot		||
    user_data == idAcQueryOrAddAnd		||
    user_data == idAcQueryOrAddOr
  )
    ((OksQueryOrExpressionForm *)dlg)->add(form);
  else
    ((OksQueryAndExpressionForm *)dlg)->add(form);
}


void
OksQuerySimpleExpressionForm::formAC(Widget w, XtPointer client_data, XEvent *event, Boolean *)
{
#if !defined(__linux__) || (XmVersion > 2000)
  if (((XButtonEvent *)event)->button != Button3) return;
#endif

  const OksClass * c = ((OksQuerySimpleExpressionForm *)client_data)->getSimpleClass();
  bool hasRelationships = c->number_of_all_relationships() ? true : false;  
  OksQueryExpression * user_data = (OksQueryExpression *)OksXm::get_user_data(XtParent(w));

  OksPopupMenu popup(w);

  switch(user_data->type()) {
    case OksQuery::not_type:
      if(((OksNotExpression *)user_data)->get()) {
        popup.addDisableItem(szAddAttributeExpression);
        popup.addDisableItem(szAddRelationshipExpression);
        popup.addDisableItem(szAddNotExpression);
        popup.addDisableItem(szAddAndExpression);
        popup.addDisableItem(szAddOrExpression);
      }
      else {
        popup.addItem(szAddAttributeExpression, idAcQueryNotAddAttribute, formCB, client_data);

        if(hasRelationships)
          popup.addItem(szAddRelationshipExpression, idAcQueryNotAddRelationship, formCB, client_data);
        else
          popup.addDisableItem(szAddRelationshipExpression);

        popup.addItem(szAddNotExpression, idAcQueryNotAddNot, formCB, client_data);
        popup.addItem(szAddAndExpression, idAcQueryNotAddAnd, formCB, client_data);
        popup.addItem(szAddOrExpression, idAcQueryNotAddOr, formCB, client_data);
      }
      break;

    case OksQuery::and_type:
      popup.addItem(szAddAttributeExpression, idAcQueryAndAddAttribute, formCB, client_data);

      if(hasRelationships)
        popup.addItem(szAddRelationshipExpression, idAcQueryAndAddRelationship, formCB, client_data);
      else
        popup.addDisableItem(szAddRelationshipExpression);

      popup.addItem(szAddNotExpression, idAcQueryAndAddNot, formCB, client_data);
      popup.addItem(szAddAndExpression, idAcQueryAndAddAnd, formCB, client_data);
      popup.addItem(szAddOrExpression, idAcQueryAndAddOr, formCB, client_data);
      break;

    case OksQuery::or_type:
      popup.addItem(szAddAttributeExpression, idAcQueryOrAddAttribute, formCB, client_data);

      if(hasRelationships)
        popup.addItem(szAddRelationshipExpression, idAcQueryOrAddRelationship, formCB, client_data);
      else
        popup.addDisableItem(szAddRelationshipExpression);

      popup.addItem(szAddNotExpression, idAcQueryOrAddNot, formCB, client_data);
      popup.addItem(szAddAndExpression, idAcQueryOrAddAnd, formCB, client_data);
      popup.addItem(szAddOrExpression, idAcQueryOrAddOr, formCB, client_data);
      break;

    default:
      break;
  }

  popup.show((XButtonPressedEvent *)event);
}


OksQuerySimpleExpressionForm::OksQuerySimpleExpressionForm(Widget parent, const char *name, OksClass *c, Widget tw) : labelFrame (parent), simpleClass (c), treeWidget (tw)
{
  XmString string = OksXm::create_string(name);
  Widget w = XmCreateLabel(parent, (char *)"query", 0, 0);

  XtVaSetValues(w, XmNlabelString, string, NULL);
  XtManageChild(w);

  XtAddEventHandler(w, ButtonPressMask, False, formAC, (XtPointer)this);

  if(string) XmStringFree(string);
}
