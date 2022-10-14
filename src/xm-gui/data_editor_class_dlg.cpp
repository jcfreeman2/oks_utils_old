#include <iostream>

#include <oks/class.h>
#include <oks/object.h>
#include <oks/relationship.h>
#include <oks/xm_utils.h>
#include <oks/xm_popup.h>

#include <Xm/Xm.h>
#include <Xm/List.h>
#include <Xm/Matrix.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/SelectioB.h>
#include <Xm/FileSB.h>


#include "data_editor_class_dlg.h"
#include "data_editor_main_dlg.h"
#include "data_editor_search_panel.h"
#include "data_editor_exceptions.h"
#include "data_editor.h"


extern OksObject *clipboardObject;


  // NOTE: this add_row is only used by MainWindow to add newly created object;
  //       normally much more efficient next add_row() is used to build/redraw window

void
OksDataEditorClassDialog::add_row(Widget matrix, const OksObject *o)
{
  const OksClass * c = o->GetClass();
  unsigned short sizeOfMatrix = c->number_of_all_attributes() + c->number_of_all_relationships();
  String * row = new String[sizeOfMatrix + 2];
  OksDataInfo odi(0, (const OksAttribute *)0);
  size_t numberOfRows = XbaeMatrixNumRows(matrix);

  row[0] = const_cast<char *>(o->GetId().c_str());

  if(sizeOfMatrix > 0) {
    std::list<OksAttribute *>::const_iterator attrs = c->all_attributes()->begin();

    do {
      const OksAttribute * a = (odi.offset < c->number_of_all_attributes()) ? *attrs : 0;
      ++attrs;

      std::string s;

      if(a) { s = o->GetAttributeValue(&odi)->str(a->is_integer() ? (int)a->get_format() : (int)0); }
      else  { s = o->GetAttributeValue(&odi)->str(c->get_kernel()); }

      ++odi.offset;

      row[odi.offset] = new char [s.length() + 1];
      strcpy(row[odi.offset], s.c_str());
    } while(odi.offset < sizeOfMatrix);
  }

  row[sizeOfMatrix + 1] = const_cast<char *>("");

  XbaeMatrixAddRows(matrix, numberOfRows, row, 0, 0, 1);
  XbaeMatrixSetRowUserData(matrix, numberOfRows, (XtPointer)o);

  for(int j=1; j<=sizeOfMatrix; j++) delete [] row[j];

  delete [] row;
}


void
ObjectTable::add_row(const OksObject *o, const OksClass * c, size_t sizeOfMatrix, std::list< ObjectRow * >& result)
{
  ObjectRow * row = new ObjectRow(o, sizeOfMatrix);

  if(sizeOfMatrix > 0) {

      // case when objects belong to different classes
    if(c) {
      for(std::list<OksAttribute *>::const_iterator ai = c->all_attributes()->begin(); ai != c->all_attributes()->end(); ++ai) {
	row->p_values.push_back(o->GetAttributeValue((*ai)->get_name())->str((*ai)->is_integer() ? (int)(*ai)->get_format() : (int)0));
      }
      for(std::list<OksRelationship *>::const_iterator ri = c->all_relationships()->begin(); ri != c->all_relationships()->end(); ++ri) {
	row->p_values.push_back(o->GetRelationshipValue((*ri)->get_name())->str(c->get_kernel()));
      }
    }

      // case when all objects belong to the same class
    else {
      c = o->GetClass();
      std::list<OksAttribute *>::const_iterator attrs = c->all_attributes()->begin();
      OksDataInfo odi(0, (const OksAttribute *)0);

      do {
        const OksAttribute * a = (odi.offset < c->number_of_all_attributes()) ? *attrs : 0;
        if(a) { row->p_values.push_back(o->GetAttributeValue(&odi)->str(a->is_integer() ? (int)a->get_format() : (int)0)); }
        else  { row->p_values.push_back(o->GetAttributeValue(&odi)->str(c->get_kernel())); }
        ++attrs;
      } while(++odi.offset < sizeOfMatrix);
    }
  }

  result.push_back(row);
}

