#include <iostream>

#include <oks/class.h>

#include "g_context.h"
#include "g_class.h"
#include "g_object.h"
#include "g_dnd.h"

#include <X11/cursorfont.h>
#include <Xm/AtomMgr.h>
#include <Xm/DragDrop.h>

unsigned char STATE_ICON_BITS[] = {
    0x00, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x78, 0x00, 0x00, 0x00,
    0xf8, 0x01, 0x00, 0x00, 0xf8, 0x01, 0x00, 0x00, 0xf8, 0x03, 0x00, 0x00,
    0xf0, 0x03, 0x00, 0x00, 0xf0, 0x07, 0x00, 0x00, 0xc0, 0x0d, 0x00, 0x00,
    0x00, 0x1b, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00,
    0x00, 0xfc, 0x00, 0x00, 0x00, 0xf8, 0x01, 0x00, 0x00, 0xf0, 0x03, 0x00,
    0x00, 0xe0, 0x07, 0x00, 0x00, 0xc0, 0x0f, 0x00, 0x00, 0x80, 0x1f, 0x00,
    0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0xfc, 0x00,
    0x00, 0x00, 0xf8, 0x01, 0x00, 0x00, 0xf0, 0x01, 0x00, 0x00, 0xe0, 0x03,
    0x00, 0x00, 0xc0, 0x07, 0x00, 0x00, 0x80, 0x0f, 0x00, 0x00, 0x00, 0x1f,
    0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x00, 0x38,
    0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0xc0};

unsigned char STATE_ICON_MASK[] = {
    0x3f, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x03, 0x00, 0x00,
    0xfc, 0x03, 0x00, 0x00, 0xfc, 0x07, 0x00, 0x00, 0xfc, 0x07, 0x00, 0x00,
    0xfc, 0x07, 0x00, 0x00, 0xf8, 0x07, 0x00, 0x00, 0xf8, 0x0f, 0x00, 0x00,
    0xe0, 0x1f, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00,
    0x00, 0xfc, 0x00, 0x00, 0x00, 0xf8, 0x01, 0x00, 0x00, 0xf0, 0x03, 0x00,
    0x00, 0xe0, 0x07, 0x00, 0x00, 0xc0, 0x0f, 0x00, 0x00, 0x80, 0x1f, 0x00,
    0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0xfe, 0x01, 0x00, 0x00, 0xfc, 0x03,
    0x00, 0x00, 0xf8, 0x03, 0x00, 0x00, 0xf0, 0x07, 0x00, 0x00, 0xe0, 0x0f,
    0x00, 0x00, 0xc0, 0x1f, 0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x00, 0x7f,
    0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0xfe,
    0x00, 0x00, 0x00, 0xfc, 0x00, 0x00, 0x00, 0xf0};

unsigned char INVALID_ICON_BITS[] = {
    0x00, 0xe0, 0x0f, 0x00, 0x00, 0xfc, 0x7f, 0x00, 0x00, 0xff, 0xff, 0x01,
    0x80, 0xff, 0xff, 0x03, 0xc0, 0x1f, 0xf0, 0x07, 0xe0, 0x07, 0xc0, 0x0f,
    0xf0, 0x07, 0x00, 0x1f, 0xf8, 0x0f, 0x00, 0x3e, 0xf8, 0x1f, 0x00, 0x3c,
    0xfc, 0x3f, 0x00, 0x7c, 0x3c, 0x7f, 0x00, 0x78, 0x3c, 0xfe, 0x00, 0x78,
    0x1e, 0xfc, 0x01, 0xf0, 0x1e, 0xf8, 0x03, 0xf0, 0x1e, 0xf0, 0x07, 0xf0,
    0x1e, 0xe0, 0x0f, 0xf0, 0x1e, 0xc0, 0x1f, 0xf0, 0x1e, 0x80, 0x3f, 0xf0,
    0x1e, 0x00, 0x7f, 0xf0, 0x3c, 0x00, 0xfe, 0x78, 0x3c, 0x00, 0xfc, 0x79,
    0x7c, 0x00, 0xf8, 0x7f, 0x78, 0x00, 0xf0, 0x3f, 0xf8, 0x00, 0xe0, 0x3f,
    0xf0, 0x01, 0xc0, 0x1f, 0xe0, 0x07, 0xc0, 0x0f, 0xc0, 0x1f, 0xf0, 0x07,
    0x80, 0xff, 0xff, 0x03, 0x00, 0xff, 0xff, 0x01, 0x00, 0xfc, 0x7f, 0x00,
    0x00, 0xe0, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00};









    //
    // This callback procedure removes the old cursor icon
    //

