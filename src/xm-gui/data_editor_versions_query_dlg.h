#ifndef __DATA_EDITOR_VERSIONS_QUERY_DLG_H
#define __DATA_EDITOR_VERSIONS_QUERY_DLG_H

#include <string>

#include <oks/xm_dialog.h>

class OksDataEditorMainDialog;


class OksDataEditorVersionsQueryDialog : public OksDialog
{
  friend class OksDataEditorMainDialog;
  friend class OksDataEditorSearchResultDialog;

public:

  OksDataEditorVersionsQueryDialog(OksDataEditorMainDialog *);
  ~OksDataEditorVersionsQueryDialog();


private:

  OksDataEditorMainDialog * parent;

  Widget m_since_text_field;
  Widget m_until_text_field;

  static void
  applyCB(Widget, XtPointer, XtPointer);

  static void
  closeCB(Widget, XtPointer, XtPointer);

  enum
  {
    idOptionTitle = 111, idSince, idUntil, idSkipIrrelevant, idApplyBtn, idCloseBtn
  };

};


#endif
