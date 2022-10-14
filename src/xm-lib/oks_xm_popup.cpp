#define _OksXmBuildDll_

#include <stdlib.h>

#include <string>

#include <oks/object.h>
#include <oks/attribute.h>
#include <oks/xm_popup.h>
#include <oks/xm_utils.h>

#include <Xm/CascadeB.h>
#include <Xm/LabelG.h>
#include <Xm/MenuShell.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/SeparatoG.h>
#include <Xm/ToggleB.h>


#define POPUP_MENU_MAX_LENGTH 36


static void
unmapAC(Widget w, XtPointer, XEvent *event, Boolean *)
{
  if(event->type != UnmapNotify) return;
  XtDestroyWidget(w);
}

void
OksPopupMenu::destroy(Widget w)
{
  Widget popup_parent = XtParent(XtParent(w));
  XtRemoveEventHandler(popup_parent, StructureNotifyMask, False, 0, 0);
  XtDestroyWidget(popup_parent);
}

OksPopupMenu::OksPopupMenu(Widget parent)
{
#if defined(__linux__) && (XmVersion < 2000)
  popup = XmCreatePopupMenu(parent, (char *)"popup", 0, 0);
#else
  popup = XmCreateSimplePopupMenu(parent, (char *)"popup", 0, 0);
#endif

  setDestroyAC(unmapAC, 0);
}


void
OksPopupMenu::setDestroyAC(XtEventHandler eh_proc, XtPointer closure) const
{
  XtAddEventHandler(XtParent(popup), StructureNotifyMask, False, eh_proc, closure);
}


void
OksPopupMenu::show(XButtonPressedEvent *event) const
{
  XmMenuPosition(popup, event);
  XtManageChild(popup);
}


Widget
OksPopupMenu::add_separator(Widget w) const
{
  Widget separator = XmCreateSeparatorGadget(w ? w : popup, 0, 0, 0);
    
  XtManageChild(separator);

  return separator;
}


Widget
OksPopupMenu::add_label(const char *name, Widget w) const
{
  Widget label = XmCreateLabelGadget(w ? w : popup, (char *)name, 0, 0);
  
  XtManageChild(label);

  return label;
}


Widget
OksPopupMenu::addItem(const char *name, const long id, XtCallbackProc f, XtPointer p, Widget w) const
{
  Widget button = XmCreatePushButtonGadget(w ? w : popup, (char *)name, 0, 0);

  OksXm::set_user_data(button, (XtPointer)id);
  XtAddCallback(button, XmNactivateCallback, f, p);
  
  XtManageChild(button);

  return button;
}


Widget
OksPopupMenu::addToggleItem(const char *name, const long id, bool state, XtCallbackProc f, XtPointer p, Widget w) const
{
  Widget button = XtVaCreateWidget(
    name, xmToggleButtonWidgetClass,
    (w ? w : popup),
    XmNset, state, NULL
  );

  OksXm::set_user_data(button, (XtPointer)id);
  XtAddCallback(button, XmNvalueChangedCallback, f, p);
  
  XtManageChild(button);

  return button;
}


Widget
OksPopupMenu::addItem(const char *name, const char *name2, const long id, XtCallbackProc f, XtPointer p, Widget w) const
{
  if(name2) {
    std::string s(name);
    std::string s2(name2);

#ifdef POPUP_MENU_MAX_LENGTH
    size_t len = s2.length();
    if(len > POPUP_MENU_MAX_LENGTH) {
      s.append("... ");
      s2.replace(0, len - (POPUP_MENU_MAX_LENGTH - 4), "");
    }
#endif

    s.append(s2);
    return addItem(s.c_str(), id, f, p, w);	
  }
  else
    return addDisableItem(name, w);
}


Widget
OksPopupMenu::addItem(const char *name, OksObject *o, const OksKernel * k, const long id, XtCallbackProc f, XtPointer p, Widget w) const
{
  if(o) {
    std::string s(name);
    OksData d(o);

    std::string cvts = d.str(k);
    s += cvts;

    return addItem(s.c_str(), id, f, p, w);	
  }
  else
    return addDisableItem(name, w);
}


Widget
OksPopupMenu::addCascade(const char *name, Widget w)
{
  Widget menu_pane = XmCreatePulldownMenu (w ? w : popup, (char *)"menu_pane", 0, 0);

  XtVaCreateManagedWidget(
    (char *)name,
    xmCascadeButtonWidgetClass, w ? w : popup,
    XmNsubMenuId, menu_pane,
    NULL
  );

  return menu_pane;
}


Widget
OksPopupMenu::addDisableItem(const char *name, Widget w) const
{
  Widget button = XmCreatePushButtonGadget(w ? w : popup, (char *)name, 0, 0);
  XtSetSensitive(button, false);

  XtManageChild(button);

  return button;
}
