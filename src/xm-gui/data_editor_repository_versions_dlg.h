#ifndef __DATA_EDITOR_REPOSITORY_VERSIONS_DLG_H
#define __DATA_EDITOR_REPOSITORY_VERSIONS_DLG_H

#include <list>

#include "data_editor_dialog.h"

class OksDataEditorRepositoryVersionsDialog : public OksDataEditorDialog
{
  public:

    OksDataEditorRepositoryVersionsDialog(std::vector<OksRepositoryVersion> versions, OksDataEditorMainDialog * parent, bool archival);

    ~OksDataEditorRepositoryVersionsDialog();

    OksDataEditorDialog::Type type() const {return OksDataEditorDialog::RepositoryVersionsDlg;}

    void add(const std::vector<OksRepositoryVersion>& versions);
    void replace(std::vector<OksRepositoryVersion> versions);


  private:

    Widget m_table;
    std::vector<OksRepositoryVersion> m_versions;
    bool m_archival;

    void refresh();

    void reload(OksKernel::RepositoryUpdateType type, const std::string& hash_val);

    static void labelCB(Widget, XtPointer, XtPointer);
    static void versionCB(Widget, XtPointer, XtPointer);
    static void saveCB(Widget, XtPointer, XtPointer);
    static void reloadCB(Widget, XtPointer, XtPointer);
    static void mergeAndReloadCB(Widget, XtPointer, XtPointer);
    static void discardAndReloadCB(Widget, XtPointer, XtPointer);
    static void closeCB(Widget, XtPointer, XtPointer);

    enum {
      idTable = 111,
      idSave,
      idDiscard,
      idMerge,
      idReload,
      idCancel
    };
};

#endif
