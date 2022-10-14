#define _OksXmBuildDll_

#include <fstream>

#include <oks/defs.h>

#include <Xm/CutPaste.h>
#include <Xm/List.h>
#include <Xm/Matrix.h>
#include <Xm/Text.h>
#include <Xm/SelectioB.h>
#include <Xm/RowColumn.h>
#include <Xm/ToggleB.h>

#include <oks/xm_msg_window.h>
#include <oks/xm_popup.h>
#include <oks/xm_utils.h>
#include <oks/cstring.h>

const short NumOfLogMatrixColumns = 1;
const size_t DefaultColumnWidth = 64;
size_t column_width = DefaultColumnWidth;

OksXm::Matrix::ResizeInfo resize_info;

void
OksXmMessageWindow::saveCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  if(((XmSelectionBoxCallbackStruct *)call_data)->reason == XmCR_OK) {
    char *fileName = XmTextGetString(XmSelectionBoxGetChild(w, XmDIALOG_VALUE_TEXT));
    OksXmMessageWindow *msg_wnd = (OksXmMessageWindow *)client_data;

    msg_wnd->p_filename = fileName;

    if(msg_wnd->p_save_on_exit) {
      std::cout << "Will save contenst on exit to \'" << fileName << "\'\n";
    }
    else {
      std::cout << "Saving contenst to \'" << fileName << "\'...\n";
	
      msg_wnd->save(fileName);
    }
	
    XtFree(fileName);
  }
}


void
OksXmMessageWindow::switchCB(Widget, XtPointer client_data, XtPointer call_data)
{
  ((OksXmMessageWindow *)client_data)->p_save_on_exit = (
    ((XmToggleButtonCallbackStruct *)call_data)->set ? true : false
  );
}


void
OksXmMessageWindow::popupCB(Widget w, XtPointer client_data, XtPointer call_data) 
{
  OksXmMessageWindow * msg_wnd = (OksXmMessageWindow *)client_data;
  Widget p_log_w = msg_wnd->get_widget(idMessageWnd);

  switch ((long)OksXm::get_user_data(w)) {
    case idAcMsgWndClear:
      XbaeMatrixDeleteRows(p_log_w, 0, XbaeMatrixNumRows(p_log_w));
      short widths[NumOfLogMatrixColumns];
      widths[0]=resize_info.column_width=DefaultColumnWidth;
      XtVaSetValues(p_log_w, XmNcolumnWidths, widths, NULL);
      resize_info.num_of_rows = 0;
      break;
    
    case idAcMsgWndLog: {
      Arg args[5];
      XmString string = OksXm::create_string("Type name of log file:");
      Widget dialog;
 
      XtSetArg(args[0], XmNdialogStyle, XmDIALOG_APPLICATION_MODAL);
      XtSetArg(args[1], XmNselectionLabelString, string);
      XtSetArg(args[2], XmNchildPlacement, XmPLACE_BELOW_SELECTION);
 
      dialog = XmCreatePromptDialog(XtParent(p_log_w), (char *)"prompt dialog", args, 3);
      XtVaSetValues(XtParent(dialog), XmNtitle, "Save Log As", NULL);

      Widget work_area = XmCreateRowColumn(dialog, (char *)"workarea", args, 0);
      XtManageChild(work_area);

      Widget button = XmCreateToggleButton(work_area, (char *)"Save When Exit", args, 0);
      XtManageChild(button);
      XtAddCallback(button, XmNvalueChangedCallback, switchCB, client_data);
      
      if(msg_wnd->p_save_on_exit)
        XtVaSetValues(button, XmNset, true, NULL);
      if(msg_wnd->p_filename.length())
        XmTextSetString(
          XmSelectionBoxGetChild(dialog, XmDIALOG_TEXT),
          const_cast<char *>(msg_wnd->p_filename.c_str())
        );

      XtAddCallback(dialog, XmNokCallback, OksXmMessageWindow::saveCB, client_data);
      XtAddCallback(dialog, XmNcancelCallback, OksXmMessageWindow::saveCB, client_data);

      XtManageChild(dialog);
      XmStringFree(string);
      break;
    }

    case idAcMsgCopyClipboard: {
      if(XmClipboardLock(XtDisplay(p_log_w), XtWindow(p_log_w)) != ClipboardSuccess) {
        std::cerr << "ERROR: XmClipboardLock() failed" << std::endl;
        return;
      }

      Time time = ((XButtonPressedEvent *)(((XmPushButtonCallbackStruct *)call_data)->event))->time;

      XmString s = XmStringCreateSimple(const_cast<char *>("OKS_GUI"));
      long itemID = 0;
      int stat = XmClipboardStartCopy(XtDisplay(p_log_w), XtWindow(p_log_w), s, time, w, NULL, &itemID);
      XmStringFree(s);
      if (stat != ClipboardSuccess) {
        std::cerr << "ERROR: XmClipboardStartCopy() failed" << std::endl;
        return;
      }

      const String **rows;

      XtVaGetValues(p_log_w, XmNcells, &rows, NULL);

      std::ostringstream text;

      for(unsigned int i = 0; i < resize_info.num_of_rows; i++) {
        text << rows[i][0] << std::endl;
      }

      if(XmClipboardCopy(XtDisplay(p_log_w), XtWindow(p_log_w), itemID, const_cast<char *>("STRING"), (XtPointer)text.str().c_str(), text.str().size(), 0, 0) != ClipboardSuccess) {
        std::cerr << "ERROR: XmClipboardCopy() failed" << std::endl;
        return;
      }

      if(XmClipboardEndCopy(XtDisplay(p_log_w), XtWindow(p_log_w), itemID) != ClipboardSuccess) {
        std::cerr << "ERROR: XmClipboardEndCopy() failed" << std::endl;
        return;
      }

      if(XmClipboardUnlock(XtDisplay(p_log_w), XtWindow(p_log_w), False) != ClipboardSuccess) {
        std::cerr << "ERROR: XmClipboardUnlock() failed" << std::endl;
      }

      break;
    }
  }
}