void
ObjectTable::show_rows(Widget matrix, size_t sizeOfMatrix, Widget pickup_bg_color, std::list< ObjectRow * >& result)
{
  std::string selectedValue;

  if (const char * s = OksXm::Matrix::get_selected_value(matrix))
    selectedValue = s;

  XbaeMatrixDeleteRows(matrix, 0, XbaeMatrixNumRows(matrix));
  XtVaSetValues(matrix, XmNrowUserData, 0, NULL);

  if (result.size() > 0)
    {
      const int num_of_rows(result.size());
      String ** cells = new String *[num_of_rows];
      XtPointer * user_data = (XtPointer *) XtCalloc(num_of_rows, sizeof(XtPointer));

      int idx(0);

      for (const auto& row : result)
        {
          cells[idx] = new String[sizeOfMatrix + 2];
          cells[idx][0] = const_cast<String>(row->p_obj->GetId().c_str());
          for (unsigned int j = 0; j < sizeOfMatrix; ++j)
            cells[idx][j + 1] = const_cast<String>(row->p_values[j].c_str());
          cells[idx][sizeOfMatrix + 1] = const_cast<String>("");
          user_data[idx++] = (XtPointer)row->p_obj;
        }

      XtVaSetValues(matrix, XmNrows, num_of_rows, NULL);
      XtVaSetValues(matrix, XmNcells, cells, NULL);
      XtVaSetValues(matrix, XmNrowUserData, user_data, NULL);

      for (auto i : result)
        delete i;

      result.clear();
    }

  OksXm::Matrix::adjust_column_widths(matrix);
  OksXm::Matrix::set_cell_background(matrix, pickup_bg_color);
  OksXm::Matrix::set_visible_height(matrix);
  OksXm::Matrix::set_widths(matrix, 3);

  if (!selectedValue.empty())
    OksXm::Matrix::select_row(matrix, selectedValue.c_str());
}

Widget
ObjectTable::create_matrix(OksForm& f, const char * name, unsigned short id, const OksClass * c, OksDataEditorMainDialog * parent)
{
  unsigned short sizeOfMatrix(c->number_of_all_attributes() + c->number_of_all_relationships());
  unsigned char * label_alinments = new unsigned char[sizeOfMatrix + 2];
  String * labels = new String[sizeOfMatrix + 2];
  short * widths = new short[sizeOfMatrix + 2];

  for (unsigned short i = 0; i <= sizeOfMatrix + 1; i++)
    label_alinments[i] = XmALIGNMENT_CENTER;

  labels[0] = (char *) "UID";
  widths[0] = 12;

  unsigned long j = 1;

  if (const std::list<OksAttribute *> * attrs = c->all_attributes())
    for (const auto& x : *attrs)
      {
        labels[j] = const_cast<char *>(x->get_name().c_str());
        widths[j++] = x->get_name().length();
      }

  if (const std::list<OksRelationship *> * rels = c->all_relationships())
    for (const auto& x : *rels)
      {
        labels[j] = const_cast<char *>(x->get_name().c_str());
        widths[j++] = x->get_name().length();
      }

  labels[sizeOfMatrix + 1] = const_cast<char *>("");
  widths[sizeOfMatrix + 1] = 1;

  Widget mw = f.add_matrix(id, (void *)name);
  f.attach_right(id);

  XtVaSetValues(
    mw,
    XmNcolumns, sizeOfMatrix + 2,
    XmNcolumnLabels, labels,
    XmNcolumnLabelAlignments, label_alinments,
    XmNcolumnWidths, widths,
    XmNfixedColumns, 1,
    XmNfill, true,
    NULL
  );

  delete [] label_alinments;
  delete [] labels;
  delete [] widths;

  XtAddCallback(mw, XmNenterCellCallback, OksXm::Matrix::cellCB, 0);
  XtAddCallback(mw, XmNlabelActivateCallback, labelCB, (XtPointer)c);
  XtAddCallback(mw, XmNdefaultActionCallback, objectCB, (XtPointer)parent);

  return mw;
}

void
ObjectTable::labelCB(Widget w, XtPointer dt, XtPointer cb)
{
  XbaeMatrixLabelActivateCallbackStruct * cbs = (XbaeMatrixLabelActivateCallbackStruct *) cb;

  if (cbs->row_label == True || cbs->label == 0 || *cbs->label == 0)
    return;

  int pos = cbs->column;
  std::string name(cbs->label);

  std::string::size_type space_pos;
  while ((space_pos = name.find('\n')) != std::string::npos)
    name[space_pos] = ' ';

  if (OksAttribute *a = ((const OksClass *) dt)->find_attribute(name.c_str()))
    switch (a->get_data_type())
      {
        case OksData::s8_int_type:
        case OksData::s16_int_type:
        case OksData::s32_int_type:
        case OksData::s64_int_type:
          OksXm::Matrix::sort_rows(w, pos, OksXm::Matrix::SortByInteger);
          return;

        case OksData::u8_int_type:
        case OksData::u16_int_type:
        case OksData::u32_int_type:
        case OksData::u64_int_type:
          OksXm::Matrix::sort_rows(w, pos, OksXm::Matrix::SortByUnsigned);
          return;

        case OksData::float_type:
        case OksData::double_type:
          OksXm::Matrix::sort_rows(w, pos, OksXm::Matrix::SortByDouble);
          return;

        case OksData::date_type:
          OksXm::Matrix::sort_rows(w, pos, OksXm::Matrix::SortByDate);
          return;

        case OksData::time_type:
          OksXm::Matrix::sort_rows(w, pos, OksXm::Matrix::SortByTime);
          return;

        default:
          break;
      }

  OksXm::Matrix::sort_rows(w, pos, OksXm::Matrix::SortByString);
}


