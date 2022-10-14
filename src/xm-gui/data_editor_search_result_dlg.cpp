#include <ers/ers.h>

#include <Xm/Matrix.h>

#include <sstream>

#include <oks/attribute.h>
#include <oks/relationship.h>
#include <oks/class.h>

#include "data_editor_main_dlg.h"
#include "data_editor_exceptions.h"
#include "data_editor_search_result_dlg.h"

OksDataEditorSearchResultDialog::OksDataEditorSearchResultDialog(
 const OksDataEditorReplaceDialog& p,
 const OksData& d_from,
 const OksData& d_to,
 const std::list<OksDataEditorReplaceResult *>& result
) :
  OksDataEditorDialog (p.parent),
  m_search_type       (p.m_find_only ? OksDataEditorReplaceDialog::find : OksDataEditorReplaceDialog::replace),
  m_num_of_columns    (p.m_find_only ? 4 : 6)
{

     // calculate title and update menu bar of the main window
  {
    static unsigned int fnum(1);
    static unsigned int rnum(1);

    std::ostringstream text;

    if(m_search_type == OksDataEditorReplaceDialog::find) { text << "Find Result [" << fnum++ << ']'; }
    else { text << "Replace Result [" << rnum++ << ']'; }

    setTitle( text.str().c_str() );
    setIcon((OksDialog *)parent());

    parent()->dialogs.push_back(this);
    parent()->update_menu_bar();
  }


    // create table name

  {
    std::ostringstream text;

    if(p.m_find_only == true) { text << "Find \'" << d_from << '\''; }
    else { text << "Replace \'" << d_from << "\' by \'" << d_to << '\''; }

    if(p.m_attr_type_on == true && p.item_type == OksDataEditorReplaceDialog::complex_string) {
      text << " (case " << (p.case_compare == OksDataEditorReplaceDialog::use_case ? "" : "in") << "sensitive match)";
    }
  
    if(p.m_selected_class) {
      text << " in ";

      if(p.m_selected_attribute) {
        text << "attribute \'" << p.m_selected_attribute->get_name() << "\' of ";
      }
      else if(p.m_selected_relationship) {
        text << "relationship \'" << p.m_selected_relationship->get_name() << "\' of ";
      }
    
      text << "class \'" << p.m_selected_class->get_name() << '\'';
      if(p.m_search_in_subclasses) text << " and subclasses";
    }
 
    text << ':';

    m_table = add_matrix(idTable, (void *)text.str().c_str());
    attach_right(idTable);
  }


    // create table title

  {
    String labels[6];

    labels[0] = (char *)"Class";
    labels[1] = (char *)"Object ID";
    labels[2] = (p.m_attr_type_on) ? (char *)"Attribute Name" : (char *)"Relationship Name";

    if(m_num_of_columns == 4) {
      labels[3] = (char *)"Value";
    }
    else {
      labels[3] = (char *)"Old Value";
      labels[4] = (char *)"New Value";
      labels[5] = (char *)"File Status";
    }

    unsigned char label_alinments[6];
    short widths[6];

    for(size_t i=0; i<m_num_of_columns; i++) {
      widths[i] = 4;
      label_alinments[i] = XmALIGNMENT_CENTER;
    }

    XtVaSetValues(
      m_table,
      XmNcolumns, m_num_of_columns,
      XmNcolumnLabels, labels,
      XmNcolumnLabelAlignments, label_alinments,
      XmNcolumnWidths, widths,
      XmNfill, true,
      NULL
    );
  }

    // fill table data

  unsigned int num_locked(0), num_read_only(0);

  XbaeMatrixDeleteRows(m_table, 0, XbaeMatrixNumRows(m_table));


  const int num_of_rows(result.size());

  String ** cells = new String * [ num_of_rows ];

  int idx(0);

  std::list<OksDataEditorReplaceResult *>::const_iterator j = result.begin();

  for(; j != result.end(); ++j, ++idx) {
    cells[idx] = new String [m_num_of_columns];
    cells[idx][0] = const_cast<String>((*j)->m_object->GetClass()->get_name().c_str());
    cells[idx][1] = const_cast<String>((*j)->m_object->GetId().c_str());
    cells[idx][2] = const_cast<String>((*j)->m_attr ? (*j)->m_attr->get_name().c_str() : (*j)->m_rel->get_name().c_str());
    cells[idx][3] = const_cast<String>((*j)->m_value.c_str());

    if(m_num_of_columns > 4) {
      cells[idx][4] = const_cast<String>((*j)->m_new_value.c_str());
      cells[idx][5] = const_cast<String>((*j)->m_text.c_str());
    }
  }

  XtVaSetValues(m_table, XmNrows, num_of_rows, NULL);
  XtVaSetValues(m_table, XmNcells, cells, NULL);

  idx = 0;

  for(j = result.begin(); j != result.end(); ++j, ++idx) {
    if(m_search_type == OksDataEditorReplaceDialog::replace) {
      Pixel * pixel(0);

      if((*j)->m_status == ReplaceDialog::read_only_file || (*j)->m_status == ReplaceDialog::bad_file) { pixel = &parent()->p_red_pixel; num_read_only++; }
      else if((*j)->m_status == ReplaceDialog::locked_file) { pixel = &parent()->p_blue_pixel; num_locked++; }
      else { pixel = &parent()->p_green_pixel; }

      XbaeMatrixSetRowColors(m_table, idx, pixel, 1);
    }

    XbaeMatrixSetRowUserData(m_table, idx, (XtPointer)((*j)->m_object));

    delete [] cells[idx];
  }

  delete [] cells;

  XtAddCallback(m_table, XmNenterCellCallback, OksXm::Matrix::cellCB, 0);
  XtAddCallback(m_table, XmNdefaultActionCallback, objectCB, reinterpret_cast<XtPointer>(this));
  XtAddCallback(m_table, XmNlabelActivateCallback, labelCB, 0);


  {
    Widget w = add_text_field(idPostInfo, "Summary:");
    attach_right(idPostInfo);

    std::ostringstream text;

    if(m_search_type == OksDataEditorReplaceDialog::replace) {
      text << "Replaced " << (result.size() - num_locked - num_read_only) << " values.";
      if(num_locked || num_read_only) {
        text << " Skip ";
	if(num_locked) {
	  text << num_locked << " values stored in locked";
	  if(num_read_only) text << " and ";
	}
	if(num_read_only) {
	  text << num_read_only << " values stored in read-only";
	}
	text << " files.";
      }
    }
    else {
      text << "Found " << result.size() << " values.";
    }

    set_value(idPostInfo, text.str().c_str());
    OksXm::TextField::set_not_editable(w);
  }


  attach_bottom(idTable);

  OksXm::Matrix::adjust_column_widths(m_table);
  OksXm::Matrix::set_visible_height(m_table);
  OksXm::Matrix::set_cell_background(m_table, parent()->schema_list);

  if(m_search_type == OksDataEditorReplaceDialog::replace) {
    OksXm::Matrix::set_widths(m_table, 5);
  }
  else {
    OksXm::Matrix::set_widths(m_table, 4);
  }

  show();

  XtAddCallback(m_table, XmNresizeCallback, OksXm::Matrix::resizeCB, 0);
  OksXm::set_close_cb(XtParent(get_form_widget()), closeCB, (void *)this);

//  setMinSize();
  setCascadePos();
}

