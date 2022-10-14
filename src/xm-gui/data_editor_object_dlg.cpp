#include <fstream>
#include <stdlib.h>

#include <sstream>

#include <oks/class.h>
#include <oks/object.h>
#include <oks/attribute.h>
#include <oks/relationship.h>
#include <oks/xm_utils.h>
#include <oks/xm_popup.h>

#include <Xm/Xm.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/SelectioB.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/ComboBox.h>

#include "data_editor_object_dlg.h"
#include "data_editor_main_dlg.h"
#include "data_editor_data_dlg.h"
#include "data_editor.h"
#include "data_editor_exceptions.h"


#include "data_editor_oks_object.xpm"


const char * OksDataEditorObjectDialog::dec_type = "decimal";
const char * OksDataEditorObjectDialog::hex_type = "hexadecimal";
const char * OksDataEditorObjectDialog::oct_type = "octal";

const char * szShowObject	= "Show Object      ";
const char * szAddObject	= "Add Object       ";
const char * szRemoveObject	= "Remove Object    ";
const char * szUpObject		= "Move Up Object   ";
const char * szDownObject	= "Move Down Object ";
const char * szSetObject	= "Set Object       ";
const char * szModifyValue	= "Modify Value     ";
const char * szAddValue		= "Add Value        ";
const char * szDeleteValue	= "Delete Value     ";
const char * szUpValue		= "Move Up Value    ";
const char * szDownValue	= "Move Down Value  ";
const char * szSortAscending    = "Sort Ascending   ";
const char * szSortDescending   = "Sort Descending  ";

const short idObjectAttributeData    = 10000;
const short idObjectRelationshipData = 20000;


OksObject *clipboardObject;


static bool
is_multiline_string(const char * s)
{
  return (strchr(s, '\n') != nullptr || strchr(s, '\t') != nullptr);
}


static bool
is_multiline_string(const OksData& d)
{
  return (
    (d.type == OksData::string_type) &&
    d.data.STRING &&
    is_multiline_string(d.data.STRING->c_str())
  );
}


    // the text field should to attempt resize width itself

static void
adjust_text_size(Widget w, const char *value)
{
  if (XmIsText(w))
    {
      short rows = 1, columns = 0, row_len = 0;

      for (const char * p = value; *p; ++p)
        {
          if (*p == '\n')
            {
              rows++;
              if (row_len > columns)
                columns = row_len;
              row_len = 0;
            }
          else if(*p == '\t')
            row_len += 8;
          else
            row_len++;
        }

      if (row_len > columns)
        columns = row_len;

      if (rows < 2)
        rows = 2;
      else if (rows > 128)
        rows = 128;

      if (columns < 12)
        columns = 12;
      else if (columns > 256)
        columns = 256;

      XtVaSetValues(w, XmNcolumns, columns, XmNrows, rows, XmNresizeWidth, True, XmNresizeHeight, True, NULL);
    }
  else
    {
      unsigned int tf_len = strlen(value);

      if (tf_len > 12)
        XtVaSetValues(w, XmNcolumns, tf_len, NULL);

      if (tf_len)
        {
          Position x = 0, y = 0;

          if (XmTextFieldPosToXY(w, strlen(value), &x, &y))
            {
              Dimension width, margin_width, hl_thickness;

              XtVaGetValues(w, XmNwidth, &width, XmNhighlightThickness, &hl_thickness, XmNmarginWidth, &margin_width, NULL);
              if (x != 0 && width > x)
                XtVaSetValues(w, XmNwidth, x + 10 + margin_width + hl_thickness, NULL); // assume '10' is width of last char
            }
        }
    }
}


void
OksDataEditorObjectDialog::createObjectByName(char *name)
{
  if(*name != '#') {
    std::string objName(name); // make a copy because findOksObject() changes 'name'

    if(OksObject *obj = findOksObject(name)) {
      parent()->create_object_dlg(obj);
    }
    else {
      std::ostringstream text;
      text << "object \'" << objName << "\' is not found (probably it was deleted)";
      ers::error(OksDataEditor::InternalProblem(ERS_HERE, text.str().c_str()));
    }
  }
  else {
    std::ostringstream text;
    text << "object \'" << name << "\" is not loaded";
    OksDataEditorMainDialog::report_error("Create Object By Name", text.str().c_str());
  }
}


void
OksDataEditorObjectDialog::closeCB(Widget, XtPointer data, XtPointer)
{
  delete (OksDataEditorObjectDialog *)data;
}


void
OksDataEditorObjectDialog::createNewObjectCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  if(((XmSelectionBoxCallbackStruct *)call_data)->reason == XmCR_OK) {
    OksDataEditorObjectDialog *dlg = (OksDataEditorObjectDialog *)client_data;
    OksXm::AutoCstring value(XmTextGetString(XmSelectionBoxGetChild(w, XmDIALOG_VALUE_TEXT)));
    try {
      dlg->parent()->create_object_dlg(
        (value.is_non_empty())
          ? new OksObject(*(dlg->getObject()), value.get())
          : new OksObject(*(dlg->getObject()))
      );
    }
    catch(oks::exception& ex)
    {
      OksDataEditorMainDialog::report_exception("Create Object", ex, dlg->get_form_widget());
    }
  }
}

void
OksDataEditorObjectDialog::renameObjectCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  if(((XmSelectionBoxCallbackStruct *)call_data)->reason == XmCR_OK) {
    OksDataEditorObjectDialog * dlg = (OksDataEditorObjectDialog *)client_data;
    OksDataEditorMainDialog * main_dlg = dlg->parent();
    OksXm::AutoCstring value(XmTextGetString(XmSelectionBoxGetChild(w, XmDIALOG_VALUE_TEXT)));

    if(value.is_non_empty()) {
      OksObject * o = dlg->getObject();
      std::string old_obj_name = OksDataEditorDataDialog::make_name(o);

      try {
        o->set_id(value.get());

        dlg->set_value(idObject, value.get());

          // update window title

        dlg->setTitle("Oks Object", o->GetClass()->get_name().c_str(), value.get());


        std::string new_obj_name = OksDataEditorDataDialog::make_name(o);


          // update the Data File Dialog, if it exists

        if(OksDataEditorDataDialog * odedd = main_dlg->find_data_dlg(o->get_file())) {
	  odedd->refresh_objects();
        }

          // update all Object Dialogs if they contain renamed object

        new_obj_name.insert((std::string::size_type)0, 1, '[');
        old_obj_name.insert((std::string::size_type)0, 1, '[');

        new_obj_name += ']';
        old_obj_name += ']';

        const std::list<OksDataEditorDialog *> & dlgs = main_dlg->get_dialogs();

        for(std::list<OksDataEditorDialog *>::const_iterator i = dlgs.begin(); i != dlgs.end(); ++i) {
          if((*i)->type() != OksDataEditorDialog::ObjectDlg) continue;

          const std::list<OksDlgEntity*> & dlg_items = (*i)->get_form(idData)->get_items();

          std::list<OksDlgEntity*>::const_iterator j = dlg_items.begin();

          for(;j != dlg_items.end();++j) {
            OksDlgEntity *e = *j;

            if(e->id >= idObjectRelationshipData) {
              if(e->type == OksDlgEntity::List)
                OksXm::List::replace(e->d.w, old_obj_name.c_str(), new_obj_name.c_str());
              else if(e->type == OksDlgEntity::TextField) {
		OksXm::AutoCstring text_field_obj_name(XmTextFieldGetString(e->d.w));

                if(old_obj_name == const_cast<const char *>(text_field_obj_name.get()))
                  XmTextFieldSetString(e->d.w, const_cast<char *>(new_obj_name.c_str()));
	      }
            }
          }
        }
      }
      catch(oks::exception& ex) {
        OksDataEditorMainDialog::report_exception("Rename Object", ex, dlg->get_form_widget());
      }
    }
  }
}


