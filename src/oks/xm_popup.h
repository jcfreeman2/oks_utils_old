#ifndef __OKS_XM_POPUP
#define __OKS_XM_POPUP

#include <Xm/Xm.h>

class OksObject;
class OksKernel;

class OksPopupMenu
{
  public:

    OksPopupMenu(Widget);

    void	show(XButtonPressedEvent *) const;
    void	setDestroyAC(XtEventHandler, XtPointer) const;
    static void destroy(Widget);

    Widget	get_popup() const {return popup;}

    Widget	add_separator(Widget = 0) const;

    Widget	add_label(const char *, Widget = 0) const;

    Widget	addItem(const char *, const long, XtCallbackProc, XtPointer, Widget = 0) const;
    Widget	addItem(const char *, const char *, const long, XtCallbackProc, XtPointer, Widget = 0) const;
    Widget	addItem(const char *, OksObject *, const OksKernel * k, const long, XtCallbackProc, XtPointer, Widget = 0) const;

    Widget	addToggleItem(const char *, const long, bool, XtCallbackProc, XtPointer p, Widget = 0) const;

    Widget	addCascade(const char *, Widget = 0);

    Widget	addDisableItem(const char *, Widget = 0) const;


  private:

    Widget	popup;
};

#endif
