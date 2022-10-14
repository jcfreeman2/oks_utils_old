#include <oks/file.h>

#include "data_editor_main_dlg.h"
#include "data_editor_data_dlg.h"
#include "data_editor_comment_dlg.h"

#include <oks/xm_utils.h>
#include <oks/xm_context.h>

#include <stdlib.h>
#include <stdio.h>

#include <Xm/Text.h>
#include <Xm/TextF.h>

#include "comment.xpm"


OksDataEditorCommentDialog::OksDataEditorCommentDialog(
  OksDataEditorMainDialog * parent, OksFile * file, const std::string& creation_time, oks::Comment * comment
) :
  OksDataEditorDialog     (parent),
  p_file                  (file),
  p_comment               (comment),
  p_creation_time         (creation_time),
  p_restart_check_updated (false)
{
  static Pixmap	pixmap(OksXm::create_color_pixmap(get_form_widget(), comment_xpm));

  setIcon(pixmap);
  
  std::string simple_time_str;

  bool is_file_read_only = OksKernel::check_read_only(p_file);

  if(comment) {
    simple_time_str = OksDataEditorMainDialog::iso2simple(creation_time);
    std::string s("\'");
    s += simple_time_str + '\'';
    setTitle("Comment", s.c_str(), p_file->get_full_file_name().c_str());
  }
  else {
    setTitle("Add Comment", p_file->get_full_file_name().c_str());
  }

  Widget pxm_w = add(OksDlgEntity::Label, OksDlgEntity::idPixmap, (char *)"");
  XtVaSetValues(pxm_w, XmNlabelType, XmPIXMAP, XmNlabelPixmap, pixmap, NULL);

  Widget label_w = 0;

  if(comment) {
    label_w = add_label(idCreation, "creation");

    std::string s("Created by \"");
    s += comment->get_created_by();
    s += "\" on \"";
    s += comment->get_created_on();
    s += "\" at \"";
    s += simple_time_str;
    s += "\" (UTC)";

    set_value(idCreation, s.c_str());

    attach_previous(label_w, pxm_w, pxm_w, 0, XmATTACH_OPPOSITE_WIDGET);
  }
  else {
    p_is_modal = true;
  }

  add_text_field(idAuthor, "Author names");
  attach_right(idAuthor);

  attach_previous(
    get_previous(idAuthor), pxm_w,
    (comment ? label_w : pxm_w),
    (comment ? 2 : 0),
    (comment ? XmATTACH_WIDGET : XmATTACH_OPPOSITE_WIDGET)
  );
  
  if(is_file_read_only) {
    OksXm::TextField::set_not_editable(get_widget(idAuthor));
  }

  if(comment) {
    set_value(idAuthor, comment->get_author().c_str());
  }
  else {
    set_value(idAuthor, OksGC::Parameters::def_params().get_string_parameter(OksParameterNames::get_author_name())->c_str());
  }

  add_text(idText, "Text");
  attach_right(idText);
  if(comment) {
    set_value(idText, comment->get_text().c_str());
  }

  if(is_file_read_only) {
    OksXm::TextField::set_not_editable(get_widget(idText));
  }

  add_separator();

  if(comment) {
    XtAddCallback(add_push_button(idApply, "OK"), XmNactivateCallback, applyCB, (XtPointer)this);
    XtAddCallback(add_push_button(idClose, "Cancel"), XmNactivateCallback, cancelCB, (XtPointer)this);
    attach_previous(idClose);
    OksXm::set_close_cb(XtParent(get_form_widget()), cancelCB, (void *)this);
  }
  else {
    XtAddCallback(add_push_button(idApply, "Add"), XmNactivateCallback, createCB, (XtPointer)this);
    XtAddCallback(add_push_button(idClose, "Skip"), XmNactivateCallback, exitCB, (XtPointer)this);
    attach_previous(idClose);
    OksXm::set_close_cb(XtParent(get_form_widget()), exitCB, (void *)this);
  }

  attach_bottom(idText);

  show(comment ? XtGrabNone: XtGrabExclusive);

  setMinSize();
  setCascadePos();

  if(!comment) {
    if(parent->p_check_updated_timer) {
      parent->stop_check_updated();
      p_restart_check_updated = true;
    }

    while(p_is_modal) {
      XtAppProcessEvent(parent->app_context(), XtIMAll);
    }

    delete this;
  }
}