void
OksDataEditorObjectDialog::actionCB(Widget w, XtPointer client_data, XtPointer)
{
  OksDataEditorObjectDialog *dlg = (OksDataEditorObjectDialog *)client_data;
  
  switch((long)OksXm::get_user_data(w)) {
    case idAcObjectSelect:
	clipboardObject = dlg->getObject();
	break;

    case idAcObjectShowReferencedBy:
	dlg->parent()->create_object_referenced_by_dlg(dlg->getObject());
	break;

    case idAcObjectDelete:
        try {
    	  OksObject::destroy(dlg->getObject());
        }
        catch(oks::exception& ex) {
          OksDataEditorMainDialog::report_exception("Destroy Object", ex, dlg->get_form_widget());
        }
	break;

    case idAcObjectCopy: {
	Arg	 args[5];
	XmString xm_string = OksXm::create_string("Input object's ID for new object:");
	XmString xm_string2 = OksXm::create_string("(empty string means anonymous object)");
	Widget	 dialog;
 
	XtSetArg(args[0], XmNallowShellResize, true);
 
	dialog = XmCreatePromptDialog(dlg->parent()->get_form_widget(), (char *)"prompt dialog", args, 1);
	
	XtVaSetValues(XtParent(dialog),
		XmNtitle, "Copy object",
		XmNdialogStyle, XmDIALOG_APPLICATION_MODAL,
		NULL);
	
	XtVaSetValues(dialog,
		XmNselectionLabelString, xm_string,
		XmNchildPlacement, XmPLACE_BELOW_SELECTION,
		NULL);
	
	XtSetArg(args[0], XmNlabelString, xm_string2);
	XtManageChild(XmCreateLabel(dialog, (char *)"simple", args, 1));
	
	XtAddCallback(dialog, XmNokCallback, createNewObjectCB, client_data);
	XtAddCallback(dialog, XmNcancelCallback, createNewObjectCB, client_data);

	XtManageChild(dialog);
	
	XmStringFree(xm_string);
	XmStringFree(xm_string2);
	
	break;
    }

    case idAcObjectRename: {
	Arg args[2];	XtSetArg(args[0], XmNallowShellResize, true);
	XmString	xm_string = OksXm::create_string("Input new unique object id:");
	Widget		dialog = XmCreatePromptDialog(dlg->parent()->get_form_widget(), (char *)"prompt dialog", args, 1);
	
	XtVaSetValues(XtParent(dialog),
		XmNtitle, "Rename object",
		XmNdialogStyle, XmDIALOG_APPLICATION_MODAL,
		NULL);
	
	XtVaSetValues(dialog, XmNselectionLabelString, xm_string, NULL);
	
	XtAddCallback(dialog, XmNokCallback, renameObjectCB, client_data);
	XtAddCallback(dialog, XmNcancelCallback, renameObjectCB, client_data);

	XtManageChild(dialog);
	XmStringFree(xm_string);
	
	break;
    }

    case idAcObjectMoveTo: {
      OksFile * old_file_h = dlg->getObject()->get_file();
      OksFile * new_file_h = dlg->parent()->get_active_data();
      if(new_file_h && old_file_h != new_file_h) {
        try {
          dlg->getObject()->set_file(new_file_h);
          dlg->set_value(idDataFile, new_file_h->get_full_file_name().c_str());
	  dlg->parent()->add_object_to_data_dlg(new_file_h);
	  dlg->parent()->remove_object_from_data_dlg(dlg->getObject(), old_file_h);
        }
        catch(oks::exception& ex) {
          OksDataEditorMainDialog::report_exception("Move Object", ex, dlg->get_form_widget());
        }
      }
    }

    break;
 }
}


    //
    // Finds relationship by widget's id
    //

OksRelationship *
OksDataEditorObjectDialog::getRelationship(int id) const
{
  const std::list<OksRelationship *> * all_rels = o->GetClass()->all_relationships();
  int j = idObjectRelationshipData + 1;

  for(std::list<OksRelationship *>::const_iterator i = all_rels->begin(); i != all_rels->end(); ++j, ++i) {
    if(j == id) {
      return *i;
    }
  }
  
  return 0;
}


    //
    // Finds attribute by widget's id
    //

OksAttribute *
OksDataEditorObjectDialog::getAttribute(int id) const
{
  const std::list<OksAttribute *> * all_attrs = o->GetClass()->all_attributes();
  int j = idObjectAttributeData + 1;

  for(std::list<OksAttribute *>::const_iterator i = all_attrs->begin(); i != all_attrs->end(); ++j, ++i) {
    if(j == id) {
      return *i;
    }
  }
  
  return 0;
}


void
OksDataEditorObjectDialog::swap(OksData *d1, OksData *d2)
{
  OksData swap;

  swap  = *d2;
  *d2	= *d1;
  *d1	= swap;
}


void
OksDataEditorObjectDialog::processItemMove(Widget list, int pos, bool up, OksData *d, OksData *& d1, OksData *& d2)
{
  OksXm::List::move_item(list, pos, up);

  int j = 0;

  d1 = d2 = 0;

  for(OksData::List::iterator i = d->data.LIST->begin(); i != d->data.LIST->end(); ++j, ++i) {
    if(j == (pos - 1)) d1 = *i;
    else if(j == pos && up == false) d2 = *i;
    else if(j == (pos - 2) && up == true) d2 = *i;
    else if(j > pos) break;
  }

  swap(d1, d2);
}


static std::string
createPopupRelationshipDescription(OksRelationship *r)
{
  std::string label("Relationship Type:\n");

  OksRelationship::CardinalityConstraint low = r->get_low_cardinality_constraint();
  OksRelationship::CardinalityConstraint high = r->get_high_cardinality_constraint();

  label += (
    low == OksRelationship::Zero && high == OksRelationship::One ? "Zero or One" :
    low == OksRelationship::Zero && high == OksRelationship::Many ? "Zero or Many" :
    low == OksRelationship::One && high == OksRelationship::Many ? "One or Many" :
    low == OksRelationship::One && high == low ? "One" :
    "Unspecified # of"
  );

  
  label += " \'";
  label += r->get_type().c_str();
  label += '\'';
  
  return label;
}


void
OksDataEditorObjectDialog::objAloneCB(Widget w, XtPointer client_data, XtPointer) 
{
  OksDataEditorObjectDialog *dlg = (OksDataEditorObjectDialog *)client_data;
  OksObject * o = dlg->getObject();
  Widget textField = XtParent(XtParent(XtParent(w)));
  long user_data = (long)OksXm::get_user_data(w);
  long user_data2 = (long)OksXm::get_user_data(textField);  
  OksRelationship * r = dlg->getRelationship(user_data2);

  OksPopupMenu::destroy(w);

  if(user_data == idAcObjectAloneSet) {
    try {
      o->SetRelationshipValue(r->get_name(), clipboardObject);
      OksData d(clipboardObject);
      std::string cvts = d.str(dlg->parent());
      dlg->get_form(idData)->set_value((int)user_data2, cvts.c_str());
      OksDataEditorMainDialog::add_object_parent_link(clipboardObject, o, r->get_name().c_str());
    }
    catch(oks::exception& ex) {
      OksDataEditorMainDialog::report_exception("Set Relationship Value", ex, dlg->get_form_widget());
    }
  }
  else if(user_data == idAcObjectAloneShow) {
    OksXm::AutoCstring objectName(XmTextFieldGetString(textField));
    dlg->createObjectByName(objectName.get());
  }
  else if(user_data == idAcObjectShowDescription) {
    dlg->show_description(0, r);
  }
  else {
    try {
      OksObject * r_obj = o->GetRelationshipValue(r->get_name())->data.OBJECT;

      o->SetRelationshipValue(r->get_name(), (OksObject *)0);
      dlg->get_form(idData)->set_value(user_data2, "");
      
      if(r_obj) {
        OksDataEditorMainDialog::remove_object_parent_link(r_obj, o, r->get_name().c_str());
      }
    }
    catch(oks::exception& ex) {
      OksDataEditorMainDialog::report_exception("Clear Relationship Value", ex, dlg->get_form_widget());
    }
  }
}


void
OksDataEditorObjectDialog::singleValueSetSensAC(Widget w, XtPointer client_data, XEvent * /*event*/, Boolean *)
{
  OksDataEditorObjectDialog * dlg(reinterpret_cast<OksDataEditorObjectDialog *>(client_data));
  bool is_read_only = OksKernel::check_read_only(dlg->getObject()->get_file());

  Boolean sensitive(is_read_only ? False : True);

  const char * name = (XmIsComboBox(w) ? XmNlist : XmNsubMenuId);
  Widget child(0);
  XtVaGetValues(w, (char *)name, &child, NULL);
  XtSetSensitive(child, sensitive);
}


void
OksDataEditorObjectDialog::objectAC(Widget w, XtPointer client_data, XEvent *event, Boolean *)
{
  OksXm::AutoCstring objectName(XmTextFieldGetString(w));

  if (((XButtonEvent *)event)->button != Button3) return;

  OksPopupMenu popup(w);

  OksDataEditorObjectDialog * dlg = (OksDataEditorObjectDialog *)client_data;
  OksRelationship * r = dlg->getRelationship(static_cast<int>(reinterpret_cast<long>(OksXm::get_user_data(w))));

  popup.add_label(createPopupRelationshipDescription(r).c_str());

  popup.add_separator();


  if(objectName.is_non_empty()) {
    Widget btn = popup.addItem(szShowObject, objectName.get(), idAcObjectAloneShow, objAloneCB, client_data);
    if(*objectName.get() == '#') XtSetSensitive(btn, false);
  }
  else {
    popup.addDisableItem(szShowObject);
  }

  popup.add_separator();


    // check if object is read-only

  bool is_read_only = OksKernel::check_read_only(dlg->getObject()->get_file());

  if(!is_read_only)
    popup.addItem(szSetObject, clipboardObject, dlg->parent(), idAcObjectAloneSet, objAloneCB, client_data);
  else
    popup.addDisableItem(szSetObject);


  if(r->get_low_cardinality_constraint() == OksRelationship::Zero && !is_read_only)
	popup.addItem("Clear", idAcObjectAloneClear, objAloneCB, client_data);
  else
	popup.addDisableItem("Clear");

  popup.add_separator();
  popup.addItem("Show Description", idAcObjectShowDescription, objAloneCB, client_data);

  popup.show((XButtonPressedEvent *)event);
}


