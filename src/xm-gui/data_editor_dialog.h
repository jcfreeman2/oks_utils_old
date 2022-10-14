#ifndef __DATA_EDITOR_DLG_H
#define __DATA_EDITOR_DLG_H 

#include <oks/xm_dialog.h>

class OksDataEditorMainDialog;

class OksDataEditorDialog : public OksDialog
{
  public:

    enum Type {
      DataDlg,
      ClassDlg,
      ObjectDlg,
      ObjectReferencedByDlg,
      RepositoryVersionsDlg,
      MergeConflictsDlg,
      QueryDlg,
      CommentDlg,
      FilesDlg,
      FindReplaceResultDlg
    };

    OksDataEditorDialog		 (OksDataEditorMainDialog *);
    virtual ~OksDataEditorDialog () {;}

    OksDataEditorMainDialog *	 parent() const {return p_parent;}
    virtual Type		 type() const = 0;


  private:

    OksDataEditorMainDialog *	 p_parent;
};

#endif
