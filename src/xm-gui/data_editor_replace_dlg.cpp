#include <ctype.h>
#include <string.h>

#include <sstream>
#include <string>
#include <memory>

#include <boost/regex.hpp>

#include <ers/ers.h>

#include <oks/relationship.h>
#include <oks/class.h>
#include <oks/object.h>
#include <oks/xm_utils.h>
#include <oks/xm_popup.h>

#include <Xm/Xm.h>
#include <Xm/List.h>
#include <Xm/TextF.h>

#include "data_editor_search_result_dlg.h"
#include "data_editor_replace_dlg.h"
#include "data_editor_object_dlg.h"
#include "data_editor_main_dlg.h"
#include "data_editor_exceptions.h"
#include "data_editor.h"

#include "warning.xpm"
#include "error.xpm"

extern OksDataEditorMainDialog * mainDlg;

const char * OksDataEditorReplaceDialog::s_extra_search_none            = "only";
const char * OksDataEditorReplaceDialog::s_extra_search_and_subclasses  = "+ subclasses";
const char * OksDataEditorReplaceDialog::s_extra_search_with_query      = "with query";

	//
	// Provide two methods for case insensitive comparison and search
	//

int
OksDataEditorReplaceDialog::compare(const char * s1, const char * s2, CompareType type)
{
  if(!s2 || !*s2) return 0;

  if(type == use_case)
    return strcmp(s1, s2);
  else
    return strcasecmp(s1, s2);
}

const char *
OksDataEditorReplaceDialog::index(const char * s1, const char * s2, CompareType type)
{
  if(!s2 || !*s2) return 0;

  if(type == use_case)
    return strstr(s1, s2);
  else {
    char * buf1 = new char[strlen(s1)+1];
    char * buf2 = new char[strlen(s2)+1];

    for(size_t i=0; i<=strlen(s1); i++)
      buf1[i] = tolower(s1[i]);
    for(size_t j=0; j<=strlen(s2); j++)
      buf2[j] = tolower(s2[j]);

    char * res = strstr(buf1, buf2);

    delete [] buf1;
    delete [] buf2;

    if(res == 0)
      return 0;
    else
      return (s1 + (res - buf1));
  }
}

void
OksDataEditorReplaceDialog::pasteCB(Widget, XtPointer client_data, XtPointer)
{
  OksDataEditorReplaceDialog * dlg = (OksDataEditorReplaceDialog *)client_data;
  XmTextFieldSetString(dlg->m_query_text_field, const_cast<char *>(dlg->parent->p_selected_query.c_str()));
}

void
OksDataEditorReplaceDialog::selectAC(Widget w, XtPointer client_data, XEvent *event, Boolean *)
{
  if (((XButtonEvent *)event)->button != Button3) return;

  OksPopupMenu popup(w);

  OksDataEditorReplaceDialog * dlg = (OksDataEditorReplaceDialog *)client_data;

  if(dlg->parent->p_selected_query.empty())
    popup.addDisableItem("Paste query");
  else
    popup.addItem("Paste query", 0, pasteCB, client_data);

  popup.show((XButtonPressedEvent *)event);
}

	//
	// Create Find/Replace dialog depending on given action type
	//