void
OksDataEditorObjectDialog::changeListHeightCB(Widget w, XtPointer size, XtPointer)
{
  Widget list = XtParent(XtParent(XtParent(w)));

  int list_height;
  XtVaGetValues(list, XmNvisibleItemCount, &list_height, NULL);
  XtVaSetValues(list, XmNvisibleItemCount, (list_height + static_cast<int>(reinterpret_cast<long>(size))), NULL);
}

void
OksDataEditorObjectDialog::delete_by_pos(OksData *d, int pos)
{
  OksData::List * d_list = d->data.LIST;
  int j = 1;

  for(OksData::List::iterator i = d_list->begin(); i != d_list->end(); ++j, ++i) {
    if(j == pos) {
      OksData *d2 = *i;
      d_list->erase(i);
      delete d2;
      break;
    }
  }
}

OksData *
OksDataEditorObjectDialog::get_by_pos(OksData *d, int pos)
{
  OksData::List * d_list = d->data.LIST;
  int j = 1;

  for(OksData::List::iterator i = d_list->begin(); i != d_list->end(); ++j, ++i) {
    if(j == pos) {
      return *i;
    }
  }
  
  return 0;
}

void
OksDataEditorObjectDialog::objsListCB(Widget w, XtPointer client_data, XtPointer) 
{
  Widget list = XtParent(XtParent(XtParent(w)));
  long user_data = (long)OksXm::get_user_data(w);
  OksPopupMenu::destroy(w);

  OksDataEditorObjectDialog *dlg = (OksDataEditorObjectDialog *)client_data;
  OksObject *o = dlg->getObject();
  OksXm::AutoCstring list_object_name(OksXm::List::get_selected_value(list));

  if(user_data == idAcObjectsListShow) {
    dlg->createObjectByName(list_object_name.get());
    return;
  }

  long user_data2 = (long)OksXm::get_user_data(list);
  OksRelationship * r = dlg->getRelationship(user_data2);
  const char * rname = r->get_name().c_str();
  int list_object_position = OksXm::List::get_selected_pos(list);

  try {
    if(user_data == idAcObjectsListAdd) {
      o->AddRelationshipValue(rname, clipboardObject);
      OksDataEditorMainDialog::add_object_parent_link(clipboardObject, o, rname);
      return;
    }

    if(user_data == idAcObjectShowDescription) {
      dlg->show_description(0, r);
      return;
    }

    o->get_file()->lock();

    if(user_data == idAcObjectValuesListSortAscending || user_data == idAcObjectValuesListSortDescending) {
      OksData *d = o->GetRelationshipValue(rname);
      OksData d2 = *d;
      d->sort(user_data == idAcObjectValuesListSortAscending);
      if (d2 != *d)
        o->SetRelationshipValue(rname, d);    // to invoke notification
    }
    else if(user_data == idAcObjectsListRemove) {
      if(!list_object_name.get() || *list_object_name.get() == '#') {
        OksData *d(o->GetRelationshipValue(rname));
        delete_by_pos(d, list_object_position);
        o->SetRelationshipValue(rname, d);	// to invoke notification
      }
      else {
        if(OksObject * child_obj = findOksObject(list_object_name.get())) {
          o->RemoveRelationshipValue(rname, child_obj);
          OksDataEditorMainDialog::remove_object_parent_link(child_obj, o, rname);
        }
      }
    }
    else {
      OksData * d(o->GetRelationshipValue(rname));

      OksData * d1 = 0, * d2 = 0;
      processItemMove( list, list_object_position, (short)user_data == idAcObjectsListUp, d, d1, d2);
      o->SetRelationshipValue(rname, d);    // to invoke notification

      if(d1->type == OksData::object_type && d2->type == d1->type && d1->data.OBJECT && d2->data.OBJECT) {
        OksDataEditorMainDialog::swap_objects_parent_link(o, d1->data.OBJECT, d2->data.OBJECT, rname);
      }

      OksDataEditorMainDialog::ChangeNotifyFN(o, (void *)(dlg->parent()));
    }
  }
  catch(oks::exception& ex) {
    OksDataEditorMainDialog::report_exception("Change Relationship Value", ex, dlg->get_form_widget());
  }
}


void
OksDataEditorObjectDialog::add_resize_list_menu(Widget w, OksPopupMenu& popup)
{
  const char * szListEnlarge = "Show more (+1 row)";
  const char * szListAllRows = "Show all rows";
  const char * szListReduce  = "Show less (-1 row)";
  const char * szListTwoRows = "Show only two rows";

  int item_count;
  int list_height;

  XtVaGetValues(w, XmNitemCount, &item_count, XmNvisibleItemCount, &list_height, NULL);

  popup.add_separator();

  if(list_height < item_count) {
    popup.addItem(szListEnlarge, idAcObjectAny, changeListHeightCB, (XtPointer)+1);
    popup.addItem(szListAllRows, idAcObjectAny, changeListHeightCB, (XtPointer)(long)(item_count - list_height));
  }
  else {
    popup.addDisableItem(szListEnlarge);
    popup.addDisableItem(szListAllRows);
  }

  if(list_height >= 3) {
    popup.addItem(szListReduce,  idAcObjectAny, changeListHeightCB, (XtPointer)-1);
    popup.addItem(szListTwoRows, idAcObjectAny, changeListHeightCB, (XtPointer)(long)(2 - list_height));
  }
  else {
    popup.addDisableItem(szListReduce);
    popup.addDisableItem(szListTwoRows);
  }
}


void
OksDataEditorObjectDialog::objectsAC(Widget w, XtPointer client_data, XEvent *event, Boolean *)
{
  if (((XButtonEvent *)event)->button != Button3) return;

  OksXm::AutoCstring selectedObject(OksXm::List::get_selected_value(w));
  int item_position = OksXm::List::get_selected_pos(w), item_count = 0;

  XtVaGetValues(w, XmNitemCount, &item_count, NULL);

  OksPopupMenu popup(w);

  OksDataEditorObjectDialog * dlg = (OksDataEditorObjectDialog *)client_data;
  OksRelationship * r = dlg->getRelationship(static_cast<int>(reinterpret_cast<long>(OksXm::get_user_data(w))));

  popup.add_label(createPopupRelationshipDescription(r).c_str());
  popup.add_separator();


    // check if object is read-only

  bool is_read_only = OksKernel::check_read_only(dlg->getObject()->get_file());

  if(selectedObject.is_non_empty()) {
    Widget button = popup.addItem(szShowObject, selectedObject.get(), idAcObjectsListShow, objsListCB, client_data);	
    if(*selectedObject.get() == '#') XtSetSensitive(button, false);
  }
  else
    popup.addDisableItem(szShowObject);

  popup.add_separator();

  if(clipboardObject && !is_read_only)
    popup.addItem(szAddObject, clipboardObject, dlg->parent(), idAcObjectsListAdd, objsListCB, client_data);
  else
    popup.addDisableItem(szAddObject);

  if(selectedObject.get() && !is_read_only)
    popup.addItem(szRemoveObject, selectedObject.get(), idAcObjectsListRemove, objsListCB, client_data);
  else
    popup.addDisableItem(szRemoveObject);

  popup.add_separator();

  if(selectedObject.get() && !is_read_only) {
    Widget button = popup.addItem(szUpObject, selectedObject.get(), idAcObjectsListUp, objsListCB, client_data);
    if(item_position == 1) XtSetSensitive(button, false);
    button = popup.addItem(szDownObject, selectedObject.get(), idAcObjectsListDown, objsListCB, client_data);
    if(item_position == item_count) XtSetSensitive(button, false);
  }
  else {
    popup.addDisableItem(szUpObject);
    popup.addDisableItem(szDownObject);
  }

  if(!is_read_only && item_count > 1)
    {
      popup.addItem(szSortAscending, idAcObjectValuesListSortAscending, objsListCB, client_data);
      popup.addItem(szSortDescending, idAcObjectValuesListSortDescending, objsListCB, client_data);
    }
  else
    {
      popup.addDisableItem(szSortAscending);
      popup.addDisableItem(szSortDescending);
    }


  add_resize_list_menu(w, popup);

  popup.add_separator();
  popup.addItem("Show Description", idAcObjectShowDescription, objsListCB, client_data);

  popup.show((XButtonPressedEvent *)event);
}


void
OksDataEditorObjectDialog::showObjectCB(Widget, XtPointer client_data, XtPointer call_data)
{
  OksXm::AutoCstring name(OksXm::string_to_c_str(((XmListCallbackStruct *)call_data)->item));
  ((OksDataEditorObjectDialog *)client_data)->createObjectByName(name.get());
}


