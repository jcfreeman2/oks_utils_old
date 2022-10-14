#ifndef __DATA_EDITOR_MERGE_CONFLICTS_DLG_H
#define  __DATA_EDITOR_MERGE_CONFLICTS_DLG_H

#include <list>

#include <oks/file.h>
#include <oks/xm_dialog.h>

#include "data_editor_dialog.h"


class OksDataEditorMergeConflictsDialog : public OksDataEditorDialog
{
  friend class OksDataEditorMainDialog;

  public:

    OksDataEditorMergeConflictsDialog(OksDataEditorMainDialog * parent, std::list<std::string> files);

    ~OksDataEditorMergeConflictsDialog();

    void refresh();

    OksDataEditorDialog::Type type() const {return OksDataEditorDialog::MergeConflictsDlg;}

  private:

    std::list<std::string> p_files;
    bool p_restart_check_updated;
    bool p_is_active;
    Widget files_w;

    static void tryAgainCB(Widget, XtPointer, XtPointer);

    enum {
      idLabel = 100,
      idFiles,
      idText,
      idApply
    };

};

#endif