void
G_Object::objDragDropFinishCB(Widget /* w */, XtPointer /* client */, XtPointer /* call */)
{
//  cerr << "call G_Object::objDragDropFinishCB()\n";

  stop_auto_scrolling();

  G_Context::enable_tooltips();

// FIXME @@ remove this after all icons will be ready
//  Widget sourceCursorIcon;

//  XtVaGetValues(w, XmNsourceCursorIcon, &sourceCursorIcon, 0);
//  XtDestroyWidget(sourceCursorIcon);
}


    //
    // This callback procedure redraws the rectangles once the drop is completed
    //

void
G_Object::objDropFinishCB(Widget, XtPointer client, XtPointer)
{
  G_Object * the_obj = (G_Object *)client;

  the_obj->dnd_state = none;
  
    // redraw object if it is on top
    // (highly probable the DnD was canceled)
    // FIXME: this always should work, but the redraw() can't be called
    //        if the object was moved

  if(the_obj->is_referenced() == false) the_obj->draw();
}


    // This callback procedure handle the drawing of the target
    // rectangle depending of the dropSiteStatus for drag over
    // effects.

void
G_Object::objDragMotionCB(Widget /* w */, XtPointer /* client */, XtPointer /* call */)
{
//    XmDragMotionCallback    cb = (XmDragMotionCallback) call;
//    DragConvertPtr          conv = (DragConvertPtr) client;
//    Display                 *display;
//    Window                  window;
//    RectPtr                 rect;

//    cerr << "call G_Object::objDragMotionCB(): ";

//    if (cb->dropSiteStatus == XmVALID_DROP_SITE) {
//      cerr << "enter valid drop site" << endl;
        // re-stipple the rectangle when the poitner is inside the drop site
/*        if ( appInfo->clearRect == 0 && appInfo->doMove) {

            display = XtDisplay(conv->widget);
            window = XtWindow(conv->widget);
            rect = conv->rect;

            RectHide(display, window, rect);
            RectDrawStippled(display, window, rect);

        }
*/
//    }
//    else {
//      cerr << "enter non-valid drop site" << endl;

        // re-fill the rectangle when the poitner is outside the drop site
/*        if (appInfo->clearRect != 0 && appInfo->doMove) {
            appInfo->clearRect = 0;
            RedrawRectangles(conv->widget);
        }
*/
//    }
    
//    cerr << "x = " << cb->x << ", y = " << cb->y << ", reason = " << (int)cb->reason << ", operations = "
//         << (int)cb->operations << ", operation = " << (int)cb->operation << endl;
    
    
}


    // This callback procedure handle the drawing of the target
    // rectangle When the operation changes.
void
G_Object::objOperationChangedCB(Widget /* w */, XtPointer /* client */, XtPointer /* call */)
{

//    XmDragMotionCallback    cb = (XmDragMotionCallback) call;
//    DragConvertPtr          conv = (DragConvertPtr) client;
//    Display                 *display;
//    Window                  window;
//    RectPtr                 rect;

//    cerr << "call G_Object::objOperationChangedCB() with client data = \'" << (void *)client << "\'\n";

    /* re-stipple the rectangle when the poitner is inside the drop site */
//    if ( appInfo->clearRect == 0 && appInfo->doMove) {

//        display = XtDisplay(conv->widget);
//        window = XtWindow(conv->widget);
//        rect = conv->rect;

//        RectHide(display, window, rect);
//        RectDrawStippled(display, window, rect);

//    }

    /* re-fill the rectangle when the operation changes to copy */
//    if (appInfo->clearRect != 0 && !appInfo->doMove) {
//        appInfo->clearRect = 0;
//        RedrawRectangles(conv->widget);
//    }

}

    //
    // This is a selection conversion function that is used in converting requests
    // for rectangle targets. The return types follow ICCC statndards.
    //