void
OksDataEditorObjectDialog::dlgAC(Widget w, XtPointer client_data, XEvent *event, Boolean *)
{
  if(((XButtonEvent *)event)->button != Button3) return;

  Widget btn = 0;
  
  OksDataEditorObjectDialog *dlg = (OksDataEditorObjectDialog *)client_data;
  bool is_read_only = OksKernel::check_read_only(dlg->getObject()->get_file());
  
  OksPopupMenu popup(w);

  popup.addItem("Select", idAcObjectSelect, actionCB, client_data);
  popup.add_separator();

  popup.addItem("Referenced By", idAcObjectShowReferencedBy, actionCB, client_data);
  popup.add_separator();

  OksFile * active_data = dlg->parent()->get_active_data();

  if(active_data) {
    btn = popup.addItem("Move to: ", active_data->get_full_file_name().c_str(), idAcObjectMoveTo, actionCB, client_data);
    if(active_data == dlg->getObject()->get_file() || is_read_only) XtSetSensitive(btn, false);
  }

  btn = popup.addItem("Copy", idAcObjectCopy, actionCB, client_data);
  if(!OksDataEditorMainDialog::is_valid_active_data()) XtSetSensitive(btn, false);
  
  btn = popup.addItem("Rename", idAcObjectRename, actionCB, client_data);
  if(is_read_only) XtSetSensitive(btn, false);

  popup.add_separator();

  btn = popup.addItem("Delete", idAcObjectDelete, actionCB, client_data);
  if(is_read_only) XtSetSensitive(btn, false);

  popup.show((XButtonPressedEvent *)event);
}


    //
    // Converts string in accordance with type of attribute.
    // Sets new attribute value. Sets text widget value.
    //

void
OksDataEditorObjectDialog::updateValue(OksAttribute *a, const char *value)
{
  try {
    OksData d;
    d.SetValues(value, a);
    o->SetAttributeValue(a->get_name(), &d);
  }
  catch(oks::exception& ex) {
    OksDataEditorMainDialog::report_exception("Update Attribute Value", ex, get_form_widget());
  }
}


	//
	// Converts value in accordance with new representation
	//

std::string
OksDataEditorObjectDialog::printValueWithBase(const char *value, OksData::Type data_type, int new_base, int old_base)
{
  std::ostringstream s;

  if(new_base == 16)
    s.setf(std::ios::hex, std::ios::basefield);
  else if(new_base == 8)
    s.setf(std::ios::oct, std::ios::basefield);

  if(
   data_type == OksData::u8_int_type ||
   data_type == OksData::u16_int_type ||
   data_type == OksData::u32_int_type
  )
    s << strtoul(value, 0, old_base);
  else if(
   data_type == OksData::s8_int_type ||
   data_type == OksData::s16_int_type ||
   data_type == OksData::s32_int_type
  )
    s << strtol(value, 0, old_base);
  else if(
    data_type == OksData::u64_int_type
  )
    s << strtoull(value, 0, old_base);
  else
    s << strtoll(value, 0, old_base);

  return s.str();
}


void
OksDataEditorObjectDialog::setValueCB(Widget w, XtPointer client_data, XtPointer call_data) 
{
  if(((XmSelectionBoxCallbackStruct *)call_data)->reason == XmCR_OK) {
    Widget attr_w = (Widget)OksXm::get_user_data(w);
    OksXm::AutoCstring value(XmTextGetString(XmSelectionBoxGetChild(w, XmDIALOG_VALUE_TEXT)));
    OksAttribute *a = (OksAttribute *)OksXm::get_user_data(XmSelectionBoxGetChild(w, XmDIALOG_OK_BUTTON));
    OksData::Type data_type = a->get_data_type();

    if(a->is_integer()) {
      std::string buf = printValueWithBase(value.get(), data_type, 0 /*10*/,
        static_cast<int>(reinterpret_cast<long>(OksXm::get_user_data(XmSelectionBoxGetChild(w, XmDIALOG_VALUE_TEXT))))
      );

      ((OksDataEditorObjectDialog *)client_data)->updateValue(a, buf.c_str());
    }
    else {
      ((OksDataEditorObjectDialog *)client_data)->updateValue(a, value.get());
      if(data_type == OksData::string_type) adjust_text_size(attr_w, value.get());
    }
  }
}


    //
    // The callback converts integer representation according
    // to given base (16, 10, 8)
    //

void
OksDataEditorObjectDialog::changeBaseCB(Widget w, XtPointer, XtPointer) 
{
  Widget dialog = XtParent(XtParent(XtParent(XtParent(w))));
  Widget text_w = XmSelectionBoxGetChild(dialog, XmDIALOG_VALUE_TEXT);


    // detect new base size by option menu button name

  XmString xm_btn_name = 0;

  XtVaGetValues(w, XmNlabelString, &xm_btn_name, NULL);

  OksXm::AutoCstring btn_name(OksXm::string_to_c_str(xm_btn_name));

  long new_base_size = (
    !strcmp(btn_name.get(), dec_type) ? 0 /*10*/ :
    !strcmp(btn_name.get(), oct_type) ? 8  :
    16
  );


  OksXm::AutoCstring value(XmTextGetString(text_w));

  std::string buf = printValueWithBase(value.get(),
    ((OksAttribute *)OksXm::get_user_data(XmSelectionBoxGetChild(dialog, XmDIALOG_OK_BUTTON)))->get_data_type(),
    new_base_size, static_cast<int>(reinterpret_cast<long>(OksXm::get_user_data(text_w)))
  );


    // set value in new representation

  XmTextSetString(text_w, const_cast<char *>(buf.c_str()));


    // save new type (as UserData resource of the text box)

  OksXm::set_user_data(text_w, (XtPointer)new_base_size);
}


void
OksDataEditorObjectDialog::valueAloneCB(Widget w, XtPointer client_data, XtPointer) 
{
  OksDataEditorObjectDialog
		*dlg	= (OksDataEditorObjectDialog *)client_data;
  OksObject	*o	= dlg->getObject();
  Widget	text	= XtParent(XtParent(XtParent(w)));
  OksAttribute	*a	= dlg->getAttribute(static_cast<int>(reinterpret_cast<long>(OksXm::get_user_data(text))));
  long		usr_d	= reinterpret_cast<long>(OksXm::get_user_data(w));

  OksPopupMenu::destroy(w);


  if(usr_d == idAcObjectValueAloneSet) {
    char *value;

    XtVaGetValues(text, XmNvalue, &value, NULL);


    std::string s("Input value of type \'");

    s += a->get_type();
    s += '\'';

    if(!(a->get_range()).empty()) {
      s += " in range \'";
      s += a->get_range();
      s += '\'';
    }

    s += ':';

    std::string t("Input attribute \'");

    t += a->get_name();
    t += "\' value";

    if(a->get_data_type() == OksData::string_type)
      {
        std::string new_value = OksXm::ask_text(dlg->get_form_widget(), t.c_str(), s.c_str(), (const char *)value);
        if(new_value != value)
          {
            dlg->updateValue(a, new_value.c_str());
            adjust_text_size(text, new_value.c_str());
          }
        return;
      }


    Arg args[10];
    XmString xm_string = OksXm::create_string(s.c_str());
    Widget dialog;

    size_t value_len = strlen(value);

    XtSetArg(args[0], XmNallowShellResize, true);
    XtSetArg(args[1], XmNdialogStyle, XmDIALOG_APPLICATION_MODAL);
    XtSetArg(args[2], XmNchildPlacement, XmPLACE_BELOW_SELECTION);

    dialog = XmCreatePromptDialog(dlg->get_form_widget(), (char *)"prompt dialog", args, 3);

    XtVaSetValues(XtParent(dialog), XmNtitle, t.c_str(), NULL);
    XtVaSetValues(dialog, XmNselectionLabelString, xm_string, NULL);

    {
      Widget text_w = XmSelectionBoxGetChild(dialog, XmDIALOG_VALUE_TEXT);
      XmTextSetString(
        text_w,
	(
	  (a->is_integer() && a->get_format() == OksAttribute::Hex && value_len > 2) ? (value + 2) :
	  (a->is_integer() && a->get_format() == OksAttribute::Oct && value_len > 1) ? (value + 1) :
	  value
	)
      );
      OksXm::set_user_data(text_w, (XtPointer)a->get_format());
    }

    XtAddCallback(dialog, XmNokCallback, setValueCB, client_data);
    XtAddCallback(dialog, XmNcancelCallback, setValueCB, client_data);

    OksXm::set_user_data(XmSelectionBoxGetChild(dialog, XmDIALOG_OK_BUTTON), (XtPointer)a);
    OksXm::set_user_data(dialog, (XtPointer)text);


      //
      // Integer types may have different base (dec, hex or oct)
      //

    if(a->is_integer()) {
      Widget work_area = XmCreateRowColumn(dialog, (char *)"workarea", 0, 0);
      XtManageChild(work_area);

      Widget pulldown = XmCreatePulldownMenu(work_area, (char *)"pulldown_menu", 0, 0);

      const char *items[] = {dec_type, hex_type, oct_type};
      const unsigned short num_of_base_types = sizeof(items) / sizeof(char *);
      Widget buttons[num_of_base_types];

      for(unsigned short j=0; j<num_of_base_types; j++) {
        XmString btn_name = OksXm::create_string(items[j]);	

        buttons[j] = XmCreatePushButtonGadget(pulldown, (char *)"option_menu_item", 0, 0);
        XtVaSetValues(buttons[j], XmNlabelString, btn_name, NULL);
        XtManageChild(buttons[j]);

        XtAddCallback(buttons[j], XmNactivateCallback, changeBaseCB, client_data);

        XmStringFree(btn_name);
      }

      XmString menu_name = OksXm::create_string("Base");
        Widget choosen_w = (
          a->get_format() == OksAttribute::Dec ? buttons[0] :
          a->get_format() == OksAttribute::Hex ? buttons[1] :
          buttons[2]
      );

      XtSetArg(args[0], XmNlabelString, menu_name);
      XtSetArg(args[1], XmNsubMenuId, pulldown);
      XtSetArg(args[2], XmNmenuHistory, choosen_w);

      XtManageChild(
        XmCreateOptionMenu(work_area, (char *)"simple", args, 3)
      );

      XmStringFree(menu_name);
    }
	
    XtManageChild(dialog);

    XmStringFree(xm_string);
    XtFree(value);
  }
  else if(usr_d == idAcObjectShowDescription) {
    dlg->show_description(a, 0);
  }
  else {
    OksData new_d;
    new_d.ReadFrom(a->get_init_value().c_str(), a->get_data_type(), a);

    try {
      o->SetAttributeValue(a->get_name(), &new_d);
      std::string cvts = new_d.str(a->is_integer() ? (int)a->get_format() : (int)0);
      XmTextSetString(text, const_cast<char *>(cvts.c_str()));
    }
    catch(oks::exception& ex) {
      OksDataEditorMainDialog::report_exception("Set Attribute Value", ex, dlg->get_form_widget());
    }
  }
}