OksDataEditorReplaceDialog::OksDataEditorReplaceDialog(OksDataEditorMainDialog *p, ActionType a_type) :
  OksDialog	           ("", p),
  m_action_type	           (a_type),
  parent	           (p),
  m_attr_type_on           (true),
  m_selected_class         (0),
  m_search_in_subclasses   (false),
  m_selected_attribute     (0),
  m_selected_relationship  (0),
  m_from_rel_tips          (0),
  m_to_rel_tips            (0)
{
  std::string title = (a_type == replace) ? "Replace" : "Find";

  setTitle(title.c_str());
  setIcon((OksDialog *)p);


    // Specify replace by attribute/relationship choice

  title += ':';
  add_label(idTitle, title.c_str());

  XtAddCallback(
    add_radio_button(idAttributeValueType, "Attribute Value"),
    XmNvalueChangedCallback,
    OksDataEditorReplaceDialog::switchTypeCB,
    (XtPointer)this
  );

  XtAddCallback(
    add_radio_button(idRelationshipValueType, "Relationship Value"),
    XmNvalueChangedCallback,
    OksDataEditorReplaceDialog::switchTypeCB,
    (XtPointer)this
  );

  set_value(idAttributeValueType, "On");

  attach_previous(idRelationshipValueType);


  add_separator();


    // Specify fields to input values

  Widget w_from;

  if(a_type == replace) {
    w_from = add_text_field(idFrom, "Value to Find:");
    attach_right(idFrom);

    Widget w_to = add_text_field(idTo, "Replace With:");
    attach_right(idTo);

    XtAddEventHandler(w_to, ButtonPressMask, False, relValueAC, reinterpret_cast<XtPointer>(this));
  }
  else {
    w_from = add_text_field(idFrom, "Value:");
  }

  attach_right(idFrom);

  XtAddEventHandler(w_from, ButtonPressMask, False, relValueAC, reinterpret_cast<XtPointer>(this));

  add_separator();


    // Specify attribute value properties

  add_label(idAttributeProperties, "Attribute Properties:");

  std::list<const char *> builtInTypesList;
  builtInTypesList.push_back(OksAttribute::string_type);
  builtInTypesList.push_back(OksAttribute::bool_type);
  builtInTypesList.push_back(OksAttribute::s8_int_type);
  builtInTypesList.push_back(OksAttribute::u8_int_type);
  builtInTypesList.push_back(OksAttribute::s16_int_type);
  builtInTypesList.push_back(OksAttribute::u16_int_type);
  builtInTypesList.push_back(OksAttribute::s32_int_type);
  builtInTypesList.push_back(OksAttribute::u32_int_type);
  builtInTypesList.push_back(OksAttribute::s64_int_type);
  builtInTypesList.push_back(OksAttribute::u64_int_type);
  builtInTypesList.push_back(OksAttribute::float_type);
  builtInTypesList.push_back(OksAttribute::double_type);
  builtInTypesList.push_back(OksAttribute::date_type);
  builtInTypesList.push_back(OksAttribute::time_type);
  builtInTypesList.push_back(OksAttribute::enum_type);
  builtInTypesList.push_back("Attribute Type:");

  add_option_menu(idAttributeTypes, &builtInTypesList);
  set_option_menu_cb(idAttributeTypes, OksDataEditorReplaceDialog::chooseAttrTypeCB, (XtPointer)this);

  add_toggle_button(idAttributeWholeString, "Match whole string");

  add_toggle_button(idAttributeCaseString, "Case sensitive");
  attach_previous(idAttributeCaseString);

  XtAddCallback(add_toggle_button(idAttributeRegExpString, "Reqular Expression"), XmNvalueChangedCallback, OksDataEditorReplaceDialog::changeOnRegExpCB, (XtPointer)this);
  attach_previous(idAttributeRegExpString);

  add_separator();
  std::list<const char *> classesList;
  classesList.push_back("*");
  for(OksClass::Map::const_iterator i = parent->classes().begin(); i != parent->classes().end(); ++i) {
    classesList.push_back(i->first);
  }
  classesList.push_back("Search in class:");
  XtAddCallback (add_combo_box(idSearchInClass, &classesList), XmNselectionCallback, OksDataEditorReplaceDialog::chooseSearchInClassCB, (XtPointer)this);


  std::list<const char *> extraList;
  extraList.push_back(s_extra_search_none);
  extraList.push_back(s_extra_search_and_subclasses);
  extraList.push_back(s_extra_search_with_query);
  extraList.push_back("");
  XtSetSensitive(add_option_menu(idSearchInClassExtra, &extraList), false);
  attach_previous(idSearchInClassExtra);

  m_query_text_field = add_text_field(idSearchInClassWithQuery, "");
  attach_previous(idSearchInClassWithQuery);
  attach_right(idSearchInClassWithQuery);
  
  if(!parent->p_selected_query.empty()) {
    XmTextFieldSetString(m_query_text_field, const_cast<char *>(parent->p_selected_query.c_str()));
  }
  
  XtAddEventHandler(m_query_text_field, ButtonPressMask, False, selectAC, (XtPointer)this);

  std::list<const char *> attrsAndRelsList;
  attrsAndRelsList.push_back("*");
  attrsAndRelsList.push_back("Search in attribute/relationship:");
  Widget search_ar_w = add_combo_box(idSearchInAttrOrRel, &attrsAndRelsList);
  XtSetSensitive(search_ar_w, false);
  XtAddCallback(search_ar_w, XmNselectionCallback, OksDataEditorReplaceDialog::chooseSearchInAttrOrRelCB, (XtPointer)this);

  set_option_menu_cb(idSearchInClassExtra, OksDataEditorReplaceDialog::chooseSearchClassExtraCB, (XtPointer)this);


  add_separator();

    // Add action buttones

  if(a_type == replace) {
    XtAddCallback(
      add_push_button(idReplaceBtn, "Replace"),
      XmNactivateCallback,
      OksDataEditorReplaceDialog::replaceCB,
      (XtPointer)this
    );
  }

  XtAddCallback(
    add_push_button(idFindBtn, "Find"),
    XmNactivateCallback,
    OksDataEditorReplaceDialog::replaceCB,
    (XtPointer)this
  );

  if(a_type == replace)
    attach_previous(idFindBtn);


  XtAddCallback(
    add_push_button(idHelpBtn, "Help"),
    XmNactivateCallback,
    OksDataEditorReplaceDialog::helpCB,
    (XtPointer)this
  );

  attach_previous(idHelpBtn);


  XtAddCallback(
    add_push_button(idCloseBtn, "Close"),
    XmNactivateCallback,
    OksDataEditorReplaceDialog::closeCB,
    (XtPointer)this
  );

  OksXm::set_close_cb(
    XtParent(get_form_widget()),
    OksDataEditorReplaceDialog::closeCB,
    (void *)this
  );

  attach_previous(idCloseBtn);


  show();


    // Fix bug with lestiff-0.87 on linux
    // It does not calculate properly height of the window when there are
    // two toggle buttons on the same line

#if defined(__linux__) && (XmVersion < 2000)
  Dimension bh, wh;
  Widget w = get_widget(idCloseBtn);
  
  XtVaGetValues(w, XmNheight, &bh, NULL);
  XtVaGetValues(XtParent(form), XmNheight, &wh, NULL);

  XtVaSetValues(XtParent(form), XmNheight, wh + 2 * bh + 10, NULL);
#endif

  setMinSize();
  setCascadePos();

  XtUnmanageChild(m_query_text_field);
}


	//
	// Destructor of Find/Replace window
	//

OksDataEditorReplaceDialog::~OksDataEditorReplaceDialog()
{
  if(m_action_type == replace)
    parent->delete_replace_dialog();
  else
    parent->delete_find_dialog();
}


	//
	// The 'switchType' callbacks change buttons access depending on
	// type of value replacement
	// * attribute type:
	//    - attribute radio button is on
	//    - relationship radio button is off
	//    - attribute types option menu is available
	//    - set string specific search options
	// * relationship type:
	//    - attribute radio button is off
	//    - relationship radio button is on
	//    - attribute types option menu is not available
	//    - string specific search options are not available
	//

