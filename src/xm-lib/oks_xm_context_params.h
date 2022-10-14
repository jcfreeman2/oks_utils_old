#ifndef __OKS_GC_CONTEXT_PARAMETERS_DIALOG_H
#define __OKS_GC_CONTEXT_PARAMETERS_DIALOG_H

#include <oks/xm_dialog.h>
#include <oks/xm_context.h>



class OksGCParameters : public OksDialog
{

  public:

    OksGCParameters	(OksTopDialog * top, OksGC *, const std::string&, OksGC::ApplyFN, XtPointer);
    virtual		~OksGCParameters();

    Widget		get_dialog() const {return dlg_w;}

    void		show_parameters();
    void		read_parameters();


  private:

    OksGC *		gc;
    Widget		dlg_w;
    OksGC::ApplyFN	apply_f;
    XtPointer		apply_f_p;

    std::list<std::string> p_to_be_removed;

    enum {
      idFontName = 100,
      idIconWidth,
      idFontDx,
      idFontDy,
      idDrawingAreaHorizontalMargin,
      idDrawingAreaVerticalMargin,

      idLabel1,
      idLabel2,
      idLabel3,

      idMoreUShorts = 1000,
      idMoreStrings = 2000,

      idOkButton = 3000,
      idApplyButton,
      idGetDefaultValueButton,
      idSetDefaultValueButton,
      idCancelButton
    };

    void		show_parameter(short, short) const;
    short		read_parameter(short) const;
    void		create_text_field(short, const char *);
    void                create_option_menu(short id, const char *name, const char *range);
    void		create_push_button(short, const char *, XtCallbackProc, int);

    static void		buttonCB(Widget, XtPointer , XtPointer);
};

#endif
