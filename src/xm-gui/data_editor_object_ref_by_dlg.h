#ifndef __DATA_EDITOR_REFERENCED_BY_DLG_H
#define __DATA_EDITOR_REFERENCED_BY_DLG_H

#include <list>

#include "data_editor_dialog.h"
#include "data_editor_replace_dlg.h"

class OksDataEditorObjectReferencedByDialog : public OksDataEditorDialog
{

  public:

    OksDataEditorObjectReferencedByDialog(const OksObject*, OksDataEditorMainDialog*);

    ~OksDataEditorObjectReferencedByDialog();

    OksDataEditorDialog::Type type() const {return OksDataEditorDialog::ObjectReferencedByDlg;}

    const OksObject * get_object() const {return m_object;}

    void refresh() const;

  private:

    Widget m_table;
    const OksObject * m_object;

    static void labelCB(Widget, XtPointer, XtPointer);
    static void objectCB(Widget, XtPointer, XtPointer);
    static void closeCB(Widget, XtPointer, XtPointer);

    enum {
      idTable = 111
    };

};

#endif