void
OksXmMessageWindow::popupAC(Widget w, XtPointer client_data, XEvent *event, Boolean *)
{
  if(((XButtonEvent *)event)->button != Button3) return;

  const char *szClearLog = "Clear Log";
  const char *szMakeLogFile = "Save Log to File";
  const char *szCopyClipboard = "Copy Log to Clipboard";

  OksPopupMenu popup(w);

  if(resize_info.num_of_rows) {
    popup.addItem(szClearLog, idAcMsgWndClear, popupCB, client_data);
    popup.addItem(szMakeLogFile, idAcMsgWndLog, popupCB, client_data);
    popup.addItem(szCopyClipboard, idAcMsgCopyClipboard, popupCB, client_data);
  }
  else {
    popup.addDisableItem(szClearLog);
    popup.addDisableItem(szMakeLogFile);
    popup.addDisableItem(szCopyClipboard);
  }

  popup.show((XButtonPressedEvent *)event);
}


void
OksXmMessageWindow::unmapAC(Widget, XtPointer d, XEvent *e, Boolean *)
{
  if(e->type != UnmapNotify) return;
  
  ((OksXmMessageWindow *)d)->p_is_visible = false;
}

void
OksXmMessageWindow::selectCellCB(Widget, XtPointer, XtPointer cb)
{
  XbaeMatrixEnterCellCallbackStruct *cbs = (XbaeMatrixEnterCellCallbackStruct *)cb;

  cbs->doit = False;
  cbs->map = True;
}

Pixel error_pixel;
Pixel warning_pixel;

OksXmMessageWindow::OksXmMessageWindow(const char *t, OksTopDialog * top, bool auto_scroll) :
  OksDialog	   (t, top),
  p_is_visible	   (true),
  p_save_on_exit   (false),
  p_auto_scroll    (auto_scroll),
  p_filename	   (""),
  p_wnd_buffer     (0),
  p_old_out_buffer (0),
  p_old_err_buffer (0)
{
  resize_info.column_width = DefaultColumnWidth;
  resize_info.num_of_rows = 0;

  XtVaSetValues(XtParent(get_form_widget()), XmNdeleteResponse, XmUNMAP, NULL);

    // create list widget

  p_log_w = add_matrix(idMessageWnd, (void *)"Message Log:");
  attach_right(idMessageWnd);

  short widths[NumOfLogMatrixColumns];
  widths[0]=resize_info.column_width;
  Dimension fixed_columns = 1;

  XtVaSetValues(
    p_log_w,
    XmNcolumns, NumOfLogMatrixColumns,
    XmNcolumnWidths, widths,
    XmNcolumnLabels, NULL,
    XmNfixedColumns, fixed_columns,
    XmNrows, 0,
    XmNcellHighlightThickness, (Dimension)0,
    XmNcellMarginHeight, (Dimension)0,
    XmNcellShadowThickness, (Dimension)0,
    XmNtraverseFixedCells, False,
    NULL
  );


  error_pixel = OksXm::string_to_pixel(p_log_w, "red");          // set red color for errors
  warning_pixel = OksXm::string_to_pixel(p_log_w, "dark blue");  // set dark blue color for warnings

  attach_bottom(idMessageWnd);

  XtAddCallback(p_log_w, XmNenterCellCallback, selectCellCB, (XtPointer)this);
  XtAddCallback(p_log_w, XmNresizeCallback, OksXm::Matrix::resizeCB, (XtPointer)&resize_info);

  XtAddEventHandler(p_log_w, ButtonPressMask, False, popupAC, (XtPointer)this);

  XtVaSetValues(form, XmNwidth, 400, XmNheight, 128, NULL);

  XtAddEventHandler(get_form_widget(), StructureNotifyMask, False, unmapAC, (XtPointer)this);

  show();
  
  setMinSize();

  *buffer = '\0';
  pnt = 0;

  p_wnd_buffer = new OksWindowBuffer(this);

  p_old_out_buffer = std::cout.rdbuf(p_wnd_buffer);
  p_old_err_buffer = std::cerr.rdbuf(p_wnd_buffer);
}


