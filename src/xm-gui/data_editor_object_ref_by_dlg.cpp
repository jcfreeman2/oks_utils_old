#include <ers/ers.h>

#include <Xm/Matrix.h>

#include <sstream>

#include <oks/object.h>

#include "data_editor_main_dlg.h"
#include "data_editor_exceptions.h"
#include "data_editor_object_ref_by_dlg.h"


OksDataEditorObjectReferencedByDialog::OksDataEditorObjectReferencedByDialog(
 const OksObject * o, OksDataEditorMainDialog *p
) : OksDataEditorDialog (p), m_object (o)
{

  std::ostringstream text;
  text << "Object " << o << " is referenced by";

    // set title

  setTitle( text.str().c_str() );
  setIcon((OksDialog *)parent());


    // create table name

  text << ':';
  m_table = add_matrix(idTable, (void *)text.str().c_str());
  attach_right(idTable);


    // create table title

  {
    String labels[2];

    labels[0] = (char *)"Class";
    labels[1] = (char *)"Object ID";

    unsigned char label_alinments[3];
    short widths[2];

    for(size_t i=0; i<2; i++) {
      widths[i] = 4;
      label_alinments[i] = XmALIGNMENT_CENTER;
    }

    XtVaSetValues(
      m_table,
      XmNcolumns, 2,
      XmNcolumnLabels, labels,
      XmNcolumnLabelAlignments, label_alinments,
      XmNcolumnWidths, widths,
      XmNfill, true,
      NULL
    );
  }

  refresh();

  XtAddCallback(m_table, XmNenterCellCallback, OksXm::Matrix::cellCB, 0);
  XtAddCallback(m_table, XmNdefaultActionCallback, objectCB, reinterpret_cast<XtPointer>(this));
  XtAddCallback(m_table, XmNlabelActivateCallback, labelCB, 0);

  attach_bottom(idTable);

  OksXm::Matrix::adjust_column_widths(m_table);
  OksXm::Matrix::set_visible_height(m_table);
  OksXm::Matrix::set_cell_background(m_table, parent()->schema_list);

  OksXm::Matrix::set_widths(m_table, 2);

  show();

  XtAddCallback(m_table, XmNresizeCallback, OksXm::Matrix::resizeCB, 0);
  OksXm::set_close_cb(XtParent(get_form_widget()), closeCB, (void *)this);

  setCascadePos();
}

void
OksDataEditorObjectReferencedByDialog::refresh() const
{
  XbaeMatrixDeleteRows(m_table, 0, XbaeMatrixNumRows(m_table));

  if(OksObject::FList * objs = m_object->get_all_rels()) {
    const int num_of_rows(objs->size());

    String ** cells = new String * [ num_of_rows ];

    int idx(0);

    OksObject::FList::const_iterator j = objs->begin();

    for(; j != objs->end(); ++j, ++idx) {
      cells[idx] = new String [2];
      cells[idx][0] = const_cast<String>((*j)->GetClass()->get_name().c_str());
      cells[idx][1] = const_cast<String>((*j)->GetId().c_str());
    }

    XtVaSetValues(m_table, XmNrows, num_of_rows, NULL);
    XtVaSetValues(m_table, XmNcells, cells, NULL);

    idx = 0;

    for(j = objs->begin(); j != objs->end(); ++j, ++idx) {
      XbaeMatrixSetRowUserData(m_table, idx, (XtPointer)(*j));
      delete [] cells[idx];
    }

    delete [] cells;
    delete objs;
  }
}

OksDataEditorObjectReferencedByDialog::~OksDataEditorObjectReferencedByDialog()
{
  parent()->remove_dlg(this);
}


void
OksDataEditorObjectReferencedByDialog::labelCB(Widget w, XtPointer /*dt*/, XtPointer cb)
{
  XbaeMatrixLabelActivateCallbackStruct * cbs(reinterpret_cast<XbaeMatrixLabelActivateCallbackStruct *>(cb));

  if(cbs->row_label == True || cbs->label == 0 || *cbs->label == 0) return;

  OksXm::Matrix::sort_rows(w, cbs->column, OksXm::Matrix::SortByString);
}

void
OksDataEditorObjectReferencedByDialog::objectCB(Widget w, XtPointer dt, XtPointer cb)
{
  XbaeMatrixLabelActivateCallbackStruct * cbs(reinterpret_cast<XbaeMatrixLabelActivateCallbackStruct *>(cb));

  if(OksObject * o = (OksObject *)(XbaeMatrixGetRowUserData(w, cbs->row))) {
    OksDataEditorObjectReferencedByDialog * dlg(reinterpret_cast<OksDataEditorObjectReferencedByDialog *>(dt));
    if(dlg->parent()->is_dangling(o)) {
      ers::error(OksDataEditor::InternalProblem(ERS_HERE, "the object was removed"));
    }
    else {
      dlg->parent()->create_object_dlg(o);
    }
  }
}

void
OksDataEditorObjectReferencedByDialog::closeCB(Widget, XtPointer d, XtPointer)
{
  delete (OksDataEditorObjectReferencedByDialog *)d;
}