void
OksDataEditorObjectDialog::valueAC(Widget w, XtPointer client_data, XEvent *event, Boolean *)
{
  if (((XButtonEvent *)event)->button != Button3) return;

  OksPopupMenu popup(w);


      // check if object is read-only

  OksDataEditorObjectDialog *dlg = (OksDataEditorObjectDialog *)client_data;
  bool is_read_only = OksKernel::check_read_only(dlg->getObject()->get_file());

  if(!is_read_only)
    popup.addItem("Set Value", idAcObjectValueAloneSet, valueAloneCB, client_data);
  else
    popup.addDisableItem("Set Value");

  popup.add_separator();

  if(!is_read_only)
    popup.addItem("Reset Default Value", idAcObjectValueAloneClear, valueAloneCB, client_data);
  else
    popup.addDisableItem("Reset Default Value");

  popup.add_separator();
  popup.addItem("Show Description", idAcObjectShowDescription, valueAloneCB, client_data);

  popup.show((XButtonPressedEvent *)event);
}

void
OksDataEditorObjectDialog::process_mv_cb(int action, const char * value, OksObject * o, OksAttribute *a, Widget list, OksDataEditorObjectDialog * dlg, int value_position)
{
  OksData *d = 0;

  try {
    d = o->GetAttributeValue(a->get_name());
  }
  catch(oks::exception & ex) {
    ers::error(OksDataEditor::InternalProblem(ERS_HERE, ex.what()));
  }

  if(action == idAcObjectValuesListAdd) {
    try {
      OksData * new_d = new OksData();
      new_d->ReadFrom(value, a->get_data_type(), a);
      new_d->check_range(a);
      d->data.LIST->push_back(new_d);

      std::string cvts = new_d->str();

      OksXm::List::add_row(list, cvts.c_str(), false);
      OksXm::List::select_row(list, cvts.c_str());
    }
    catch(oks::exception & e) {
      OksDataEditorMainDialog::report_exception("Bad User Input", e, dlg->get_form_widget());
    }
  }
  else {
    try {
      OksData  *d2 = get_by_pos(d, value_position);
      d2->ReadFrom(value, a->get_data_type(), a);
      d2->check_range(a);
      std::string cvts = d2->str();
      XmString xm_string = OksXm::create_string(cvts.c_str());

      XmListDeletePos(list, value_position);
      XmListAddItem(list, xm_string, value_position);

      OksXm::List::select_row(list, cvts.c_str());

      XmStringFree(xm_string);
    }
    catch(oks::exception & e) {
      OksDataEditorMainDialog::report_exception("Bad User Input", e, dlg->get_form_widget());
    }
  }

  o->SetAttributeValue(a->get_name(), d);  // to invoke notification
}


void
OksDataEditorObjectDialog::addValueCB(Widget w, XtPointer client_data, XtPointer call_data) 
{
  if(((XmSelectionBoxCallbackStruct *)call_data)->reason == XmCR_OK) {
    OksDataEditorObjectDialog * dlg((OksDataEditorObjectDialog *)client_data);
    OksObject * o = dlg->getObject();
    OksXm::AutoCstring value(XmTextGetString(XmSelectionBoxGetChild(w, XmDIALOG_VALUE_TEXT)));
    int user_data = static_cast<int>(reinterpret_cast<long>(OksXm::get_user_data(XmSelectionBoxGetChild(w, XmDIALOG_OK_BUTTON))));
    Widget list = (Widget)OksXm::get_user_data(w);
    int value_position = static_cast<int>(reinterpret_cast<long>(OksXm::get_user_data(XmSelectionBoxGetChild(w, XmDIALOG_CANCEL_BUTTON))));
    OksAttribute *a = (OksAttribute *)OksXm::get_user_data(XmSelectionBoxGetChild(w, XmDIALOG_HELP_BUTTON));

    process_mv_cb(user_data, value.get(), o, a, list, dlg, value_position);
  }
}


void
OksDataEditorObjectDialog::enumValueCB(Widget w, XtPointer cld, XtPointer)
{
  OksXm::AutoCstring value(OksXm::List::get_selected_value(w));
  OksObject * o = ((OksDataEditorObjectDialog *)cld)->getObject();
  Widget listw = (Widget)OksXm::get_user_data(w);
  OksDataEditorObjectDialog * dlg = (OksDataEditorObjectDialog *)cld;
  OksAttribute * a = (OksAttribute *)OksXm::get_user_data(dlg->get_form_widget());
  long user_data = (long)OksXm::get_user_data(XtParent(XtParent(w)));
  std::string cvts;			
  OksData * d = 0;

  try {
    d = o->GetAttributeValue(a->get_name());
  }
  catch(oks::exception & ex) {
    ers::error(OksDataEditor::InternalProblem(ERS_HERE, ex.what()));
  }

  if(user_data == idAcObjectValuesListAdd) {
    OksData *new_d = new OksData();

    new_d->ReadFrom(value.get(), a->get_data_type(), a);
    d->data.LIST->push_back(new_d);

    cvts = new_d->str();

    OksXm::List::add_row(listw, cvts.c_str());
  }
  else {
    int value_position = OksXm::List::get_selected_pos(listw);
    OksData *d2 = get_by_pos(d, value_position);

    d2->ReadFrom(value.get(), a->get_data_type(), a);
    cvts = d2->str();

    XmString xm_string = OksXm::create_string(cvts.c_str());

    XmListDeletePos(listw, value_position);
    XmListAddItem(listw, xm_string, value_position);

    XmStringFree(xm_string);
  }
  
    // call notification
  
  OksDataEditorMainDialog::ChangeNotifyFN(o, (void *)dlg->parent());
  

  OksXm::List::select_row(listw, cvts.c_str());

  Widget top = XtParent(XtParent(XtParent(w)));
  XtUnmanageChild(top);
  XtDestroyWidget(top);
}