OksDataEditorCommentDialog::~OksDataEditorCommentDialog()
{
  parent()->remove_dlg(this);

  if(p_restart_check_updated) {
    parent()->start_check_updated();
  }
}

void
OksDataEditorCommentDialog::exitCB(Widget w, XtPointer client_data, XtPointer)
{
  OksDataEditorCommentDialog * dlg = reinterpret_cast<OksDataEditorCommentDialog *>(client_data);
  dlg->p_is_modal = false;
  XtRemoveGrab(XtParent(w));
}


void
OksDataEditorCommentDialog::set_author(const char * author)
{
  if(*OksGC::Parameters::def_params().get_string_parameter(OksParameterNames::get_author_name()) != author) {
    OksGC::Parameters::def_params().set_generic_parameter(OksParameterNames::get_author_name(), author);
    try {
      OksGC::Parameters::def_params().save();
    }
    catch(oks::FailedSaveGuiRcFile& ex) {
      OksDataEditorMainDialog::report_error("Save OKS Data Editor Resource File", ex.what(), get_form_widget());
    }
  }
}

void
OksDataEditorCommentDialog::createCB(Widget w, XtPointer client_data, XtPointer)
{
  OksDataEditorCommentDialog * dlg = reinterpret_cast<OksDataEditorCommentDialog *>(client_data);

  OksXm::AutoCstring text(XmTextGetString(dlg->get_widget(idText)));
  OksXm::AutoCstring author(XmTextGetString(dlg->get_widget(idAuthor)));

  dlg->set_author(author.get());

  try {
    dlg->p_file->add_comment(text.get(), author.get());
    dlg->parent()->update_data_file_row(dlg->p_file);
    if(OksDataEditorDataDialog * odedd = dlg->parent()->find_data_dlg(dlg->p_file)) {
      odedd->refresh();
    }
  }
  catch(oks::FailedAddComment & ex) {
    OksDataEditorMainDialog::report_error("Create Comment", ex.what(), dlg->get_form_widget());
  }

  dlg->p_is_modal = false;
  XtRemoveGrab(XtParent(w));
}

void
OksDataEditorCommentDialog::applyCB(Widget, XtPointer client_data, XtPointer)
{
  OksDataEditorCommentDialog * dlg = reinterpret_cast<OksDataEditorCommentDialog *>(client_data);
  
  OksXm::AutoCstring text(XmTextGetString(dlg->get_widget(idText)));
  OksXm::AutoCstring author(XmTextGetString(dlg->get_widget(idAuthor)));

  bool need_to_update = false;

  if(dlg->p_comment->get_author() != author.get()) {
    need_to_update = true;
    dlg->set_author(author.get());
  }

  if(dlg->p_comment->get_text() != text.get()) {
    need_to_update = true;
  }
  
  if(need_to_update) {
    try {
      dlg->p_file->modify_comment(dlg->p_creation_time, text.get(), author.get());
      dlg->parent()->update_data_file_row(dlg->p_file);
      if(OksDataEditorDataDialog * odedd = dlg->parent()->find_data_dlg(dlg->p_file)) {
        odedd->refresh();
      }

    }
    catch(oks::FailedChangeComment& ex) {
      OksDataEditorMainDialog::report_error("Update Comment", ex.what(), dlg->get_form_widget());
    }
  }

  delete dlg;
}

void
OksDataEditorCommentDialog::cancelCB(Widget, XtPointer client_data, XtPointer)
{
  delete (OksDataEditorCommentDialog *)client_data;
}