Boolean
G_Object::objDragDropConvert(Widget w, Atom * /* selection */, Atom *target, Atom *type, XtPointer *value, unsigned long *length, int *format)
{
  Display * display = XtDisplay(w);
  Atom      oksObjectAtom = XmInternAtom(display, (char *)OksObjectDragAndDropInfo::atom_oks_object_name, False);
  Atom      oksObjectInfoAtom = XmInternAtom(display, (char *)OksObjectDragAndDropInfo::atom_oks_object_info_name, False);
  Atom      DELETE = XmInternAtom(display, (char *)"DELETE", False);
  Atom      TARGETS = XmInternAtom(display, (char *)"TARGETS", False);
  Atom      MULTIPLE = XmInternAtom(display, (char *)"MULTIPLE", False);
  Atom      TIMESTAMP = XmInternAtom(display, (char *)"TIMESTAMP", False);
  const int MAX_TARGS = 6;

  G_Object *g_obj = 0;
  XtVaGetValues(w, XmNclientData, &g_obj, NULL);


    // get the widget that initiated the drag

  Widget widget = g_obj->gc->get_widget();


    //
    // Make sure we are doing a motif drag by checking if the widget that
    // is passed in is a drag context. Make sure the widget in the client
    // data is not NULL.
    //

  if (!XmIsDragContext(w) || widget == 0) return False;


  if (*target == oksObjectAtom) {
    OksObjectDragAndDropInfo *data2 = OksObjectDragAndDropInfo::create(
      g_obj->get_id().c_str(),
      g_obj->get_oks_object()->GetClass()->get_name().c_str(),
      widget
    );

    *value = (XtPointer)data2;
    *type = oksObjectInfoAtom;
    *length = data2->info_length;
    *format = 8;
  }
//    else if (*target == DELETE) {

        /* delete the old rectangle */
//        RectHide(XtDisplay(widget), XtWindow(widget), conv->rect);
//        RectFree(conv->rect);

//        conv->rect = 0;
        /*
         * DELETE target return parameters MUST be assigned as follows to
         * ICCC compliant.
         */
//        *value = 0;
//        *type = XmInternAtom(XtDisplay(w), "NULL", False);
//        *length = 0;
//        *format = 8;

 //   }
  else if (*target == TARGETS) {

        // This target is required by ICCC

    Atom *targs = (Atom *)XtMalloc((unsigned) (MAX_TARGS * sizeof(Atom)));
    int target_count = 0;

    *value = (XtPointer) targs;
    *targs++ = oksObjectAtom;
    target_count++;
    *targs++ = DELETE;
    target_count++;
    *targs++ = TARGETS;
    target_count++;
    *targs++ = MULTIPLE;
    target_count++;		/* supported in the Intrinsics */
    *targs++ = TIMESTAMP;
    target_count++;		/* supported in the Intrinsics */
    *type = XA_ATOM;
    *length = (target_count * sizeof(Atom)) >> 2;
    *format = 32;

  }
  else
    return False;

  return True;
}


  // This function creates the Drag Icon
/*static Widget
GetDragIcon(Widget w, Pixmap icon, Pixmap iconMask, Dimension width, Dimension height, Pixel background, Pixel foreground)
{
    Widget  dragIcon;
    Arg     args[10];
    int     n = 0;

    XtSetArg(args[n], XmNhotX, 0); n++;
    XtSetArg(args[n], XmNhotY, 0); n++;
    XtSetArg(args[n], XmNwidth, width); n++;
    XtSetArg(args[n], XmNheight, height); n++;
    XtSetArg(args[n], XmNbackground, background); n++;
    XtSetArg(args[n], XmNforeground, foreground); n++;
    XtSetArg(args[n], XmNpixmap, icon); n++;
    XtSetArg(args[n], XmNmask, iconMask); n++;
    dragIcon = XmCreateDragIcon(w, "dragIcon", args, n);

    return(dragIcon);
}
*/


  // This function creates the bitmaps for the icon and the mask
  // and then calls GetDragIcon() to  create the drag icon.
/*
static Widget
GetDragIconFromBits(Widget w, char *bits, char *mask, Dimension width, Dimension height, Pixel background, Pixel foreground)
{
  Display * display = XtDisplay(w);

  Pixmap icon     = XCreateBitmapFromData(display, DefaultRootWindow(display), bits, width, height);
  Pixmap iconMask = XCreateBitmapFromData(display, DefaultRootWindow(display), mask, width, height);

  return(GetDragIcon(w, icon, iconMask, width, height, background, foreground));
}
*/