void
OksDataEditorObjectDialog::valuesListCB(Widget w, XtPointer client_data, XtPointer) 
{
  OksObject	*o = ((OksDataEditorObjectDialog *)client_data)->getObject();
  OksDataEditorObjectDialog * dlg = (OksDataEditorObjectDialog *)client_data;
  Widget	form_w = dlg->get_form_widget();
  Widget	listw = XtParent(XtParent(XtParent(w)));
  int		value_position = OksXm::List::get_selected_pos(listw);
  OksXm::AutoCstring value(OksXm::List::get_selected_value(listw));
  long		user_data = (long)OksXm::get_user_data(w);
  OksAttribute	*a = ((OksDataEditorObjectDialog *)client_data)->getAttribute(static_cast<int>(reinterpret_cast<long>(OksXm::get_user_data(listw))));
  OksData	*d = 0;

  OksPopupMenu::destroy(w);

  try {
    d = o->GetAttributeValue(a->get_name());
  }
  catch(oks::exception & ex) {
    ers::error(OksDataEditor::InternalProblem(ERS_HERE, ex.what()));
  }

  switch(user_data) {
    case idAcObjectValuesListAdd:
    case idAcObjectValuesListModify:
      if(a->get_data_type() != OksData::enum_type && a->get_data_type() != OksData::class_type) {
        Arg args[5];
        std::string s = ( user_data == idAcObjectValuesListAdd ? "Input" : "Change" );

        std::string s1(s);
        s1 += " value of type \'";
        s1 += a->get_type();
        s1 += '\'';

        if(!(a->get_range()).empty()) {
          s1 += " in range \'";
          s1 += a->get_range();
          s1 += '\'';
        }

        s1 += ':';


        std::string t(s);
        t += " attribute \'";
        t += a->get_name();
        t += "\' value";

        if(a->get_data_type() == OksData::string_type)
          {
            std::string new_value = OksXm::ask_text(dlg->get_form_widget(), t.c_str(), s1.c_str(), value.get());
            process_mv_cb(user_data, new_value.c_str(), o, a, listw, dlg, value_position);
            return;
          }

        XmString xm_string = OksXm::create_string(s1.c_str());

        XtSetArg(args[0], XmNallowShellResize, true);
        XtSetArg(args[1], XmNdialogStyle, XmDIALOG_APPLICATION_MODAL);

        Widget dialog = XmCreatePromptDialog(form_w, (char *)"prompt dialog", args, 2);

        XtVaSetValues(XtParent(dialog), XmNtitle, t.c_str(), NULL);
        XtVaSetValues(dialog, XmNselectionLabelString, xm_string, NULL);

        if(user_data == idAcObjectValuesListModify)
          XmTextSetString(XmSelectionBoxGetChild(dialog, XmDIALOG_VALUE_TEXT), value.get());

        XtAddCallback(dialog, XmNokCallback, addValueCB, client_data);
        XtAddCallback(dialog, XmNcancelCallback, addValueCB, client_data);


        OksXm::set_user_data(XmSelectionBoxGetChild(dialog, XmDIALOG_OK_BUTTON), (XtPointer)user_data);
        OksXm::set_user_data(XmSelectionBoxGetChild(dialog, XmDIALOG_CANCEL_BUTTON), (XtPointer)(long)value_position);
        OksXm::set_user_data(XmSelectionBoxGetChild(dialog, XmDIALOG_HELP_BUTTON), (XtPointer)a);
        OksXm::set_user_data(dialog, (XtPointer)listw);

        XtManageChild(dialog);
        XmStringFree(xm_string);
        break;
      }
      else {
        Widget kid[6];
        Arg args[10];
        XmString listLabelString = OksXm::create_string("Enumeration list:");
        XmString textLabelString = OksXm::create_string("double click to select an item");

        XtSetArg (args[0], XmNallowShellResize, True);
        Widget shell = XtCreatePopupShell("Select Enumeration Type", topLevelShellWidgetClass, form_w, args, 1);

        XtSetArg(args[0], XmNdialogType, XmDIALOG_SELECTION);
        XtSetArg(args[1], XmNlistLabelString, listLabelString);
        XtSetArg(args[2], XmNselectionLabelString, textLabelString);
        Widget dialog = XmCreateSelectionBox (shell, (char *)"Select Enumeration Item", args, 3);
        XtVaSetValues(XtParent(dialog), XmNdialogStyle, XmDIALOG_APPLICATION_MODAL, NULL);

        kid[0] = XmSelectionBoxGetChild(dialog, XmDIALOG_TEXT);
        kid[1] = XmSelectionBoxGetChild(dialog, XmDIALOG_SEPARATOR);
        kid[2] = XmSelectionBoxGetChild(dialog, XmDIALOG_OK_BUTTON);
        kid[3] = XmSelectionBoxGetChild(dialog, XmDIALOG_CANCEL_BUTTON);
        kid[4] = XmSelectionBoxGetChild(dialog, XmDIALOG_APPLY_BUTTON);
        kid[5] = XmSelectionBoxGetChild(dialog, XmDIALOG_HELP_BUTTON);
        XtUnmanageChildren(kid, 6);

        Widget dlg_list = XmSelectionBoxGetChild(dialog, XmDIALOG_LIST);

        if(a->get_data_type() == OksData::enum_type)
          {
            Oks::Tokenizer t(a->get_range(), " ,");
            std::string token;

            while(!(token = t.next()).empty()) {
              OksXm::List::add_row(dlg_list, token.c_str());
            }
          }
        else
          {
            for(const auto& x : o->GetClass()->get_kernel()->classes())
              OksXm::List::add_row(dlg_list, x.first);
          }

        OksXm::set_user_data(dlg_list, (XtPointer)listw);
        OksXm::set_user_data(dialog, (XtPointer)user_data);
        OksXm::set_user_data(form_w, (XtPointer)a);

        XtAddCallback(dlg_list, XmNdefaultActionCallback, enumValueCB, client_data);

        XtManageChild(dialog);
        XtRealizeWidget(XtParent(dialog));  
        XtPopup(XtParent(dialog), XtGrabNone);

        XmStringFree(listLabelString);
        XmStringFree(textLabelString);
      }

      break;


    case idAcObjectValuesListDelete:
      delete_by_pos(d, value_position);
      try {
        o->SetAttributeValue(a->get_name(), d);
      }
      catch(oks::exception& ex) {
        OksDataEditorMainDialog::report_exception("Remove Attribute Value", ex, dlg->get_form_widget());
      }
      break;

    case idAcObjectValuesListSortAscending:
    case idAcObjectValuesListSortDescending:
      try {
        OksData *d = o->GetAttributeValue(a->get_name());
        OksData d2 = *d;
        d->sort(user_data == idAcObjectValuesListSortAscending);
        if (d2 != *d)
          o->SetAttributeValue(a->get_name(), d);    // to invoke notification
      }
      catch(oks::exception& ex) {
        OksDataEditorMainDialog::report_exception("Sort Attribute Value", ex, dlg->get_form_widget());
      }
      break;

    case idAcObjectValuesListUp:
    case idAcObjectValuesListDown:
      try {
        o->get_file()->lock();
        OksData * d1 = 0, * d2 = 0;
        processItemMove(listw, value_position, (short)user_data == idAcObjectValuesListUp, d, d1, d2);
        o->SetAttributeValue(a->get_name(), d);
      }
      catch(oks::exception& ex) {
        OksDataEditorMainDialog::report_exception("Move Attribute Value", ex, dlg->get_form_widget());
      }

      break;

    case idAcObjectShowDescription:
      ((OksDataEditorObjectDialog *)client_data)->show_description(a, 0);
      break;
  }
}


void
OksDataEditorObjectDialog::valuesAC(Widget w, XtPointer client_data, XEvent *event, Boolean *)
{
  if (((XButtonEvent *)event)->button != Button3) return;

  OksXm::AutoCstring selectedValue(OksXm::List::get_selected_value(w));
  int item_position = OksXm::List::get_selected_pos(w), item_count = 0;

  XtVaGetValues(w, XmNitemCount, &item_count, NULL);

  OksPopupMenu popup(w);


    // check if object is read-only

  OksDataEditorObjectDialog *dlg = (OksDataEditorObjectDialog *)client_data;
  bool is_read_only = OksKernel::check_read_only(dlg->getObject()->get_file());

  if(selectedValue.get() && !is_read_only)
    popup.addItem(szModifyValue, selectedValue.get(), idAcObjectValuesListModify, valuesListCB, client_data);
  else
    popup.addDisableItem(szModifyValue);

  if(!is_read_only)
    popup.addItem(szAddValue, idAcObjectValuesListAdd, valuesListCB, client_data);	
  else
    popup.addDisableItem(szAddValue);

  if(selectedValue.get() && !is_read_only)
    popup.addItem(szDeleteValue, selectedValue.get(), idAcObjectValuesListDelete, valuesListCB, client_data);	
  else
    popup.addDisableItem(szDeleteValue);	

  popup.add_separator();

  if(selectedValue.get() && !is_read_only) {
    Widget button = popup.addItem(szUpValue, selectedValue.get(), idAcObjectValuesListUp, valuesListCB, client_data);
    if(item_position == 1) XtSetSensitive(button, false);
    button = popup.addItem(szDownValue, selectedValue.get(), idAcObjectValuesListDown, valuesListCB, client_data);
    if(item_position == item_count) XtSetSensitive(button, false);
  }
  else {
    popup.addDisableItem(szUpValue);
    popup.addDisableItem(szDownValue);
  }

  if(!is_read_only && item_count > 1)
    {
      popup.addItem(szSortAscending, idAcObjectValuesListSortAscending, valuesListCB, client_data);
      popup.addItem(szSortDescending, idAcObjectValuesListSortDescending, valuesListCB, client_data);
    }
  else
    {
      popup.addDisableItem(szSortAscending);
      popup.addDisableItem(szSortDescending);
    }

  add_resize_list_menu(w, popup);

  popup.add_separator();
  popup.addItem("Show Description", idAcObjectShowDescription, valuesListCB, client_data);

  popup.show((XButtonPressedEvent *)event);
}