OksDataEditorSearchResultDialog::~OksDataEditorSearchResultDialog()
{
  parent()->remove_dlg(this);
}


void
OksDataEditorSearchResultDialog::labelCB(Widget w, XtPointer /*dt*/, XtPointer cb)
{
  XbaeMatrixLabelActivateCallbackStruct * cbs(reinterpret_cast<XbaeMatrixLabelActivateCallbackStruct *>(cb));

  if(cbs->row_label == True || cbs->label == 0 || *cbs->label == 0) return;

  OksXm::Matrix::sort_rows(w, cbs->column, OksXm::Matrix::SortByString);
}

void
OksDataEditorSearchResultDialog::objectCB(Widget w, XtPointer dt, XtPointer cb)
{
  XbaeMatrixLabelActivateCallbackStruct * cbs(reinterpret_cast<XbaeMatrixLabelActivateCallbackStruct *>(cb));

  if(OksObject * o = (OksObject *)(XbaeMatrixGetRowUserData(w, cbs->row))) {
    OksDataEditorSearchResultDialog * dlg(reinterpret_cast<OksDataEditorSearchResultDialog *>(dt));
    if(dlg->parent()->is_dangling(o)) {
      ers::error(OksDataEditor::InternalProblem(ERS_HERE, "the object was removed"));
    }
    else {
      dlg->parent()->create_object_dlg(o);
    }
  }
}

void
OksDataEditorSearchResultDialog::closeCB(Widget, XtPointer d, XtPointer)
{
  delete (OksDataEditorSearchResultDialog *)d;
}
