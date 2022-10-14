#include <iostream>

#include "data_editor_main_dlg.h"
#include "data_editor_exceptions.h"
#include "g_window.h"
#include "g_class.h"
#include "g_object.h"
#include "g_dnd.h"

#include <oks/xm_utils.h>

#include <Xm/Xm.h>
#include <Xm/AtomMgr.h>
#include <Xm/DropSMgr.h>
#include <Xm/DropTrans.h>
#include <Xm/DragC.h>
#include <Xm/DrawingA.h>
#include <Xm/Screen.h>


	//
	// This struct is used by G_Window::dropCB method to pass information
	// to the G_Window::transferCB method in accordance with D&D standard
	//

struct G_WindowDropInfo {
  G_Window * g_window;
  XButtonEvent * event;
};

    //
    // This procedure handles the data that is being transfer
    //

void
G_Window::transferCB(Widget w, XtPointer closure, Atom *, Atom *type, XtPointer value, unsigned long *, int *)
{
  G_WindowDropInfo * data = (G_WindowDropInfo *)closure;
  Display * display = XtDisplay(data->g_window->d_area);
  Atom oksObjectInfoAtom = XmInternAtom(display, (char *)OksObjectDragAndDropInfo::atom_oks_object_info_name, False);
  Atom nullAtom = XmInternAtom(display, (char *)"NULL", False);


    //
    // The delete target returns a nullAtom type and value equal to NULL
    // so it isn't a failure.  Otherwise, check for NULL value or targets
    // that wee don't support and set transfer failure.
    //

  if(
   *type != nullAtom &&
   (
     !value ||
     (*type != oksObjectInfoAtom && *type != XA_DRAWABLE)
   )
  ) {
    XtVaSetValues(
      w,
      XmNtransferStatus, XmTRANSFER_FAILURE,
      XmNnumDropTransfers, 0,
      NULL
    );

    if(value) XtFree((char *)value);

    return;
  }

  if(*type == oksObjectInfoAtom) {
    OksObjectDragAndDropInfo *dnd_info = (OksObjectDragAndDropInfo *)value;

    OksObject *src_obj = data->g_window->parent->find_oks_object(
      dnd_info->get_class_name(),
      dnd_info->get_object_id()
    );

    if(src_obj) {
      DragAndDropTransferResult *t_info = data->g_window->dnd_dest_obj->get_user_dnd_action(src_obj, data->event);

      if(t_info) {
        switch (t_info->get_type()) {
	  case DragAndDropTransferResult::link:
            OksDataEditorMainDialog::link_rel_objects(
	      t_info->get_rel_name(),
	      data->g_window->dnd_dest_obj,
	      src_obj
	    );
	    break;

	  case DragAndDropTransferResult::move:
            OksDataEditorMainDialog::swap_rel_objects(
	      (static_cast<DragAndDropMoveTransferResult *>(t_info))->get_parent()->get_oks_object(),
	      t_info->get_rel_name(),
	      src_obj,
	      data->g_window->dnd_dest_obj->get_oks_object()
	    );
	    break;

	}

	delete t_info;
      }
    }

    data->g_window->dnd_dest_obj->set_drag_and_drop_state(G_Object::none);
    data->g_window->dnd_dest_obj->draw();
    data->g_window->dnd_dest_obj = 0;
  }

    /* Handle pixel type (i.e. change in background) */
//    if (*type == PIXEL) {
//        rect = RectFind(transferRec->x, transferRec->y);
//        RectSetColor(rect, display, XtWindow(wid), *((Pixel*)value));
//    }
    /* Handle drawable type (i.e. change in pixmap) */
//    else if (*type == XA_DRAWABLE) {
//        rect = RectFind(transferRec->x, transferRec->y);
//        RectSetPixmap(rect, display, XtWindow(wid), *((Pixmap *)value));
//    }
    /* Handle rect_info type (i.e. new rectangle) */
//    else if (*type == oksObjectInfoAtom) {
//        rect = (RectPtr) value;
//        RectRegister(rect, transferRec->x, transferRec->y);
//        value = NULL; /* No need to free, it is being stored in RecTable */
//    }

    // Free the value if there is one, or we would have a memory leak
  if(value) XtFree((char *)value);
}


void
G_Window::dropDestroyCB(Widget, XtPointer clientData, XtPointer)
{
  XtFree((char *)clientData);
}

    //
    // The procedure either begins the drop of initiates the help dialog
    // depending on the dropAction.
    //

