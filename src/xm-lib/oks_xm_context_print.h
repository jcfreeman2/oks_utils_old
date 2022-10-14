#ifndef __OKS_GC_PRINT_DIALOG_H
#define __OKS_GC_PRINT_DIALOG_H

#include <oks/xm_dialog.h>
#include <oks/xm_context.h>

class OksGCPrint : public OksDialog
{
  public:

    OksGCPrint	(OksTopDialog * top, OksGC *, const std::string &, OksGC::ApplyFN, XtPointer, const OksFile::Map &);
    virtual	~OksGCPrint	();
    
    Widget	get_dialog() const {return dlg_w;}

    struct Format {
      const char *   name;
      unsigned short width;
      unsigned short height;
    };

  private:

    OksGC *		gc;
    Widget		dlg_w;
    OksGC::ApplyFN	apply_f;
    XtPointer		apply_f_p;
    const OksFile::Map& p_files;
  
    Widget		w_print_to_file;
    Widget		w_send_to_printer;
  
    Widget		w_file_name;
    Widget		w_print_command;
  
    Widget		w_header_text;
    Widget		w_footer_text;

    static Format	formats[];
    static const char *	user_defined_format;
    static const char *	as_is_format;
  
    static const char * title_font_names[];
    static const char * title_font_sizes[];

    static const char * portrait_orientation;
    static const char * landscape_orientation;
    static const char * best_fit_orientation;
  
    static const char *	no_title;
    static const char *	header_title;
    static const char *	footer_title;


    enum {
      idDestination = 100,
      idPrintToFile,
      idFileFormat,
      idSendToPrinter,
      idFileName,
      idPrintCommand,
      idFormatLabel,
      idFormat,
      idUserDefinedWidth,
      idUserDefinedHeight,
      idOrientation,
      idImageParametersLabel,
      idScaleToFit,
      idWhiteBackground,
      idPrintImageBox,
      idHeaderText,
      idFooterText,
      idPrintDate,
      idPrintFileList,
      idMarginsLabel,
      idLeftMargin,
      idRightMargin,
      idTopMargin,
      idBottomMargin,
      idHeaderFontNames,
      idHeaderFontSizes,
      idFooterFontNames,
      idFooterFontSizes,
      idPrintButton,
      idCancelButton
    };

    static void		buttonCB(Widget, XtPointer , XtPointer);
    static void		switchOutputCB(Widget, XtPointer , XtPointer);
    static void		chooseFormatCB(Widget, XtPointer , XtPointer);
    static void		chooseFileFormatCB(Widget, XtPointer , XtPointer);

    static const char *	get_tmp_output_filename();

    const char *	get_format(unsigned short &, unsigned short &) const;
    void		get_image_size(unsigned short &, unsigned short &) const;

    Widget		create_text_field(short, const char *, const char *, short);
    void		set_text_field(short, short) const;
    short		read_text_field(short) const;

    void		enable_text_field(Widget, int) const;
    void		enable_format_size_boxes(int) const;
    void		use_as_is_settings(int) const;

    double		scale_factor(short, short) const;

    Widget		create_option_menu(short, const char *, const char **);

};

#endif