void
OksDataEditorObjectDialog::changeClassTypeCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  OksDataEditorObjectDialog * dlg(reinterpret_cast<OksDataEditorObjectDialog *>(client_data));
  XmComboBoxCallbackStruct * cb(reinterpret_cast<XmComboBoxCallbackStruct *>(call_data));
  OksAttribute * a(reinterpret_cast<OksAttribute *>(OksXm::get_user_data(w)));
  OksXm::AutoCstring value(OksXm::string_to_c_str(cb->item_or_text));

  try {
    OksData d; d.SetValues(value.get(), a);
    dlg->getObject()->SetAttributeValue(a->get_name(), &d);
  }
  catch(oks::exception& ex) {
    OksDataEditorMainDialog::report_exception("Change Class Value", ex, dlg->get_form_widget());
  }
}

void
OksDataEditorObjectDialog::changeEnumTypeCB(Widget w, XtPointer data, XtPointer)
{
  OksDataEditorObjectDialog * dlg(reinterpret_cast<OksDataEditorObjectDialog *>(data));
  OksAttribute *a = (OksAttribute *)OksXm::get_user_data(XtParent(w));
  XmString s; XtVaGetValues(w, XmNlabelString, &s, NULL);
  OksXm::AutoCstring value(OksXm::string_to_c_str(s));

  try {
    OksData d; d.SetValues(value.get(), a);
    dlg->getObject()->SetAttributeValue(a->get_name(), &d);
  }
  catch(oks::exception& ex) {
    OksDataEditorMainDialog::report_exception("Change Enum Value", ex, dlg->get_form_widget());
  }

  XmStringFree(s);	/* TO BE TESTED ! */
}


static void
setDlgHeaderItemValue(OksDataEditorObjectDialog *dlg, short id, const char *value)
{
  OksXm::TextField::set_not_editable(dlg->get_widget(id));
  dlg->set_value(id, value);
}


	//
	// Set number of list visible lines
	// minimum is 2, maximum is 8
	//

static void
setListVisibleLines(Widget w, size_t num)
{
  XtVaSetValues(
    w,
    XmNvisibleItemCount,
    (
      num < 2 ? 2 :
      num > 8 ? 8 :
      num
    ),
    NULL
  );
}


void
OksDataEditorObjectDialog::refresh_file_name()
{
    // set title

  {
    std::string title_end(o->GetId());

    if(OksKernel::check_read_only(getObject()->get_file()))
      title_end.append(" (read-only)");

    setTitle("Oks Object", o->GetClass()->get_name().c_str(), title_end.c_str());
  }

    // set name of file

  set_value(idDataFile, o->get_file()->get_full_file_name().c_str());
}

//void
//OksDataEditorObjectDialog::fileAC(Widget w, XtPointer client_data, XEvent *event, Boolean *)
//{
//  if (((XButtonEvent *)event)->button != Button3) return;
//
//  OksDataEditorObjectDialog * dlg((OksDataEditorObjectDialog *)client_data);
//
//  if(dlg->getObject()->get_file()->get_repository() == OksFile::GlobalRepository) {
//    OksPopupMenu popup(w);
//    popup.addItem("Checkout", idAcObjectFileCheckout, fileCB, client_data);
//    popup.show((XButtonPressedEvent *)event);
//  }
//  else {
//    std::cout << "File " << dlg->getObject()->get_file()->get_full_file_name() << " is already in user repository" << std::endl;
//  }
//}

//void
//OksDataEditorObjectDialog::fileCB(Widget w, XtPointer client_data, XtPointer)
//{
//  OksDataEditorObjectDialog * dlg = (OksDataEditorObjectDialog *)client_data;
//  OksPopupMenu::destroy(w);
//  dlg->parent()->checkout_file(dlg->getObject()->get_file());
//}


OksDataEditorObjectDialog::OksDataEditorObjectDialog(OksObject *obj, OksDataEditorMainDialog *p) :
  OksDataEditorDialog	(p),
  o			(obj)
{
  static Pixmap pixmap = 0;
  const OksClass *obj_class = o->GetClass();
  const char * class_name = obj_class->get_name().c_str();
  const char * object_id = o->GetId().c_str();

  if(!pixmap)
    pixmap = OksXm::create_color_pixmap(get_form_widget(), data_editor_oks_object_xpm);

  createOksDlgHeader(&pixmap, dlgAC, (void *)this, idObject, "Object ID:", idClass, "Class:", idDataFile, "Data File:");

  {
    Widget data_file_w = get_widget(idDataFile);

    OksXm::TextField::set_not_editable(data_file_w);

//    if(o->get_file()->get_repository() != OksFile::NoneRepository) {
//      XtAddEventHandler(data_file_w, ButtonPressMask, False, fileAC, (XtPointer)this);
//    }
  }

  refresh_file_name();
  setIcon(pixmap);

  setDlgHeaderItemValue(this, idObject, object_id);
  setDlgHeaderItemValue(this, idClass, class_name);

  add_form(idData, "Data:");
  attach_right(idData);

  OksForm	*dataForm = get_form(idData);
  size_t	count = idObjectAttributeData;
  Widget	w;

  dataForm->add_label(idAttributes, "Attributes:");
  if(obj_class->number_of_all_attributes()) {
    const std::list<OksAttribute *> * attrs = obj_class->all_attributes();

    if(attrs) for(const auto& a : *attrs) {
      const char * name = a->get_name().c_str();
      OksData::Type type = a->get_data_type();
      OksData * d = 0;

      try {
        d = o->GetAttributeValue(name);
      }
      catch(oks::exception & ex) {
        ers::error(OksDataEditor::InternalProblem(ERS_HERE, ex.what()));
      }

      ++count;

      if( a->get_is_multi_values() ) {
        w = dataForm->add_list(count, name);

        dataForm->attach_right(count);

        XtAddEventHandler(w, ButtonPressMask, False, valuesAC, (XtPointer)this);
        OksXm::set_user_data(w, (XtPointer)count);

        OksData::List * d_list = d->data.LIST;

        setListVisibleLines(w, d_list->size());

        for (const auto& i2 : *d_list)
          dataForm->set_value(count, i2->str(a->is_integer() ? (int)a->get_format() : (int)0).c_str(), 0, false);
      }
      else {
        if( (type == OksData::bool_type) || (type == OksData::class_type) || (type == OksData::enum_type) ) {
          std::list<const char *> types;

          if(type == OksData::bool_type) {
            types.push_back("true");
            types.push_back("false");
          }
          else if(type == OksData::class_type) {
            for (const auto& i : obj_class->get_kernel()->classes())
              types.push_back(i.first);
          }
          else {
            Oks::Tokenizer t(a->get_range(), " ,");
            std::string token2;

            while(!(token2 = t.next()).empty()) {
	      std::string * token = new std::string(token2);
              types.push_back(token->c_str());
              toBeDeleted.push_back(token);
            }
          }

          types.push_back(name);

          if(type == OksData::class_type) {
            w = dataForm->add_combo_box(count, &types);
            XtAddCallback(w, XmNselectionCallback, changeClassTypeCB, (XtPointer)this);
            OksXm::set_user_data(w, (XtPointer)a);
          }
          else {
            w = dataForm->add_option_menu(count, &types);
            Widget pull_down_w;
            XtVaGetValues(w, XmNsubMenuId, &pull_down_w, NULL);
            OksXm::set_user_data(pull_down_w, (XtPointer)a);
            dataForm->set_option_menu_cb(count, changeEnumTypeCB, (XtPointer)this);
          }

            // make disabled or sensible depending on file write permission

          XtAddEventHandler(w, ButtonPressMask, False, singleValueSetSensAC, (XtPointer)this);

        }
        else {
          if (is_multiline_string(*d))
            w = dataForm->add_text(count, name);
          else
            w = dataForm->add_text_field(count, name);

          short columns = (
            type == OksData::s8_int_type ?  4 :
            type == OksData::u8_int_type ?  4 :
            type == OksData::s16_int_type ? 6 :
            type == OksData::u16_int_type ? 5 :
            type == OksData::s32_int_type ? 11 :
            type == OksData::u32_int_type ? 10 :
            type == OksData::s64_int_type ? 20 :
            type == OksData::u64_int_type ? 20 :
            type == OksData::float_type ?  12 :
            type == OksData::double_type ? 12 :
            type == OksData::date_type ?   11 :
            type == OksData::time_type ?   20 :
            type == OksData::string_type ? 12 :
            20
          );

          OksXm::TextField::set_not_editable(w);

          XtVaSetValues(w, XmNresizeWidth, true, XmNcolumns, columns, NULL);


          XtAddEventHandler(w, ButtonPressMask, False, valueAC, (XtPointer)this);
          OksXm::set_user_data(w, (XtPointer)count);
          // dataForm->attach_right(count);
        }

        std::string cvts = d->str(a->is_integer() ? (int)a->get_format() : (int)0);
        dataForm->set_value(count, cvts.c_str());
        if(type == OksData::string_type) adjust_text_size(w, cvts.c_str());
      }

      p_label_tips.push_back(OksXm::show_text_as_tips(w, make_description(a).c_str()));
    }
  }


  if(obj_class->number_of_all_relationships()) {
    dataForm->add_separator();
    count = idObjectRelationshipData;

    dataForm->add_label(idRelationships, "Relationships:");
    const std::list<OksRelationship *> * all_rels = obj_class->all_relationships();

    if (all_rels) for(const auto& r : *all_rels) {
      const char * name = r->get_name().c_str();
      OksData * d = 0;
		
      try {
        d = o->GetRelationshipValue(name);
      }
      catch(oks::exception & ex) {
        ers::error(OksDataEditor::InternalProblem(ERS_HERE, ex.what()));
      }

      if(r->get_high_cardinality_constraint() == OksRelationship::Many) {
        w = dataForm->add_list(++count, name);
        dataForm->attach_right(count);
        XtAddCallback (w, XmNdefaultActionCallback, showObjectCB, (XtPointer)this);
        
        XtAddEventHandler(w, ButtonPressMask, False, objectsAC, (XtPointer)this);
        OksXm::set_user_data(w, (XtPointer)count);
        
        OksData::List * d_list = d->data.LIST;
        setListVisibleLines(w, d_list->size());

        for(OksData::List::iterator i2 = d_list->begin(); i2 != d_list->end(); ++i2) {
          OksData *d2 = *i2;
          if(d2->type == OksData::object_type && d2->data.OBJECT) {
            if(parent()->is_dangling(d2->data.OBJECT)) {
              std::ostringstream text;
              text << "the object " << (void *)d2->data.OBJECT << " doesn't exist\n"
                      "and is referenced by object \"" << class_name << "@" << object_id << "\"\n"
                      "through relationship \"" << name << "\".\n"
                      "Possibly it was deleted";
              ers::error(OksDataEditor::InternalProblem(ERS_HERE, text.str().c_str()));
              continue;
            }
          }
        	
          std::string cvts = d2->str(parent());
          dataForm->set_value(count, cvts.c_str(), 0, false);
        }
      }
      else {
        w = dataForm->add_text_field(++count, name);
        OksXm::TextField::set_not_editable(w);
//        dataForm->attach_right(count);

        XtVaSetValues(w, XmNresizeWidth, true, NULL);

        if(d->type == OksData::object_type && d->data.OBJECT) {
          if(parent()->is_dangling(d->data.OBJECT)) {
            std::ostringstream text;
            text << "the object " << (void *)d->data.OBJECT << " doesn't exist\n"
                    "and is referenced by object \"" << class_name << "@" << object_id << "\"\n"
                    "through relationship \"" << name << "\".\n"
                    "Possibly it was deleted";
            ers::error(OksDataEditor::InternalProblem(ERS_HERE, text.str().c_str()));
          }
          else {
            std::string cvts = d->str(parent());
            dataForm->set_value(count, cvts.c_str());
          }
        }
        else if(d->type == OksData::uid_type || d->type == OksData::uid2_type) {
          std::string cvts = d->str(parent());
          dataForm->set_value(count, cvts.c_str());
        }

        XtAddEventHandler(w, ButtonPressMask, False, objectAC, (XtPointer)this);
        OksXm::set_user_data(w, (XtPointer)count);
      }

      p_label_tips.push_back(OksXm::show_text_as_tips(w, make_description(r).c_str()));
    }
  }

  dataForm->attach_bottom(0);

  add_separator();

  w = addCloseButton();
  XtAddCallback (w, XmNactivateCallback, closeCB, (XtPointer)this);
  OksXm::set_close_cb(XtParent(get_form_widget()), closeCB, (void *)this);

  attach_bottom(idData);

  show();

  dataForm->set_min_height();

  setMinSize();
  setCascadePos();
}