void
OksDataEditorReplaceDialog::switchTypeCB(Widget w, XtPointer client_data, XtPointer)
{
  OksDataEditorReplaceDialog * dlg(reinterpret_cast<OksDataEditorReplaceDialog *>(client_data));

  if(dlg->get_widget(idAttributeValueType) == w) {
    dlg->m_attr_type_on = true;
    dlg->set_value(idAttributeValueType, "On");
    dlg->set_value(idRelationshipValueType, "Off");

    XtSetSensitive(dlg->get_widget(idAttributeTypes), true);

    chooseAttrTypeCB(w, client_data, 0);
  }
  else {
    dlg->m_attr_type_on = false;
    dlg->set_value(idAttributeValueType, "Off");
    dlg->set_value(idRelationshipValueType, "On");

    XtSetSensitive(dlg->get_widget(idAttributeTypes), false);

    XtSetSensitive(dlg->get_widget(idAttributeWholeString), false);
    XtSetSensitive(dlg->get_widget(idAttributeCaseString), false);
    XtSetSensitive(dlg->get_widget(idAttributeRegExpString), false);

    const char * tips =
      "To fill a relationship value\n"
      "use object-ID@class-name format\n"
      "or select an object and click\n"
      "here right mouse button to paste.";

    if(!dlg->m_from_rel_tips) {
      dlg->m_from_rel_tips = OksXm::show_text_as_tips(dlg->get_widget(idFrom), tips);
    }

    if(dlg->m_action_type == replace && !dlg->m_to_rel_tips) {
      dlg->m_to_rel_tips = OksXm::show_text_as_tips(dlg->get_widget(idTo), tips);
    }
  }

  dlg->update_search_in_attrs_and_rels();
}

void
OksDataEditorReplaceDialog::changeOnRegExpCB(Widget, XtPointer client_data, XtPointer call_data)
{
  OksDataEditorReplaceDialog * dlg(reinterpret_cast<OksDataEditorReplaceDialog *>(client_data));

  if(((XmToggleButtonCallbackStruct *)call_data)->set == True) {
    XtSetSensitive(dlg->get_widget(idAttributeWholeString), false);
    XtSetSensitive(dlg->get_widget(idAttributeCaseString), false);
  }
  else {
    XtSetSensitive(dlg->get_widget(idAttributeWholeString), true);
    XtSetSensitive(dlg->get_widget(idAttributeCaseString), true);
  }
}

	//
	// The 'chooseAttrType' callbacks change access to string specific
	// search parameters
	//

void
OksDataEditorReplaceDialog::chooseAttrTypeCB(Widget, XtPointer client_data, XtPointer)
{
  OksDataEditorReplaceDialog * dlg(reinterpret_cast<OksDataEditorReplaceDialog *>(client_data));

  char *value = dlg->get_value(idAttributeTypes);
  
  if(!strcmp(value, OksAttribute::string_type)) {
    XtSetSensitive(dlg->get_widget(idAttributeWholeString), true);
    XtSetSensitive(dlg->get_widget(idAttributeCaseString), true);
    XtSetSensitive(dlg->get_widget(idAttributeRegExpString), true);
  }
  else {
    XtSetSensitive(dlg->get_widget(idAttributeWholeString), false);
    XtSetSensitive(dlg->get_widget(idAttributeCaseString), false);
    XtSetSensitive(dlg->get_widget(idAttributeRegExpString), false);
  }
}

void
OksDataEditorReplaceDialog::chooseSearchInAttrOrRelCB(Widget, XtPointer client_data, XtPointer call_data)
{
  OksDataEditorReplaceDialog * dlg(reinterpret_cast<OksDataEditorReplaceDialog *>(client_data));
  XmComboBoxCallbackStruct * cb(reinterpret_cast<XmComboBoxCallbackStruct *>(call_data));

  OksXm::AutoCstring value(OksXm::string_to_c_str(cb->item_or_text));

  if(dlg->m_selected_class) {
    if(dlg->m_attr_type_on) {
      dlg->m_selected_attribute = dlg->m_selected_class->find_attribute(value.get());
      dlg->m_selected_relationship = 0;

      if(dlg->m_selected_attribute) {
        const std::string val(dlg->get_value(idAttributeTypes));
        OksData::Type type = OksAttribute::get_data_type(val);

        if(type != dlg->m_selected_attribute->get_data_type()) {
          type = dlg->m_selected_attribute->get_data_type();
          const char * new_val = (
	    type == OksData::s8_int_type  ? OksAttribute::s8_int_type   :
	    type == OksData::u8_int_type  ? OksAttribute::u8_int_type   :
	    type == OksData::s16_int_type ? OksAttribute::s16_int_type  :
	    type == OksData::u16_int_type ? OksAttribute::u16_int_type  :
	    type == OksData::s32_int_type ? OksAttribute::s32_int_type  :
	    type == OksData::u32_int_type ? OksAttribute::u32_int_type  :
	    type == OksData::s64_int_type ? OksAttribute::s64_int_type  :
	    type == OksData::u64_int_type ? OksAttribute::u64_int_type  :
	    type == OksData::float_type   ? OksAttribute::float_type    :
	    type == OksData::double_type  ? OksAttribute::double_type   :
	    type == OksData::bool_type    ? OksAttribute::bool_type     :
	    type == OksData::class_type   ? OksAttribute::class_type    :
	    type == OksData::date_type    ? OksAttribute::date_type     :
	    type == OksData::time_type    ? OksAttribute::time_type     :
	    type == OksData::string_type  ? OksAttribute::string_type   :
	    type == OksData::enum_type    ? OksAttribute::enum_type     :
	    "unknown"
	  );
	  dlg->set_value(idAttributeTypes, new_val);
	  chooseAttrTypeCB(dlg->get_widget(idAttributeValueType), client_data, 0);
	}
        XtSetSensitive(dlg->get_widget(idAttributeTypes), false);
      }
      else {
        XtSetSensitive(dlg->get_widget(idAttributeTypes), true);
      }
    }
    else {
      dlg->m_selected_attribute = 0;
      dlg->m_selected_relationship = dlg->m_selected_class->find_relationship(value.get());
    }
  }
  else {
    dlg->m_selected_relationship = 0;
    dlg->m_selected_attribute = 0;
  }
}

void
OksDataEditorReplaceDialog::update_search_in_attrs_and_rels()
{
  if(m_selected_class) {
    std::list<const char *> items;
    items.push_back("*");

    if(m_attr_type_on) {
      for(std::list<OksAttribute *>::const_iterator i = m_selected_class->all_attributes()->begin(); i != m_selected_class->all_attributes()->end(); ++i) {
	items.push_back((*i)->get_name().c_str());
      }
    }
    else {
      for(std::list<OksRelationship *>::const_iterator i = m_selected_class->all_relationships()->begin(); i != m_selected_class->all_relationships()->end(); ++i) {
	items.push_back((*i)->get_name().c_str());
      }
    }

    change_combo_box(idSearchInAttrOrRel, items);
  }
}