void
ObjectTable::objectCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  OksDataEditorMainDialog * parent_dlg = (OksDataEditorMainDialog *) client_data;
  XbaeMatrixDefaultActionCallbackStruct *cb = (XbaeMatrixDefaultActionCallbackStruct *) call_data;

  if (OksObject *o = (OksObject *) XbaeMatrixGetRowUserData(w, cb->row))
    {
      if (parent_dlg->is_dangling(o))
        ers::error(OksDataEditor::InternalProblem(ERS_HERE, "selected object does not exist (was deleted, file was closed, ...)"));
      else
        parent_dlg->create_object_dlg(o);
    }
  else
    {
      std::ostringstream text;
      text << "XbaeMatrixGetRowUserData(" << (void *)w << ", " << cb->row << ") got 0";
      ers::error(OksDataEditor::InternalProblem(ERS_HERE, text.str().c_str()));
    }
  }


void
OksDataEditorClassDialog::closeCB(Widget, XtPointer d, XtPointer)
{
  delete (OksDataEditorClassDialog *)d;
}


void
OksDataEditorClassDialog::findCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  if (((XmSelectionBoxCallbackStruct *) call_data)->reason == XmCR_OK)
    {
      OksXm::AutoCstring objName(XmTextGetString(XmSelectionBoxGetChild(w, XmDIALOG_VALUE_TEXT)));

      if (objName.is_non_empty())
        {
          Widget matrix = ((OksDataEditorClassDialog *) client_data)->get_widget(idObjects);
          int rowsNumber = XbaeMatrixNumRows(matrix);
          int i;

          for (i = 0; i < rowsNumber; i++)
            {
              const char *uid = XbaeMatrixGetCell(matrix, i, 0);

              if (uid && !strcmp(uid, objName.get()))
                {
                  XbaeMatrixDeselectAll(matrix);
                  XbaeMatrixSelectRow(matrix, i);
                  XbaeMatrixMakeCellVisible(matrix, i, 0);
                  break;
                }
            }

          if (i == rowsNumber)
            {
              std::ostringstream text;
              text << "cannot find object [" << objName.get() << ']';
              ers::error(OksDataEditor::InternalProblem(ERS_HERE, text.str().c_str()));
            }
        }
    }
}


void
OksDataEditorClassDialog::actionCB(Widget w, XtPointer d, XtPointer)
{
  OksDataEditorClassDialog *dlg = (OksDataEditorClassDialog *) d;
  OksClass *c = dlg->get_class();
  OksDataEditorMainDialog *main_dlg = dlg->parent();
  long user_data = (long) OksXm::get_user_data(w);
  Widget matrix = XtParent(XtParent(XtParent(w)));
  OksObject *o = nullptr;

  if (user_data == idShow || user_data == idSelect || user_data == idDelete)
    {
      int row = -1, column = -1;
      XbaeMatrixFirstSelectedCell(matrix, &row, &column);

      if (row == -1 || column == -1)
        return;

      o = (OksObject *) XbaeMatrixGetRowUserData(matrix, row);
    }

  switch (user_data)
    {
  case idNew:
    main_dlg->create_object_dlg(c);
    break;

  case idShow:
    main_dlg->create_object_dlg(o);
    break;

  case idSelect:
    clipboardObject = o;
    break;

  case idDelete:
    try
      {
        OksObject::destroy(o);
      }
    catch (oks::exception& ex)
      {
        OksDataEditorMainDialog::report_exception("Delete Object", ex, dlg->get_form_widget());
      }
    break;

  case idFind:
    {
      Arg args[5];
      XmString string = OksXm::create_string("Input object id:");
      Widget dialog;

      XtSetArg(args[0], XmNallowShellResize, true);
      XtSetArg(args[1], XmNdialogStyle, XmDIALOG_APPLICATION_MODAL);
      XtSetArg(args[2], XmNselectionLabelString, string);

      dialog = XmCreatePromptDialog(dlg->get_form_widget(), (char *) "prompt dialog", args, 3);
      XtVaSetValues(XtParent(dialog), XmNtitle, "Find OKS Object", NULL);

      XtAddCallback(dialog, XmNokCallback, findCB, (XtPointer) d);
      XtAddCallback(dialog, XmNcancelCallback, findCB, (XtPointer) d);

      XtManageChild(dialog);
      XmStringFree(string);

      break;
    }

  case idQuery:
    main_dlg->create_query_dlg(c);
    break;

  case idLoadQuery:
    {
      Arg args[5];
      Widget dialog;

      XtSetArg(args[0], XmNdialogStyle, XmDIALOG_APPLICATION_MODAL);
      dialog = XmCreateFileSelectionDialog(dlg->get_form_widget(), (char *) "file selection", args, 1);

      XtVaSetValues(XtParent(dialog), XmNtitle, "Open OKS Query", NULL);
      XtAddCallback(dialog, XmNokCallback, OksDataEditorMainDialog::OpenQueryCB, (XtPointer) main_dlg);
      XtAddCallback(dialog, XmNcancelCallback, OksDataEditorMainDialog::OpenQueryCB, (XtPointer) main_dlg);

      XtVaSetValues(dialog, XmNuserData, (XtPointer) c, NULL);
      XtManageChild(dialog);

      break;
    }
    }
}