OksDataEditorObjectDialog::~OksDataEditorObjectDialog()
{
  parent()->remove_dlg(this);

  while(!toBeDeleted.empty()) {
    std::string * s = toBeDeleted.front();
    toBeDeleted.pop_front();
    delete s;
  }

  while(!p_label_tips.empty()) {
    OksXm::TipsInfo * p = p_label_tips.front();
    p_label_tips.pop_front();
    delete p;
  }
}


void
OksDataEditorObjectDialog::refresh() const
{
  unsigned short count = 0;

  const OksClass * c = o->GetClass();

  if(c->number_of_all_attributes()) {
    const std::list<OksAttribute *> * attrs = c->all_attributes();
  
    if(attrs) for(std::list<OksAttribute *>::const_iterator i = attrs->begin(); i != attrs->end(); ++i) {
      if((*i)->get_is_multi_values()) {
        refresh_multi_value_attribute(count++, *i);
      }
      else {
        refresh_single_value_attribute(count++, *i);
      }
    }
  }

  if(c->number_of_all_relationships()) {
    const std::list<OksRelationship *> * all_rels = c->all_relationships();

    if(all_rels) for(std::list<OksRelationship *>::const_iterator i = all_rels->begin(); i != all_rels->end(); ++i) {
      if((*i)->get_high_cardinality_constraint() == OksRelationship::Many) {
        refresh_multi_value_relationship(count++);
      }
      else {
        refresh_single_value_relationship(count++);
      }
    }
  }
}

void
OksDataEditorObjectDialog::refresh_single_value_attribute(unsigned short count, const OksAttribute *a) const
{
  OksDataInfo data_info(count, a);
  OksData *d(o->GetAttributeValue(&data_info));

  std::string cvts = d->str(a->is_integer() ? (int)a->get_format() : (int)0);
  get_form(idData)->set_value(count + 1 + idObjectAttributeData, cvts.c_str());
}

void
OksDataEditorObjectDialog::refresh_single_value_relationship(unsigned short count) const
{
  size_t      num_of_attrs = o->GetClass()->number_of_all_attributes();
  OksDataInfo data_info(count, (const OksRelationship *)0);
  OksData *   d(o->GetRelationshipValue(&data_info));

  if(d->type == OksData::object_type && d->data.OBJECT && parent()->is_dangling(d->data.OBJECT)) return;

  std::string cvts = d->str(o->GetClass()->get_kernel());
  get_form(idData)->set_value(count - num_of_attrs + 1 + idObjectRelationshipData, cvts.c_str());
}


void
OksDataEditorObjectDialog::refresh_multi_value_attribute(unsigned short count, const OksAttribute *a) const
{
  size_t id = count + 1 + idObjectAttributeData;
  OksForm * f = get_form(idData);

  OksDataInfo data_info(count, a);
  OksData *d(o->GetAttributeValue(&data_info));

  OksData::List * d_list = d->data.LIST;

  XmListDeleteAllItems(f->get_widget(id));

  for(OksData::List::iterator i2 = d_list->begin(); i2 != d_list->end(); ++i2) {
    std::string cvts = (*i2)->str(a->is_integer() ? (int)a->get_format() : (int)0);
    f->set_value(id, cvts.c_str(), 0, false);
  }
}


void
OksDataEditorObjectDialog::refresh_multi_value_relationship(unsigned short count) const
{
  size_t num_of_attrs = o->GetClass()->number_of_all_attributes();
  size_t id = count - num_of_attrs + 1 + idObjectRelationshipData;
  OksForm * f = get_form(idData);

  OksDataInfo data_info(count, (const OksRelationship *)0);
  OksData *d(o->GetRelationshipValue(&data_info));

  OksData::List * d_list = d->data.LIST;

  XmListDeleteAllItems(f->get_widget(id));

  for(OksData::List::iterator i2 = d_list->begin(); i2 != d_list->end(); ++i2) {
    OksData *d2 = *i2;

    if(
      d2->type == OksData::object_type &&
      d2->data.OBJECT &&
      parent()->is_dangling(d2->data.OBJECT)
    ) continue;

    std::string cvts = d2->str(o->GetClass()->get_kernel());
    f->set_value(id, cvts.c_str(), 0, false);
  }
}


void
OksDataEditorObjectDialog::show_description(const OksAttribute * a, const OksRelationship * r) const
{
  std::ostringstream s;

  if(a) {
    s << make_description(a);
  }
  else if(r) {
    s << make_description(r);
  }
  
  s << std::ends;

  std::string s2 = s.str();

  OksXm::show_info(
    get_form_widget(),
    (a ? "Attribute Description" : "Relationship Description"),
    data_editor_oks_object_xpm, 
    s2.c_str(),
    "OK"
  );
}