void
OksDataEditorReplaceDialog::chooseSearchInClassCB(Widget, XtPointer client_data, XtPointer call_data)
{
  OksDataEditorReplaceDialog * dlg(reinterpret_cast<OksDataEditorReplaceDialog *>(client_data));
  XmComboBoxCallbackStruct * cb(reinterpret_cast<XmComboBoxCallbackStruct *>(call_data));

  OksXm::AutoCstring value(OksXm::string_to_c_str(cb->item_or_text));

  if(!strcmp(value.get(), "*")) {
    XtSetSensitive(dlg->get_widget(idSearchInClassExtra), false);
    XtSetSensitive(dlg->m_query_text_field, false);
    XtSetSensitive(dlg->get_widget(idSearchInAttrOrRel), false);
    dlg->m_selected_class = 0;
    dlg->m_selected_relationship = 0;
    dlg->m_selected_attribute = 0;

    dlg->set_value(idSearchInAttrOrRel, "*");
    dlg->set_value(idAttributeTypes, OksAttribute::string_type);

    Widget x = dlg->get_widget(idAttributeTypes);
    XtSetSensitive(x, true);
    chooseAttrTypeCB(x, client_data, 0);
  }
  else {
    XtSetSensitive(dlg->get_widget(idSearchInAttrOrRel), true);
    XtSetSensitive(dlg->get_widget(idSearchInClassExtra), true);
    XtSetSensitive(dlg->m_query_text_field, true);
    OksClass * c = dlg->parent->find_class(value.get());
    if(c != dlg->m_selected_class) {
      dlg->m_selected_class = c;
      dlg->update_search_in_attrs_and_rels();
    }
  }
}

void
OksDataEditorReplaceDialog::chooseSearchClassExtraCB(Widget, XtPointer client_data, XtPointer)
{
  OksDataEditorReplaceDialog * dlg(reinterpret_cast<OksDataEditorReplaceDialog *>(client_data));

  char * value = dlg->get_value(idSearchInClassExtra);
  Widget x(dlg->m_query_text_field);

  if(!strcmp(value, s_extra_search_with_query)) {
    if(XtIsManaged(x) == false) XtManageChild(x);
  }
  else {
    if(XtIsManaged(x) == true) XtUnmanageChild(x);
  }
}

	//
	// The 'close' callbacks destroyes Find/Replace dialog window
	//

void
OksDataEditorReplaceDialog::closeCB(Widget, XtPointer client_data, XtPointer)
{
  delete (OksDataEditorReplaceDialog *)client_data;
}


	//
	// The 'help' callbacks displays help window
	//

void
OksDataEditorReplaceDialog::helpCB(Widget, XtPointer client_data, XtPointer)
{
  ((OksDataEditorReplaceDialog *)client_data)->parent->show_help_window("ReplaceWindow.html");
}

       //
       // This method searches an attribute of enum type with range valid for passed enumarator 
       // If enumerator is 0, return an attribute
       // If attribute with non-null enumerator cannot be found, report error
       //

const OksAttribute *
OksDataEditorReplaceDialog::find_enum_attribute(const char * enumerator)
{
  size_t len = (enumerator ? strlen(enumerator) : 0);
  for(OksClass::Map::const_iterator i = parent->classes().begin(); i != parent->classes().end(); ++i) {
    if(const std::list<OksAttribute *> * attrs = i->second->direct_attributes()) {
      for(std::list<OksAttribute *>::const_iterator j = attrs->begin(); j != attrs->end(); ++j) {
        if(!enumerator) return *j;
        if((*j)->get_enum_index(enumerator, len) >= 0) {
          return *j;
        }
      }
    }
  }

  if(enumerator) {
    std::ostringstream text;
    text << "OKS database has no any attribute with enumerator \'" << enumerator << '\'';
    XtManageChild(
      OksXm::create_dlg_with_pixmap(
        get_form_widget(), "Search Attribute Error", error_xpm, text.str().c_str(), "Ok", 0, 0, 0, 0
      )
    );
  }

  return 0;
}

	//
	// This callback reads information from the replace dialog,
	// fills it in the class and calls method to perform replacement
	//

