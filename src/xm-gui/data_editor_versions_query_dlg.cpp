#include <ers/ers.h>

#include <Xm/TextF.h>
#include <Xm/ToggleB.h>

#include <oks/kernel.h>
#include <oks/xm_utils.h>

#include "data_editor_versions_query_dlg.h"
#include "data_editor_repository_versions_dlg.h"
#include "data_editor_main_dlg.h"

#include "error.xpm"

std::list<const char *> query_types
  { "datetime", "commit hash or tag", "To select versions specify" };

OksDataEditorVersionsQueryDialog::OksDataEditorVersionsQueryDialog(OksDataEditorMainDialog *p) :
  OksDialog	           ("", p),
  parent	           (p)
{
    m_since_text_field = 0;
    m_until_text_field = 0;

  setTitle("Select archive versions");
  setIcon((OksDialog *)p);

  add_option_menu(idOptionTitle, &query_types);

  m_since_text_field = add_text_field(idSince, "Since: ");
  m_until_text_field = add_text_field(idUntil, "Until: ");

  XmToggleButtonSetState(add_toggle_button(idSkipIrrelevant, "Skip irrelevant"), !p->data_files().empty(), false);

  add_separator();

  XtAddCallback(
    add_push_button(idApplyBtn, "OK"),
    XmNactivateCallback,
    OksDataEditorVersionsQueryDialog::applyCB,
    (XtPointer)this
  );

  XtAddCallback(
    add_push_button(idCloseBtn, "Cancel"),
    XmNactivateCallback,
    OksDataEditorVersionsQueryDialog::closeCB,
    (XtPointer)this
  );

  attach_previous(idCloseBtn);

  OksXm::set_close_cb(
    XtParent(get_form_widget()),
    OksDataEditorVersionsQueryDialog::closeCB,
    (void *)this
  );

  show();

  setMinSize();
  setCascadePos();

//  XtUnmanageChild(m_query_text_field);
}


	//
	// Destructor of Find/Replace window
	//

OksDataEditorVersionsQueryDialog::~OksDataEditorVersionsQueryDialog()
{
  parent->hide_versions_query_dialog();
}

void
OksDataEditorVersionsQueryDialog::applyCB(Widget, XtPointer client_data, XtPointer)
{
  OksDataEditorVersionsQueryDialog * dlg = reinterpret_cast<OksDataEditorVersionsQueryDialog *>(client_data);

  char * since = XmTextFieldGetString(dlg->m_since_text_field);
  char * until = XmTextFieldGetString(dlg->m_until_text_field);

  char *value = dlg->get_value(idOptionTitle);

  try
    {
      bool skip_irrelevant = XmToggleButtonGetState(dlg->get_widget(idSkipIrrelevant));

      std::vector<OksRepositoryVersion> versions = (!strcmp(value, query_types.front()))
        ? dlg->parent->get_repository_versions_by_date(skip_irrelevant, since, until)
        : dlg->parent->get_repository_versions_by_hash(skip_irrelevant, since, until);

      if (!versions.empty())
        dlg->parent->show_archived_repository_versions_dlg(versions);

      delete dlg;
    }
  catch (oks::exception& ex)
    {
      XtManageChild(
        OksXm::create_dlg_with_pixmap(
            dlg->get_form_widget(), "Cannot get versions", error_xpm, ex.what(), "Ok", 0, 0, 0, 0
        )
      );
    }
}

void
OksDataEditorVersionsQueryDialog::closeCB(Widget, XtPointer client_data, XtPointer)
{
  delete reinterpret_cast<OksDataEditorVersionsQueryDialog *>(client_data);
}