Widget
G_Object::get_drag_icon(Pixel background, Pixel foreground) const
{
  if(g_class->get_icon_bits() == 0) return 0;

  Display * display = XtDisplay(gc->get_widget());

      // create icons for given graphic class

  Pixmap iconPixmap = XCreateBitmapFromData(
    display,
    DefaultRootWindow(display),
    (char *)g_class->get_icon_bits(),
    g_class->get_icon_width(),
    g_class->get_icon_height()
  );

  Pixmap iconMask = XCreateBitmapFromData(
    display,
    DefaultRootWindow(display),
    (char *)g_class->get_icon_mask_bits(),
    g_class->get_icon_mask_width(),
    g_class->get_icon_mask_height()
  );

  if(!iconPixmap || !iconMask) return 0;

    // create drag icon from created icons

  Arg     args[10];
  int     n = 0;

  XtSetArg(args[n], XmNhotX, 0); n++;
  XtSetArg(args[n], XmNhotY, 0); n++;
  XtSetArg(args[n], XmNwidth, g_class->get_icon_width()); n++;
  XtSetArg(args[n], XmNheight, g_class->get_icon_height()); n++;
  XtSetArg(args[n], XmNbackground, background); n++;
  XtSetArg(args[n], XmNforeground, foreground); n++;
  XtSetArg(args[n], XmNpixmap, iconPixmap); n++;
  XtSetArg(args[n], XmNmask, iconMask); n++;

  return(XmCreateDragIcon(gc->get_widget(), (char *)"dragIcon", args, n));
}


void
G_Object::create_drag_context(XButtonEvent *event)
{
  static XtCallbackRec dragDropFinishCB[] = {
      {G_Object::objDragDropFinishCB, 0},
      {0, 0}
  };

  static XtCallbackRec dropFinishCB[] = {
      {G_Object::objDropFinishCB, 0},
      {0, 0}
  };

  static XtCallbackRec dragMotionCB[] = {
      {G_Object::objDragMotionCB, 0},
      {0, 0}
  };

  static XtCallbackRec operationChangedCB[] = {
      {G_Object::objOperationChangedCB, 0},
      {0, 0}
  };

  Atom targets[1];
  Arg  args[20];
  int  n = 0;

    // initially only show the source icon

  XtSetArg(args[n], XmNblendModel, XmBLEND_JUST_SOURCE); n++;


    // Create the drag cursor icons

  Pixel background, foreground;

  XtVaGetValues(gc->get_widget(), XmNbackground, &background, XmNforeground, &foreground, NULL);

  Widget sourceIcon = get_drag_icon(background, foreground);

  //Widget sourceIcon = GetDragIconFromBits(gc->w, (char *)sw_object_bits, (char *)sw_object_mask_bits, 32, 32, background, foreground);
  //Widget stateIcon  = GetDragIconFromBits(gc->w, (char *)STATE_ICON_BITS, (char *)STATE_ICON_MASK, 32, 32, background, foreground);


    // set args for the drag cursor icons */

  XtSetArg(args[n], XmNcursorBackground, background); n++;
  XtSetArg(args[n], XmNcursorForeground, foreground); n++;
  
  if(sourceIcon) {
    XtSetArg(args[n], XmNsourceCursorIcon, sourceIcon); n++;
  }
  //XtSetArg(args[n], XmNstateCursorIcon,  stateIcon); n++;


    //
    // set up the available export targets.  These are targets that we
    // wish to provide data on
    //

  targets[0] = XmInternAtom(gc->get_display(), (char *)OksObjectDragAndDropInfo::atom_oks_object_name, False);
  
  XtSetArg(args[n], XmNexportTargets, targets); n++;
  XtSetArg(args[n], XmNnumExportTargets, 1); n++;


    //
    // Set up information to pass to the convert function and callback procs
    //

  //OksObjectDragAndDropInfo *data = OksObjectDragAndDropInfo::create(get_id(), obj->GetClass()->get_name(), gc->w, this);


    //
    // identify the conversion procedure and
    // the client data passed to the procedure
    //

  //XtSetArg(args[n], XmNclientData, (XtPointer)data); n++;
  XtSetArg(args[n], XmNclientData, (XtPointer)this); n++;
  XtSetArg(args[n], XmNconvertProc, G_Object::objDragDropConvert); n++;

    // identify the necessary callbacks and the client data to be passed */

  dragDropFinishCB[0].closure = (XtPointer)this;
  XtSetArg(args[n], XmNdragDropFinishCallback, dragDropFinishCB); n++;

  dropFinishCB[0].closure = (XtPointer)this;
  XtSetArg(args[n], XmNdropFinishCallback, dropFinishCB); n++;

  dragMotionCB[0].closure = (XtPointer)this;
  XtSetArg(args[n], XmNdragMotionCallback, dragMotionCB); n++;

  operationChangedCB[0].closure = (XtPointer)this;
  XtSetArg(args[n], XmNoperationChangedCallback, operationChangedCB); n++;


    // set the drag operations that are supported

  XtSetArg(args[n], XmNdragOperations, XmDROP_LINK); n++;

    // start the drag. This creates a drag context.

  XmDragStart(gc->get_widget(), (XEvent *)event, args, n);
}
