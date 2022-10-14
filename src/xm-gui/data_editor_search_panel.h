#ifndef __DATA_EDITOR_SEARCH_PANEL_H
#define __DATA_EDITOR_SEARCH_PANEL_H

#include <string>
#include <boost/regex.hpp>
#include <oks/xm_dialog.h>

class OksDataEditorSearchPanel {

  public:

    OksDataEditorSearchPanel(OksForm& form, const char *title, const char *tool_tips, XtCallbackProc cb, XtPointer cb_param, unsigned short num = 0);

    void refresh();
    void init();

    bool test_match(const std::string& s);


  private:

    std::string p_select_str;
    bool p_select_match_case;
    bool p_select_is_reg_expr;
    boost::regex * p_select_re;

    OksForm& p_form;

    XtCallbackProc p_cb;
    XtPointer p_cb_param;

    short idSelectPanelName;
    short idSelectPanelMatchCase;
    short idSelectPanelRegExp;
    short idSelectPanelBtn;

    static void valueCB(Widget, XtPointer, XtPointer call_data);
    static void disable_widgetCB(Widget, XtPointer, XtPointer call_data);
    static void manage_widgetCB(Widget, XtPointer, XtPointer call_data);

};



#endif
