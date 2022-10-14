#ifndef __OKS_MESSAGE_WINDOW_H
#define __OKS_MESSAGE_WINDOW_H


#include <iostream>
#include <string>

#include <oks/xm_dialog.h>

class OksWindowBuffer;

class OksXmMessageWindow : public OksDialog
{
  public:

      // the messages longer BufSize without '\n' will be wraped

    enum {
      BufSize = 1024
    };

    enum {
      idMessageWnd = 100,
      idAcMsgWndClear,
      idAcMsgWndLog,
      idAcMsgCopyClipboard
    };

    OksXmMessageWindow	(const char *, OksTopDialog *, bool auto_scroll);
    ~OksXmMessageWindow	();
	
    int			put(const char *, int);
    void		show_window();


  private:

    Widget              p_log_w;
    bool		p_is_visible;
    bool		p_save_on_exit;
    bool		p_auto_scroll;
    std::string		p_filename;
    char		buffer[BufSize];
    int			pnt;

    OksWindowBuffer *   p_wnd_buffer;
    std::streambuf *    p_old_out_buffer;
    std::streambuf *    p_old_err_buffer;

    void		save(const char *) const;

    static void		switchCB(Widget, XtPointer, XtPointer);
    static void		popupCB(Widget, XtPointer, XtPointer);
    static void		saveCB(Widget, XtPointer, XtPointer);
    static void		selectCellCB(Widget, XtPointer, XtPointer);
    static void		unmapAC(Widget, XtPointer, XEvent *, Boolean *);
    static void		popupAC(Widget, XtPointer, XEvent *, Boolean *);
};


class OksWindowBuffer : public std::streambuf {
  OksXmMessageWindow *w;

public:
  OksWindowBuffer	(OksXmMessageWindow * wnd) { w = wnd; }

  int			sync();
  int			overflow(int);
  std::streamsize	xsputn(const char *, std::streamsize);
#ifdef _MSC_VER
  int			underflow() {return 0;}
#endif
};

#endif
