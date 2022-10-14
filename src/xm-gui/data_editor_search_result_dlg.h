#ifndef __DATA_EDITOR_SEARCH_RESULT_DLG_H
#define __DATA_EDITOR_SEARCH_RESULT_DLG_H

#include <list>

#include "data_editor_dialog.h"
#include "data_editor_replace_dlg.h"

class OksDataEditorSearchResultDialog : public OksDataEditorDialog
{

  public:

    OksDataEditorSearchResultDialog(
      const OksDataEditorReplaceDialog&,
      const OksData& d_from,
      const OksData& d_to,
      const std::list<OksDataEditorReplaceResult *>& result
   );
   
   ~OksDataEditorSearchResultDialog();

   OksDataEditorDialog::Type type() const {return OksDataEditorDialog::FindReplaceResultDlg;}

  private:

    Widget m_table;
    const OksDataEditorReplaceDialog::ActionType m_search_type;
    const unsigned int m_num_of_columns;

    static void labelCB(Widget, XtPointer, XtPointer);
    static void objectCB(Widget, XtPointer, XtPointer);
    static void closeCB(Widget, XtPointer, XtPointer);

    enum {
      idTable = 111,
      idPostInfo
    };
};

#endif