void
OksDataEditorClassDialog::objectAC(Widget w, XtPointer client_data, XEvent *event, Boolean *)
{
  if (((XButtonEvent *) event)->button != Button3)
    return;

  OksPopupMenu popup(w);

  int rowsNumber = XbaeMatrixNumRows(w);
  int row = -1, column = -1;

  XbaeMatrixFirstSelectedCell(w, &row, &column);

  const char * value = ((row != -1 && column != -1) ? XbaeMatrixGetCell(w, row, column) : 0);

  if (value && *value == '\0')
    value = nullptr;

  Widget btn = popup.addItem("New", idNew, actionCB, client_data);

  if (!OksDataEditorMainDialog::is_valid_active_data())
    XtSetSensitive(btn, false);

  popup.add_separator();

  if (value)
    {
      popup.addItem("Show   ", value, idShow, actionCB, client_data);
      popup.addItem("Select ", value, idSelect, actionCB, client_data);
      popup.add_separator();
      popup.addItem("Delete ", value, idDelete, actionCB, client_data);
    }
  else
    {
      popup.addDisableItem("Show   ");
      popup.addDisableItem("Select ");
      popup.add_separator();
      popup.addDisableItem("Delete ");
    }

  popup.add_separator();

  if (rowsNumber > 1)
    popup.addItem("Find by UID", idFind, actionCB, client_data);
  else
    popup.addDisableItem("Find by UID");

  popup.add_separator();

  popup.addItem("Query", idQuery, actionCB, client_data);
  popup.addItem("Load Query", idLoadQuery, actionCB, client_data);

  popup.show((XButtonPressedEvent *) event);
}


void
OksDataEditorClassDialog::findObjectsCB(Widget, XtPointer client_data, XtPointer)
{
  OksDataEditorClassDialog * dlg(reinterpret_cast<OksDataEditorClassDialog *>(client_data));
  dlg->p_select_objects_panel->refresh();
  dlg->refresh();
}

OksDataEditorClassDialog::OksDataEditorClassDialog(OksClass *cl, OksDataEditorMainDialog *p) :
  OksDataEditorDialog	(p),
  p_class		(cl),
  p_select_objects_panel(nullptr)
{
  Widget w;

  setTitle("Oks Class", get_class()->get_name().c_str());
  setIcon((OksDialog *)p);

  w = add_text_field(idClass, "Class:");
  attach_right(idClass);
  set_value(idClass, get_class()->get_name().c_str());
  OksXm::TextField::set_not_editable(w);

  Widget mw = ObjectTable::create_matrix(*this, "Objects:", idObjects, get_class(), parent());

  XtAddEventHandler(mw, ButtonPressMask, False, objectAC, (XtPointer)this);

  OksXm::set_close_cb(XtParent(get_form_widget()), closeCB, (void *)this);

  p_select_objects_panel = new OksDataEditorSearchPanel(*this, "Matching UIDs:", "objects", findObjectsCB, (XtPointer)this);

  attach_bottom(idObjects);

  refresh();
  show();

  XtAddCallback(mw, XmNresizeCallback, OksXm::Matrix::resizeCB, 0);

  setMinSize();
  setCascadePos();

  p_select_objects_panel->init();
}


OksDataEditorClassDialog::~OksDataEditorClassDialog()
{
  parent()->remove_dlg(this);
}


void
OksDataEditorClassDialog::refresh()
{
  size_t sizeOfMatrix(get_class()->number_of_all_attributes() + get_class()->number_of_all_relationships());

  std::list<ObjectRow *> result;

  if (const OksObject::Map * objs = get_class()->objects())
    for (const auto& i : *objs)
      if (p_select_objects_panel->test_match(i.second->GetId()) == true)
        ObjectTable::add_row(i.second, 0, sizeOfMatrix, result);

  Widget objs_w(get_widget(idObjects));

  ObjectTable::show_rows(objs_w, sizeOfMatrix, parent()->schema_list, result);

  if (!OksXm::Matrix::resort_rows(objs_w))
    OksXm::Matrix::sort_rows(objs_w, 0, OksXm::Matrix::SortByString);
 }