void
OksDataEditorReplaceDialog::replaceCB(Widget w, XtPointer client_data, XtPointer)
{
  OksDataEditorReplaceDialog *dlg = (OksDataEditorReplaceDialog *)client_data;

  dlg->m_find_only = (w == dlg->get_widget(idFindBtn) ? true : false);

  bool status = true;
  std::ostringstream msg_text; // non-empty, when status is false

  OksXm::AutoCstring value_from(XmTextFieldGetString(dlg->get_widget(idFrom)));
  OksXm::AutoCstring value_to((dlg->m_find_only ? 0 : XmTextFieldGetString(dlg->get_widget(idTo))));

  OksData d_from;
  OksData d_to;

  dlg->item_type = simple;

  if(dlg->m_selected_class) {
    char * value = dlg->get_value(idSearchInClassExtra);

    dlg->m_search_query.clear();
    dlg->m_search_in_subclasses = false;

    if(!strcmp(value, s_extra_search_and_subclasses)) {
      dlg->m_search_in_subclasses = true;
    }
    else if(!strcmp(value, s_extra_search_with_query)) {
      OksXm::AutoCstring q(XmTextFieldGetString(dlg->get_widget(idSearchInClassWithQuery)));
      dlg->m_search_query = q.get();
    }
  }

  if(dlg->m_attr_type_on == true) {
    const char *type_txt = dlg->get_value(idAttributeTypes);

    if(strcmp(type_txt, OksAttribute::enum_type)) {

        // find any attribute used below by ReadFrom() methods;
        // which attribute is not really important, since enumerations are proceeed differently

      const OksAttribute * a = dlg->find_enum_attribute(0);

      if(a == 0) {
        XtManageChild(
          OksXm::create_dlg_with_pixmap(
            dlg->get_form_widget(), "Search Attribute Error", error_xpm, "this OKS database has no any attribute", "Ok", 0, 0, 0, 0
          )
        );
        return;
      }

      try {
        d_from.ReadFrom(value_from.get(), OksAttribute::get_data_type(type_txt, strlen(type_txt)), a);
      }
      catch(oks::exception& ex) {
        std::ostringstream msg_text;

        msg_text << "\"Value to Find\" parameter ERROR:\n" << ex.what();

        XtManageChild(
          OksXm::create_dlg_with_pixmap(
            dlg->get_form_widget(), "Search Parameter Error", error_xpm, msg_text.str().c_str(), "Ok", 0, 0, 0, 0
          )
        );
        return;
      }

      if(dlg->m_find_only == false) {
        try {
          d_to.ReadFrom(value_to.get(), OksAttribute::get_data_type(type_txt, strlen(type_txt)), a);
        }
        catch(oks::exception& ex) {
          std::ostringstream msg_text;

          msg_text << "\"Replace With\" parameter ERROR:\n" << ex.what();

          XtManageChild(
            OksXm::create_dlg_with_pixmap(
              dlg->get_form_widget(), "Search Parameter Error", error_xpm, msg_text.str().c_str(), "Ok", 0, 0, 0, 0
            )
          );
          return;
        }
      }


	// A string replacement may be different from replacement of other types
	// (e.g. it may require 'case insensitive' or/and 'part of string' match)

      if(!strcmp(type_txt, OksAttribute::string_type)) {
        Boolean b;

	XtVaGetValues(dlg->get_widget(idAttributeWholeString), XmNset, &b, NULL);

        dlg->match_whole_string = (b ? true : false);

	XtVaGetValues(dlg->get_widget(idAttributeCaseString), XmNset, &b, NULL);

	if(dlg->match_whole_string == false || !b) {
	  dlg->item_type = complex_string;
	  dlg->case_compare = (b ? use_case : ignore_case);
	}

	XtVaGetValues(dlg->get_widget(idAttributeRegExpString), XmNset, &b, NULL);
	if(b) {
	  dlg->item_type = complex_string;
	  dlg->m_is_reg_exp = true;
	}
	else {
	  dlg->m_is_reg_exp = false;
	}
      }
    }
    else {
      const OksAttribute * a_from = dlg->find_enum_attribute(value_from.get());
      if(a_from == 0) return;

      dlg->item_type = enumeration;

      d_from.data.ENUMERATION = a_from->get_enum_value(value_from.get(), strlen(value_from.get()));
      d_from.type = OksData::enum_type;

      if(dlg->m_find_only == false) {
        const OksAttribute * a_to = dlg->find_enum_attribute(value_to.get());
        if(a_to == 0) return;
        d_to.data.ENUMERATION = a_to->get_enum_value(value_to.get(), strlen(value_to.get()));
        d_to.type = OksData::enum_type;
      }
    }
  }
  else {
    bool value_is_set = (value_from.get() && strlen(value_from.get()));
    std::string value_copy(value_is_set ? value_from.get() : "[nil]");

    d_from.Set(value_is_set ? findOksObject(value_from.get()) : (OksObject *)0);

    if(!d_from.data.OBJECT) {
      msg_text << (dlg->m_find_only ? "Find" : "Replace") << ": cannot find object \'" << value_copy << "\'.";
      status = false;
    }

    if(status == true && dlg->m_find_only == false) {
      value_is_set = (value_to.get() && strlen(value_to.get()));
      value_copy = (value_is_set ? value_to.get() : "[nil]");

      d_to.Set(value_is_set ? findOksObject(value_to.get()) : (OksObject *)0);

      if(!d_to.data.OBJECT) {
        msg_text << "Replace: cannot find object \'" << value_copy << "\'.";
        status = false;
      }
    }
  }

  if(status) {
    std::string s(dlg->m_attr_type_on ? d_from.str() : d_from.str(dlg->parent));
    XmTextFieldSetString(dlg->get_widget(idFrom), const_cast<char *>(s.c_str()));

    if(dlg->m_find_only == false) {
      s = (dlg->m_attr_type_on ? d_to.str() : d_to.str(dlg->parent));
      XmTextFieldSetString(dlg->get_widget(idTo), const_cast<char *>(s.c_str()));
    }

    if(dlg->m_find_only == false && d_from == d_to) {
      msg_text << "Replace: nothing to do because values are equal.";
      status = false;
    }
    else {
      if(dlg->do_replacement(d_from, d_to) == 0) {
        msg_text << "No objects satisfying find parameters been found.";
        status = false;
      }
    }
  }

  if(status == false) {
    XtManageChild(
      OksXm::create_dlg_with_pixmap(
        dlg->get_form_widget(), "Search Result", warning_xpm, msg_text.str().c_str(), "Ok", 0, 0, 0, 0
      )
    );
  }
}

	//
	// Replace substring by pattern and report
	//

std::string
OksDataEditorReplaceDialog::do_partial_replacement(const OksData & pattern, const OksData & subst, const OksString& str) const
{
  ERS_DEBUG(2, "found (case " << (case_compare == use_case ? "" : "in") << "sensitive match) substring " << pattern << " in \'" << str << '\'');

  if(m_find_only == false) {
    std::string new_str(str.c_str());			// new string

    const OksString& pat = *pattern.data.STRING;	// pattern string
    const size_t pat_len = pat.length();		// pattern string length

    const char * sub_str = subst.data.STRING->c_str();	// replacement string
    size_t sub_len = subst.data.STRING->length();	// replacement string length

    size_t new_index = 0;				// new search place (to avoid recursion)
    const char *ptr = 0;

    while((ptr = index(new_str.c_str() + new_index, pat.c_str(), case_compare)) != 0) {
      size_t idx = ptr - new_str.c_str();		// index of pattern string
      new_str.replace(idx, pat_len, sub_str);
      new_index = idx + sub_len;
    }

    ERS_DEBUG(2, "new value is \'" << new_str << '\'');

    return new_str;
  }
  else
    return str.c_str();
}


	//
	// Checks correspondance of object described by the given oks data
	// to relationship class type:
	//  - returns true is the object's class is equal to the relationship class type,
	//    or any it's superclass is equal to the relationship class type;
	//  - returns false otherwise.
	//

