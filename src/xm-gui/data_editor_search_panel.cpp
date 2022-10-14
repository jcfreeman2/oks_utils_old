#include <sstream>

#include <Xm/ToggleB.h>
#include <Xm/TextF.h>

#include "oks/xm_utils.h"

#include "data_editor.h"
#include "data_editor_search_panel.h"

#include "error.xpm"
#include "select-class.xpm"

OksDataEditorSearchPanel::OksDataEditorSearchPanel(
  OksForm& form, const char *title, const char *tool_tips, XtCallbackProc cb, XtPointer cb_param, unsigned short num) :
  p_select_match_case    (false),
  p_select_is_reg_expr   (false),
  p_select_re            (0),
  p_form                 (form),
  p_cb                   (cb),
  p_cb_param             (cb_param),
  idSelectPanelName      (idSelectPanelBase + (num*4) + 0),
  idSelectPanelMatchCase (idSelectPanelBase + (num*4) + 1),
  idSelectPanelRegExp    (idSelectPanelBase + (num*4) + 2),
  idSelectPanelBtn       (idSelectPanelBase + (num*4) + 3)
{
  XtAddCallback(p_form.add_text_field(idSelectPanelName, title), XmNvalueChangedCallback, valueCB, (XtPointer)this);

  Widget re_btn;

  {
    Widget w = p_form.add_toggle_button(idSelectPanelMatchCase, "Match case");
    p_form.attach_previous(idSelectPanelMatchCase);
    XtAddCallback(w, XmNvalueChangedCallback, cb, cb_param);

    re_btn = p_form.add_toggle_button(idSelectPanelRegExp, "Regular expression");
    XtAddCallback(re_btn, XmNvalueChangedCallback, disable_widgetCB, (XtPointer)w);
    p_form.attach_previous(idSelectPanelRegExp);
  }

  {
    std::string s = std::string("Select ") + tool_tips + " matching regular expression";
  
    Widget w = p_form.add_push_button(idSelectPanelBtn, s.c_str());
    XtAddCallback(w, XmNactivateCallback, cb, cb_param);

    if(Pixmap select_btn_pxm = OksXm::create_color_pixmap(p_form.get_form_widget(), select_class_xpm)) {
      XtVaSetValues(w, XmNlabelPixmap, select_btn_pxm, XmNlabelType, XmPIXMAP, NULL);
      OksXm::show_label_as_tips(w);
    }

    XtAddCallback(re_btn, XmNvalueChangedCallback, manage_widgetCB, (XtPointer)w);

    p_form.attach_previous(idSelectPanelBtn);
  }
}

void
OksDataEditorSearchPanel::init()
{
  p_form.hide_w(idSelectPanelBtn);
}

void
OksDataEditorSearchPanel::valueCB(Widget, XtPointer client_data, XtPointer)
{
  OksDataEditorSearchPanel * dlg(reinterpret_cast<OksDataEditorSearchPanel *>(client_data));

  Boolean b;

  XtVaGetValues(dlg->p_form.get_widget(dlg->idSelectPanelRegExp), XmNset, &b, NULL);

  if(b == False) {
    (*dlg->p_cb)(0, dlg->p_cb_param, 0);
  }
}


void
OksDataEditorSearchPanel::disable_widgetCB(Widget, XtPointer client_data, XtPointer call_data)
{
  XtSetSensitive(reinterpret_cast<Widget>(client_data), (((XmToggleButtonCallbackStruct *)call_data)->set == False));
}

void
OksDataEditorSearchPanel::manage_widgetCB(Widget, XtPointer client_data, XtPointer call_data)
{
  Widget w(reinterpret_cast<Widget>(client_data));
  
  if(((XmToggleButtonCallbackStruct *)call_data)->set == True) {
    if(XtIsManaged(w) == false) XtManageChild(w);
  }
  else {
    if(XtIsManaged(w) == true) XtUnmanageChild(w);
  }
}

void
OksDataEditorSearchPanel::refresh()
{
  if(p_select_re) {
    delete p_select_re;
    p_select_re = 0;
  }

  if(char * value = XmTextFieldGetString(p_form.get_widget(idSelectPanelName))) {
    p_select_str = value;
    XtFree(value);
  }
  else {
    p_select_str = "";
  }

  if(!p_select_str.empty()) {
    Boolean b;

    XtVaGetValues(p_form.get_widget(idSelectPanelMatchCase), XmNset, &b, NULL);
    p_select_match_case = b;

    XtVaGetValues(p_form.get_widget(idSelectPanelRegExp), XmNset, &b, NULL);
    p_select_is_reg_expr = b;

    if(p_select_is_reg_expr) {
      try {
        p_select_re = new boost::regex(p_select_str);
      }
      catch(std::exception& ex) {
        std::ostringstream text;
        text << "ERROR: cannot create regular expression\nfrom string \'" << p_select_str << "\':\n" << ex.what() << std::ends;
        XtManageChild(
          OksXm::create_dlg_with_pixmap(
            p_form.get_form_widget(), "Select Error", error_xpm, text.str().c_str(), "Ok", 0, 0, 0, 0
          )
        );
	p_select_str = "";
	p_select_re = 0;
      }
    }
  }
}

bool
OksDataEditorSearchPanel::test_match(const std::string& s)
{
  if(p_select_re) {
    if(boost::regex_match(s, *p_select_re) == false) {
      return false;
    }
  }
  else if(!p_select_str.empty()) {
    if(p_select_match_case) {
      if(strstr(s.c_str(), p_select_str.c_str()) == 0) {
        return false;
      }
    }
    else {
      if(strcasestr(s.c_str(), p_select_str.c_str()) == 0) {
        return false;
      }
    }
  }

  return true;
}
