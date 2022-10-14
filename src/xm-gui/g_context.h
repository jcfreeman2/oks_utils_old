#ifndef _CONFDB_GUI_G_CONTEXT_
#define _CONFDB_GUI_G_CONTEXT_

#include <oks/xm_context.h>
#include "g_rectangle.h"

class G_Object;


  //
  // G_Context describes graphical context of the object
  //

class G_Context : public OksGC {

  friend class G_Object;
  friend class G_Window;
  friend class PostScriptContext;
  friend class ConfdbGuiRefTree;
  
  public:

    G_Context		();
    ~G_Context		();

    void		initialize();

    void		set_tips_text(const char *text) {tips_text = text;}
    int			draw_tips();
  
    void		create_edit_widget(G_Object *, size_t, XButtonEvent *, int = 0);
    void		destroy_edit_widget();
  
    static void		enable_tooltips();
    static void		disable_tooltips();

    unsigned short	get_icon_width() const {return p_icon_width;}
    unsigned short      get_ch_obj_dx() const {return p_ch_obj_dx;}
    unsigned short      get_ch_obj_dy() const {return p_ch_obj_dy;}
    unsigned short      get_ch_max_width() const {return p_ch_max_width;}
    unsigned short      get_obj_dx() const {return p_obj_dx;}
    unsigned short      get_obj_dy() const {return p_obj_dy;}


  private:

    Pixel		tips_pixel;

    XmFontList		font_list;

    unsigned short      p_icon_width;
    unsigned short      p_ch_obj_dx;
    unsigned short      p_ch_obj_dy;
    unsigned short      p_ch_max_width;
    unsigned short      p_obj_dx;
    unsigned short      p_obj_dy;

    unsigned short	pointer_x;
    unsigned short	pointer_y;

    Pixmap		one_pxm;
    unsigned int	one_pxm_w;
    unsigned int	one_pxm_h;

    Pixmap		many_pxm;
    unsigned int	many_pxm_w;
    unsigned int	many_pxm_h;

    Pixmap		aggregation_pxm;
    unsigned int	aggregation_pxm_w;
    unsigned int	aggregation_pxm_h;

    Pixmap		next_pxm;
    unsigned int	next_pxm_w;
    unsigned int	next_pxm_h;

    std::string		tips_text;
    Dimension		tips_x;
    Dimension		tips_y;
    Widget		tips_widget;

    G_Object *		edit_g_obj;
    size_t		edit_attr_count;
    Widget		attr_edit_widget;
  
    G_Rectangle		selected_attr;
  
    static void		editControlLosingFocusCB(Widget, XtPointer, XtPointer);

    static void		multiTokenCB(Widget, XtPointer, XtPointer);
    static void		singleTokenCB(Widget, XtPointer, XtPointer);
  
    static void		multiBoolCB(Widget, XtPointer, XtPointer);
    static void		singleBoolCB(Widget, XtPointer, XtPointer);
  
    static void		unselectAC(Widget, XtPointer, XEvent *, Boolean *);
};

#endif