bool
OksDataEditorReplaceDialog::check_class_type(const OksRelationship *r, const OksData *d) const
{
  const OksClass *r_ctype = r->get_class_type();
  const OksClass *o_ctype = d->data.OBJECT->GetClass();

  if(r_ctype == o_ctype) return true;
  else {
    if(const OksClass::FList * ss = o_ctype->all_super_classes()) {
      for(OksClass::FList::const_iterator iss = ss->begin(); iss != ss->end();++iss)
        if(r_ctype == *iss) return true;
    }
  }

  return false;
}

struct StatusOfFiles {
  std::set<OksFile *> m_writable;
  std::set<OksFile *> m_read_only;
  std::map<OksFile *, std::string> m_locked;
  
  ReplaceDialog::FileStatus get_status(const OksObject *o, std::string& status);
};


ReplaceDialog::FileStatus
StatusOfFiles::get_status(const OksObject *o, std::string& status)
{
  OksFile * f = o->get_file();

  if(m_read_only.find(f) != m_read_only.end()) return ReplaceDialog::read_only_file;

  if(m_writable.find(f) != m_writable.end()) return ReplaceDialog::writable_file;

  std::map<OksFile *, std::string>::iterator i = m_locked.find(f);
  if(i != m_locked.end()) {
    status = i->second;
    return ReplaceDialog::locked_file;
  }

//  if(f->get_repository() == OksFile::GlobalRepository && !o->GetClass()->get_kernel()->get_user_repository_root().empty()) {
//    std::ostringstream text;
//    text << "Some objects satisfy replacement criteria and are stored\n"
//            "on repository file \"" << f->get_repository_name() << "\"\n"
//            "Would you like to check it out on user repository and to modify locally?";
//
//    if(mainDlg->confirm_dialog("Checkout Repository File", text.str().c_str(), exit_xpm)) {
//      mainDlg->checkout_file(f);
//    }
//  }

  if(OksKernel::check_read_only(f)) {
    m_read_only.insert(f);
    return ReplaceDialog::read_only_file;
  }

  std::string lock_info;
  if(f->is_locked()) {
    status = "locked";
  }
  else if(f->get_lock_string(lock_info)) {
    status = std::string("locked by ") + lock_info;
    m_locked[f] = status;
    return ReplaceDialog::locked_file;
  }

  m_writable.insert(f);
  return ReplaceDialog::writable_file;
}


static bool
set_attribute_value(OksObject * o, const OksDataInfo& data_info, OksData& d_from, OksData& d_to, StatusOfFiles& files, std::list<OksDataEditorReplaceResult *>& result)
{
  std::string file_status_string;
  ReplaceDialog::FileStatus file_status(files.get_status(o, file_status_string));

  std::string from(d_from.str());
  std::string to(d_to.str());

  if(file_status == ReplaceDialog::writable_file) {
    try {
      o->SetAttributeValue(&data_info, &d_to);
      result.push_back(new OksDataEditorReplaceResult(o, data_info.attribute, from, to));
      return true;
    }
    catch(oks::exception& ex) {
      ers::error(OksDataEditor::Problem(ERS_HERE, ex.what()));
      result.push_back(new OksDataEditorReplaceResult(o, data_info.attribute, from, to, ex));
    }
  }
  else if(file_status == ReplaceDialog::read_only_file) {
    std::ostringstream text;
    text << "read-only file \'" << o->get_file()->get_full_file_name() << '\'';
    result.push_back(new OksDataEditorReplaceResult(o, data_info.attribute, from, to, text.str(), file_status));
  }
  else {
    std::ostringstream text;
    text << "file \'" << o->get_file()->get_full_file_name() << "\' is " << file_status_string;
    result.push_back(new OksDataEditorReplaceResult(o, data_info.attribute, from, to, text.str(), file_status));
  }

  if(d_to.type == OksData::list_type) d_to = d_from;

  return false;
}

static bool
set_relationship_value(OksObject * o, const OksDataInfo& data_info, OksData& d_from, OksData& d_to, StatusOfFiles& files, std::list<OksDataEditorReplaceResult *>& result)
{
  std::string file_status_string;
  ReplaceDialog::FileStatus file_status(files.get_status(o, file_status_string));

  std::string from(d_from.str(o->GetClass()->get_kernel()));
  std::string to(d_to.str(o->GetClass()->get_kernel()));

  if(file_status == ReplaceDialog::writable_file) {
    try {
      o->SetRelationshipValue(&data_info, &d_to);
      result.push_back(new OksDataEditorReplaceResult(o, data_info.relationship, from, to));
      return true;
    }
    catch(oks::exception& ex) {
      ers::error(OksDataEditor::Problem(ERS_HERE, ex.what()));
      result.push_back(new OksDataEditorReplaceResult(o, data_info.relationship, from, to, ex));
    }
  }
  else if(file_status == ReplaceDialog::read_only_file) {
    result.push_back(new OksDataEditorReplaceResult(o, data_info.relationship, from, to, "read-only", file_status));
  }
  else {
    result.push_back(new OksDataEditorReplaceResult(o, data_info.relationship, from, to, file_status_string, file_status));
  }

  if(d_to.type == OksData::list_type) d_to = d_from;

  return false;
}


	//
	// Performs find and replacement if required
	//