OksXmMessageWindow::~OksXmMessageWindow()
{
  std::cout.rdbuf(p_old_out_buffer);
  std::cerr.rdbuf(p_old_err_buffer);

  if(p_save_on_exit && p_filename.length()) save(p_filename.c_str());

  XtUnmanageChild(form);
  
  delete p_wnd_buffer;
}


void
OksXmMessageWindow::show_window()
{
  if(p_is_visible) {
	XMapRaised(XtDisplay(get_form_widget()), XtWindow(XtParent(get_form_widget())));
  }
  else {
	XtManageChild(get_form_widget());
	p_is_visible = true;
  }
}


void
OksXmMessageWindow::save(const char *file) const
{
  std::ofstream f(file);

  if(!f.good()) {
    Oks::error_msg("OksXmMessageWindow::save(const char *) const")
      << "Can't create file \"" << file << "\" to save log\n";

    return;
  }

  int itemCount;
  const String **rows;

  XtVaGetValues(get_widget(idMessageWnd), XmNrows, &itemCount, XmNcells, &rows, NULL);

  for(int i = 0; i < itemCount; i++) {
    f << rows[i][0] << std::endl;
  }
}

static Pixel
tag2pixel(const char * msg, int len)
{
  if(len > 5) {
    if(
      oks::cmp_str5n(msg, "ERROR") ||
      oks::cmp_str5n(msg, "Error") ||
      oks::cmp_str5n(msg, "error") ||
      oks::cmp_str5n(msg, "FATAL")
    ) return error_pixel;
  }

  if(len > 7) {
    if(
      oks::cmp_str7n(msg, "WARNING") ||
      oks::cmp_str7n(msg, "Warning") ||
      oks::cmp_str7n(msg, "warning")
    ) return warning_pixel;
  }

  return 0;
}

int
OksXmMessageWindow::put(const char *msg, int len)
{
  int    lpnt = 0;
  bool	 line_shown = false;
  bool   popup = false;
  
  int num_of_rows = 0;

  while(lpnt < len) {
    bool print_out = false;
    if(pnt != BufSize - 1) {
      buffer[pnt] = msg[lpnt++];

      if(buffer[pnt] == '\n') {
        print_out = true;
      }
      else {
        pnt++;
      }
    }
    else {
      print_out = true;
    }

    if(print_out) {
      buffer[pnt] = '\0';
      Pixel tag = tag2pixel(buffer, pnt);

      num_of_rows = XbaeMatrixNumRows(p_log_w);

      const size_t put_item_at(resize_info.num_of_rows);  //  cannot use resize_info.num_of_rows since it must be incremented in resize callback
      resize_info.num_of_rows++;

      if(put_item_at < (size_t)num_of_rows) {
        XbaeMatrixSetCell(p_log_w, put_item_at, 0, buffer);
      }
      else {
        String row[NumOfLogMatrixColumns];
        row[0] = buffer;
        XbaeMatrixAddRows(p_log_w, put_item_at, row, 0, 0, 1);
      }

      if(Pixel tag = tag2pixel(buffer, pnt)) {
        XbaeMatrixSetCellColor(p_log_w, put_item_at, 0, tag);
      }

      size_t len=strlen(buffer);
      
      if(len>resize_info.column_width) {
        resize_info.column_width = len;
        short widths[NumOfLogMatrixColumns];
        widths[0]=resize_info.column_width;
        XtVaSetValues(p_log_w, XmNcolumnWidths, widths, NULL);
      }
      
      
      line_shown = true;
      if(tag) popup = true;
      pnt = 0;
    }
  }

  if(line_shown) {
    if(p_auto_scroll || popup) {
      int visibleItemCount = XbaeMatrixVisibleRows(p_log_w);
      
      if(visibleItemCount < num_of_rows) {
        XbaeMatrixEditCell(p_log_w, num_of_rows-visibleItemCount, 0);         // workaround Xbae matrix problem: move focus - otherwise redraw shows first row
        XbaeMatrixMakeCellVisible(p_log_w, num_of_rows, 0);                   // make last row visible
	XbaeMatrixCancelEdit(p_log_w, True);                                  // remove edit
      }
    }

    show_window();
  }

  return len;
}


int
OksWindowBuffer::sync ()
{
  std::streamsize n = pptr() - pbase();
  return (
	(n && w->put(pbase(), n) != n)
		? EOF
		: 0
  );
}


int
OksWindowBuffer::overflow (int ch)
{
  std::streamsize n = pptr() - pbase();
  if(n && sync()) return EOF;
  if(ch != EOF) {
	char cbuf[1]; cbuf[0] = ch;
	if(w->put(cbuf, 1) != 1) return EOF;
  }
  pbump(-n);
  
  return 0;
}


std::streamsize
OksWindowBuffer::xsputn (const char* text, std::streamsize n)
{
  return ( (sync() == EOF) ? 0 : w->put(text, n) );
}
