#ifndef __OKS_XM_UTILS
#define __OKS_XM_UTILS

#include <Xm/Xm.h>
#include <X11/xpm.h>

#include <string>
#include <list>
#include <set>

namespace OksXm {

  class AutoCstring {
   private:
     char * p_value;

   public:
     AutoCstring (char * s) : p_value(s) {;}
     ~AutoCstring() { if(p_value) XtFree(p_value); }
     char * get() const {return p_value;}
     bool is_non_empty() const {return (p_value != 0 && *p_value != 0);}
  };

    // tips tool

  struct TipsInfo {
    XtTimerCallbackProc m_proc;
    Widget m_widget;
    const char * m_text;

    TipsInfo(XtTimerCallbackProc f, const char * p) : m_proc(f), m_widget(0), m_text(p) { ; }
    ~TipsInfo() { if(m_text) delete [] m_text; }
  };

  void               show_tips(Widget parent, TipsInfo * params);
  OksXm::TipsInfo *  show_label_as_tips(Widget);
  OksXm::TipsInfo *  show_text_as_tips(Widget w, const char * text);
  void               make_tips(Widget parent, const char * text);


    /**
     * \brief Wrap text into a pseudo-table string.
     *
     *  Convert several user title:value pairs into a pseudo-table string, e.g. produce string like:
     *
     *    Relationship   : RunsWhat
     *    Class type     : BaseApplication
     *    Cardinality    : 0..N
     *    Reference type : is composite - yes
     *                     is exclusive - no
     *                     is shared    - yes
     *    Description    : It is a test application object
     *                     with multi-line description
     */

  class TipsText {
    struct Pair {
      Pair(const std::string& t, const std::string& v) : title(t), value(v) { ; }
      std::string title;
      std::string value;
    };

    public:

      TipsText(char separator = ':');
      void add(const std::string& title, const std::string& value) { m_data.push_back(Pair(title,value)); }
      std::string text();

    private:

      std::list<Pair> m_data;
      char m_separator;
      static unsigned long m_max_width;

  };

    // cursor utility

  class ShowWatchCursor
  {
    public:

      ShowWatchCursor(Display * display, std::set<Widget>& widgets);
      ~ShowWatchCursor();

    private:

      Display * m_display;
      std::set<Widget> m_widgets;
  };


    // General utilities

  XtPointer	get_user_data(Widget);
  void		set_user_data(Widget, XtPointer);
  XmString	create_string(const char *);
  char *	string_to_c_str(const XmString);
  void		set_close_cb(Widget, XtCallbackProc, void *);
  void          remove_close_cb(Widget, XtCallbackProc, void *);
  Pixmap	create_color_pixmap(Widget, const char **);
  Widget	create_dlg_with_pixmap(Widget, const char *, const char **, const char *, const char *, const char *, const char *, ArgList, int);
  std::string	ask_name(Widget, const char *, const char *, const char * = 0, XtCallbackProc = 0, XtPointer = 0);
  std::string   ask_text(Widget, const char *, const char *, const char * = 0);
  std::string	ask_file(Widget, const char * = 0, const char * = 0, const char * = 0, const char * = 0, bool = true, XtCallbackProc = 0, XtPointer = 0);
  bool          ask_overwrite_file(Widget parent, const char * file);
  void		show_info(Widget, const char *, const char **, const char *, const char *);
  Widget        create_file_selection_box(Widget, const char *, const char *, const char *, const char * = "*", bool = true, XtCallbackProc = 0, XtPointer = 0, XtCallbackProc = 0, XtPointer = 0);

  XFontStruct*  get_font_from_list(XmFontList);
  Pixel		string_to_pixel(Widget, const char *);

  void          destroy_modal_dialogs();


   /**
    *  Initialization of mouse wheel actions for scrolled windows.
    *  Call once per application.
    */

  void init_mouse_wheel_actions(XtAppContext context);


   /**
    *  Add mouse wheel action to a widget, that has ScrolledWindow parent.
    */

  void add_mouse_wheel_action(Widget w);


    // XmLabel

  namespace Label {
    void set(Widget, const char *);
  }

    // XmList

  namespace List {
    void     sortCB(Widget, XtPointer, XtPointer);
    char *   get_selected_value(Widget);
    int	     get_selected_pos(Widget);
    char *   get_value(Widget, int);
    int      size(Widget);
    void     select_row(Widget, const char *);
    void     delete_row(Widget, const char *);
    void     add_row(Widget, const char *, Boolean = true, const char * = 0);
    void     move_item(Widget, int, bool);                  // true = up
    void     replace(Widget, const char *, const char *);   // (w, old, new)
  }


    // XbaeMatrix

  namespace Matrix {

    enum SortType {
      SortByString = 1,
      SortByUnsigned,
      SortByInteger,
      SortByDouble,
      SortByDate,
      SortByTime
    };
    
    struct ResizeInfo {
      size_t column_width;
      size_t num_of_rows;
    };
  
    void		sort_rows(Widget, int, SortType);
    bool                resort_rows(Widget);
    void		set_cell_background(Widget, Widget);
    void		adjust_column_widths(Widget, int = 300);
    void		set_widths(Widget, int = 3);
    void		set_visible_height(Widget, size_t = 4);
    size_t	        get_row_by_value(Widget, const char *);
    const char *	get_selected_value(Widget);
    XtPointer           get_selected_row_user_data(Widget);
    void                delete_row_by_user_data(Widget, XtPointer);
    void		select_row(Widget, const char *);
    void		cellCB(Widget, XtPointer, XtPointer);
    void		resizeCB(Widget, XtPointer, XtPointer);
  }


    // MenuBar

  namespace MenuBar {
    Widget add_push_button(Widget, const char *, long, XtCallbackProc);
    Widget add_toggle_button(Widget, const char *, long, int, XtCallbackProc);
    Widget add_cascade_button(Widget, Widget, const char *, char);
    void   add_separator(Widget);
  }

  
    // XmRowColumn

  namespace RowColumn {
    Widget create_pixmap_and_text(Widget, const char **, const char *);
  }


    // XmTextField
  
  namespace TextField {
    void set_editable(Widget);
    void set_not_editable(Widget);
    void set_width(Widget, short = 0);
  }

}

#endif