int
OksDataEditorReplaceDialog::do_replacement(OksData & d_from, OksData & d_to)
{
  if(parent->classes().empty()) {
    return 0;
  }
  
  int count = 0;
  
  std::list<OksDataEditorReplaceResult *> result;
  StatusOfFiles files;
  
  boost::regex * re = 0;
 
  if(m_attr_type_on && (item_type == complex_string) && m_is_reg_exp) {
    try {
      re = new boost::regex(*d_from.data.STRING);
    }
    catch(std::exception& ex) {
      std::ostringstream text;
      text << "ERROR: cannot create reqular expression\nfrom string \'" << *d_from.data.STRING << "\':\n" << ex.what() << std::ends;
      XtManageChild(
        OksXm::create_dlg_with_pixmap(
          get_form_widget(), "Search Error", error_xpm, text.str().c_str(), "Ok", 0, 0, 0, 0
        )
      );
      return -1;
    }
  }


    // if there is a query, parse and execute it

  bool use_query(m_selected_class && !m_search_query.empty());
  std::map<const OksClass*, const OksObject::Map*> query_result;

  if(use_query) {
    try {
      std::unique_ptr<OksQuery> qe(new OksQuery(m_selected_class, m_search_query));
      if(qe->good() == false) {
        std::ostringstream text;
        text << "bad query syntax \"" << m_search_query << "\" in scope of class \"" << m_selected_class->get_name() << '\"';
        XtManageChild(
          OksXm::create_dlg_with_pixmap(
            get_form_widget(), "Search Error", error_xpm, text.str().c_str(), "Ok", 0, 0, 0, 0
          )
        );
        return -1;
      }

      if(OksObject::List * objs = m_selected_class->execute_query(qe.get())) {
        OksObject::Map * cur_map = 0;
        const OksClass * cur_class = 0;

        for(OksObject::List::iterator i = objs->begin(); i != objs->end(); ++i) {
          OksObject * o = *i;
          
            // note, the objects in the query result list are ranged by classes
          
          if(cur_class != o->GetClass()) {
            cur_class = o->GetClass();
            cur_map = new OksObject::Map();
            query_result[cur_class] = cur_map;
          }

          (*cur_map)[&o->GetId()] = o;
        }

        delete objs;
      }

    }
    catch(oks::exception& ex) {
      std::ostringstream text;
      text << "failed to execute query:\n" << ex.what();
      XtManageChild(
        OksXm::create_dlg_with_pixmap(
          get_form_widget(), "Query Error", error_xpm, text.str().c_str(), "Ok", 0, 0, 0, 0
        )
      );
      return -1;
    }
  }


  for(OksClass::Map::const_iterator ic = parent->classes().begin(); ic != parent->classes().end(); ++ic) {
    OksClass *c = ic->second;

    const OksObject::Map * objs = c->objects();
    if(!objs) continue;

    if(m_selected_class) {
      if(use_query) {
        std::map<const OksClass*, const OksObject::Map*>::iterator x = query_result.find(c);
        if(x == query_result.end()) continue;
        else objs = x->second;

      }
      else {
        if(c != m_selected_class) {
          if(m_search_in_subclasses == false) {
	    continue;
	  }
	  else {
	    bool is_derived = false;
	    if(const OksClass::FList * sc = c->all_super_classes()) {
	      for(OksClass::FList::const_iterator j = sc->begin(); j != sc->end(); ++j) {
	        if(*j == m_selected_class) { is_derived = true; break; }
	      }
	    }
	    if(!is_derived) { continue; }
	  }
        }
      }
    }

    if(m_attr_type_on == true) {
      const std::list<OksAttribute *> * attrs = c->all_attributes();
      if(!attrs) continue;

      OksDataInfo data_info(0, (const OksAttribute *)0);

      for(std::list<OksAttribute *>::const_iterator ia = attrs->begin(); ia != attrs->end(); ++ia) {
        OksAttribute *a = *ia;
	data_info.attribute = a;

        if((!m_selected_attribute || (m_selected_attribute == a)) && (a->get_data_type() == d_from.type)) {

	     // ignore if attribute's enumeration type does not contain given token

          if(item_type == enumeration) {
            if(m_find_only == false && (a->get_enum_index(*d_to.data.ENUMERATION) < 0)) {
	      data_info.offset++;
	      continue;
	    }
	  }

          for(OksObject::Map::const_iterator io = objs->begin(); io != objs->end(); ++io) {
	    OksObject *o = (*io).second;
	    OksData *d(o->GetAttributeValue(&data_info));

	    bool updated = false;

            if(a->get_is_multi_values() == false) {
              if(item_type != complex_string) {

                   // update simple attribute (not string)

	        if(*d == d_from) {
		  count++;

                  if(m_find_only == false) {
		    updated |= set_attribute_value(o, data_info, *d, d_to, files, result);
		  }
		  else {
		    result.push_back(new OksDataEditorReplaceResult(o, a, *d));
		  }
	        }
	      }
	      else {

	          // find/update string attribute if reqular expression match is required

                if(re) {
		  if(boost::regex_match(*d->data.STRING, *re)) {
	            count++;

                    if(m_find_only == false) {
		      updated |= set_attribute_value(o, data_info, *d, d_to, files, result);
	            }
		    else {
		      result.push_back(new OksDataEditorReplaceResult(o, a, *d));
		    }
		  }
		}

	          // find/update string attribute if case insensitive match is required

	        else if(match_whole_string) {

		    // no partial match is required

	          if(!compare(d->data.STRING->c_str(), d_from.data.STRING->c_str(), case_compare)) {
	            count++;

                    if(m_find_only == false) {
		      updated |= set_attribute_value(o, data_info, *d, d_to, files, result);
	            }
		    else {
		      result.push_back(new OksDataEditorReplaceResult(o, a, *d));
		    }
	          }
                }

	          // else find/update string attribute if partial match is required

	        else {

		    // partial match is required

	          if(index(d->data.STRING->c_str(), d_from.data.STRING->c_str(), case_compare) != 0) {
	            count++;

		    OksString new_str = do_partial_replacement(d_from, d_to, *d->data.STRING).c_str();

                    if(m_find_only == false) {
		      OksData new_d(new_str);
		      updated |= set_attribute_value(o, data_info, *d, new_d, files, result);
	            }
		    else {
		      result.push_back(new OksDataEditorReplaceResult(o, a, *d));
		    }
	          }
	        }
	      }

	      if(updated) {
	        if(OksDataEditorObjectDialog * dlg = parent->find_object_dlg(o)) {
		  dlg->refresh_single_value_attribute(data_info.offset, a);
		}
	      }
	    }
	    else {
	      OksData::List * d_list = d->data.LIST;
	      
	      if(d_list && !d_list->empty()) {
	        OksData * old_value = 0; // may be used to store original value
		bool value_updated = false; // the OksData value can be updated several times while iterate the list

		for(OksData::List::iterator li = d_list->begin(); li != d_list->end(); ++li) {
		  OksData *d2 = *li;

                     // update simple attribute (not string)

                  if(item_type != complex_string) {
	            if(*d2 == d_from) {
                      if(m_find_only == false) {
		        if(!old_value) { old_value = new OksData(); *old_value = *d; }
                        *d2 = d_to;
	              }

	              value_updated = true;
	            }
	          }
	          else {

	              // update string attribute (if case insensitive or/and partial match are required)

	            if(match_whole_string) {

		        // no partial match is required

		      if(!compare(d2->data.STRING->c_str(), d_from.data.STRING->c_str(), case_compare)) {
                        if(m_find_only == false) {
		          if(!old_value) { old_value = new OksData(); *old_value = *d; }
			  *d2 = d_to;
			}

			value_updated = true;
	              }
                    }
	            else {

		        // partial match is required

		      if(index(d2->data.STRING->c_str(), d_from.data.STRING->c_str(), case_compare) != 0) {
		        OksString new_str = do_partial_replacement(d_from, d_to, *d2->data.STRING).c_str();

                        if(m_find_only == false) {
		          if(!old_value) { old_value = new OksData(); *old_value = *d; }
			  d2->Set(new_str);
	                }

	                value_updated = true;
	              }
	            }
	          }
		}

		if(value_updated) {
		  count++;

		  if(m_find_only == false) {
		    updated |= set_attribute_value(o, data_info, *old_value, *d, files, result);
		  }
		  else {
		    result.push_back(new OksDataEditorReplaceResult(o, a, *d));
		  }

		  delete old_value;
		}
	      }

	      if(updated) {
	        if(OksDataEditorObjectDialog * dlg = parent->find_object_dlg(o)) {
		  dlg->refresh_multi_value_attribute(data_info.offset, a);
		}
	      }
	    }
          }
        }

        data_info.offset++;
      }
    }
    else {
      const std::list<OksRelationship *> * rels = c->all_relationships();
      if(!rels) continue;

      OksDataInfo data_info(c->number_of_all_attributes(), (const OksRelationship *)0);

      for(std::list<OksRelationship *>::const_iterator ir = rels->begin(); ir != rels->end(); ++ir) {
        OksRelationship *r = *ir;
	data_info.relationship = r;

           // check relationship is selected and the 'object from' has compartible relationship class type

        if((!m_selected_relationship || (m_selected_relationship == r)) && (check_class_type(r, &d_from) == true)) {

             // check that the 'object to' has compartible relationship class type

	  if(m_find_only == false && check_class_type(r, &d_to) != true) continue;

          data_info.relationship = r;

          for(OksObject::Map::const_iterator io = objs->begin(); io != objs->end(); ++io) {
	    OksObject *o = (*io).second;
	    OksData *d(o->GetRelationshipValue(&data_info));

	    bool updated = false;

            if(r->get_high_cardinality_constraint() != OksRelationship::Many) {
	      if(*d == d_from) {
                count++;

                if(m_find_only == false) {
		  updated |= set_relationship_value(o, data_info, *d, d_to, files, result);
		}
		else {
		  result.push_back(new OksDataEditorReplaceResult(o, r, *d));
		}
	      }

	      if(updated) {
	        if(OksDataEditorObjectDialog * obj_dlg = parent->find_object_dlg(o)) {
		  obj_dlg->refresh_single_value_relationship(data_info.offset);
		}
	      }
	    }
	    else {
	      OksData::List * d_list = d->data.LIST;

	      if(d_list && !d_list->empty()) {
		for(OksData::List::iterator li = d_list->begin(); li != d_list->end(); ++li) {
		  OksData *d2 = *li;

	          if(*d2 == d_from) {
	            count++;

                    if(m_find_only == false) {
		      OksData old_value; old_value = *d;
		      *d2 = d_to;
		      updated |= set_relationship_value(o, data_info, old_value, *d, files, result);
	            }
		    else {
		      result.push_back(new OksDataEditorReplaceResult(o, r, *d));
		    }

		    break; // each object should be put only once into a relationship value
	          }
		}
	      }

	      if(updated) {
	        if(OksDataEditorObjectDialog * obj_dlg = parent->find_object_dlg(o)) {
		  obj_dlg->refresh_multi_value_relationship(data_info.offset);
		}
	      }
	    }
          }
        }

        data_info.offset++;
      }
    }
  }

  if(!result.empty()) {
    new OksDataEditorSearchResultDialog(*this, d_from, d_to, result);

    while(!result.empty()) {
      OksDataEditorReplaceResult * res = result.front();
      result.pop_front();
      delete res;
    }
  }

  delete re;

  for(std::map<const OksClass*, const OksObject::Map*>::iterator i = query_result.begin(); i != query_result.end(); ++i) {
    delete i->second;
  }

  return count;
}

extern OksObject *clipboardObject;

void
OksDataEditorReplaceDialog::relValueAC(Widget w, XtPointer client_data, XEvent *event, Boolean *)
{
  if (((XButtonEvent *)event)->button != Button3) return;

  OksDataEditorReplaceDialog * dlg = (OksDataEditorReplaceDialog *)client_data;

  if(dlg->m_attr_type_on) return;

  if(clipboardObject && !dlg->parent->is_dangling(clipboardObject)) {
    OksData d(clipboardObject);
    XmTextFieldSetString(w, const_cast<char *>(d.str(dlg->parent).c_str()));
  }
}