void
G_Window::dropCB(Widget w, XtPointer /* client */, XtPointer call)
{
  XmDropProcCallbackStruct *cb = (XmDropProcCallbackStruct *)call;

  if(cb->dropAction == XmDROP_HELP) ers::error(OksDataEditor::InternalProblem(ERS_HERE, "ask for HELP is not supported"));

  
  Arg args[10];
  int n = 0;
  XmDropTransferEntryRec transferEntries[1];

  G_Window * g_window = (G_Window *)OksXm::get_user_data(w);

  if(
   (g_window->dnd_dest_obj != 0) &&
   (g_window->dnd_dest_obj->get_drag_and_drop_state() == G_Object::dest_invalid)
  ) {
    g_window->dnd_dest_obj->set_drag_and_drop_state(G_Object::none);
    g_window->dnd_dest_obj->draw();
    g_window->dnd_dest_obj = 0;
  }

  if(g_window->dnd_dest_obj && (cb->operations & XmDROP_LINK)) {

      // callcack to destroy transferred data

    static XtCallbackRec dropDestroyRecCB[] = {
      {G_Window::dropDestroyCB, 0},
      {0, 0}
    };


      // set operation type and valid drop site status

    cb->operation = XmDROP_LINK;
    cb->dropSiteStatus = XmVALID_DROP_SITE;


      // create event description required to create popup menu

    static XButtonEvent event;

    XQueryPointer(
      XtDisplay(g_window->d_area), XtWindow(g_window->d_area),
      &event.root, &event.window,
      &event.x_root, &event.y_root, &event.x, &event.y,
      &event.state
    );


      // create data to be passed to transfer function

    G_WindowDropInfo * data = (G_WindowDropInfo *)XtMalloc(sizeof(G_WindowDropInfo));
    data->g_window = g_window;
    data->event = &event;


      // setup transfer data
    
    transferEntries[0].target = XmInternAtom(XtDisplay(w), (char *)OksObjectDragAndDropInfo::atom_oks_object_name, False);
    transferEntries[0].client_data = (XtPointer)data;

    XtSetArg(args[n], XmNdropTransfers, transferEntries); n++;
    XtSetArg(args[n], XmNnumDropTransfers, 1); n++;


      // setup destroy callback to free transfer data

    dropDestroyRecCB[0].closure = (XtPointer)data;
    XtSetArg(args[n], XmNdestroyCallback, dropDestroyRecCB); n++;

      // setup transfer proc to accept the drop transfer data

    XtSetArg(args[n], XmNtransferProc, G_Window::transferCB); n++;
  }
  else {
    cb->operation = XmDROP_NOOP;
    cb->dropSiteStatus = XmINVALID_DROP_SITE;

    XtSetArg(args[n], XmNtransferStatus, XmTRANSFER_FAILURE); n++;
    XtSetArg(args[n], XmNnumDropTransfers, 0); n++;
  }

  XmDropTransferStart(cb->dragContext, args, n);
}


    //
    // This procedure handles drop site messages and performs the
    // appropriate drag under effects.
    //

void
G_Window::dragCB(Widget w, XtPointer /* client */, XtPointer call)
{
  XmDragProcCallbackStruct *cb = (XmDragProcCallbackStruct *)call;

  G_Window * g_window = (G_Window *)OksXm::get_user_data(w);
  G_Object * g_obj = g_window->find_object(cb->x, cb->y);

  if(g_obj) {
    if(g_obj->get_drag_and_drop_state() == G_Object::none)
      g_obj->set_drag_and_drop_state(g_obj->get_dest_link_state());

    g_obj->draw();
  }

  if(g_window->dnd_dest_obj != g_obj) {
    if(g_window->dnd_dest_obj) {
      if(g_window->dnd_dest_obj->get_drag_and_drop_state() != G_Object::src) {
        g_window->dnd_dest_obj->set_drag_and_drop_state(G_Object::none);
	g_window->dnd_dest_obj->draw();
      }
    }

    g_window->dnd_dest_obj = g_obj;
  }


  switch(cb->reason) {
    case XmCR_DROP_SITE_ENTER_MESSAGE:
      stop_auto_scrolling();
      if(g_window->dnd_dest_obj) {
        cb->operations &= XmDROP_LINK;
	cb->operation = (cb->operations) ? XmDROP_LINK : XmDROP_NOOP;
        cb->dropSiteStatus = XmVALID_DROP_SITE;
      }
      else {
        cb->operation = XmDROP_NOOP;
        cb->dropSiteStatus = XmINVALID_DROP_SITE;
      }
      break;

    case XmCR_DROP_SITE_MOTION_MESSAGE:
      if(g_window->dnd_dest_obj) {
        cb->operations &= XmDROP_LINK;
	cb->operation = (cb->operations) ? XmDROP_LINK : XmDROP_NOOP;
        cb->dropSiteStatus = XmVALID_DROP_SITE;
      }
      else {
        cb->operation = XmDROP_NOOP;
        cb->dropSiteStatus = XmINVALID_DROP_SITE;
      }
       break;

    case XmCR_DROP_SITE_LEAVE_MESSAGE:
      start_auto_scrolling(g_window);
      break;

    case XmCR_OPERATION_CHANGED:
      break;

    default:
      cb->dropSiteStatus = XmINVALID_DROP_SITE;
      break;

  }

    // allow animation to be performed
  cb->animate = True;
}


void
G_Window::register_drop_site(Widget w)
{
  Atom targets[] = { XmInternAtom(XtDisplay(w), (char *)OksObjectDragAndDropInfo::atom_oks_object_name, False) };

  Arg args[5];

  XtSetArg(args[0], XmNdropSiteOperations, XmDROP_LINK);
  XtSetArg(args[1], XmNimportTargets, targets);
  XtSetArg(args[2], XmNnumImportTargets, 1);
  XtSetArg(args[3], XmNdragProc, dragCB);
  XtSetArg(args[4], XmNdropProc, dropCB);

  XmDropSiteRegister(w, args, 5);
}


void
G_Window::unregister_drop_site(Widget w)
{
  XmDropSiteUnregister(w);
}
