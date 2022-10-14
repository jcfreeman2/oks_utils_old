/**	
 *	\file oks/xm_context.h
 *	
 *	This file is part of the OKS package.
 *	Author: <Igor.Soloviev@cern.ch>
 *	
 *	This file contains the declarations for the OKS object.
 */

#ifndef OKS_XM_CONTEXT_H
#define OKS_XM_CONTEXT_H

#include <oks/file.h>
#include <oks/exceptions.h>

#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/xpm.h>
#include <Xm/Xm.h>

#include <fstream>
#include <list>
#include <string>

class OksTopDialog;

namespace oks {

    /**
     *  \brief Cannot load GUI resource file.
     *
     *  Such exception is thrown when OKS cannot load GUI resource file.
     */

  class FailedLoadGuiRcFile : public exception {

    public:

        /** The constructor gets reason from nested oks exception. **/
      FailedLoadGuiRcFile(const std::string& name, const exception& reason) noexcept : exception (fill(name, reason.what()), reason.level() + 1) { }

        /** The constructor gets reason from non-oks exception. **/
      FailedLoadGuiRcFile(const std::string& name, const std::string& reason) noexcept : exception (fill(name, reason), 0) { }

      virtual ~FailedLoadGuiRcFile() noexcept { }


    private:

      static std::string fill(const std::string& name, const std::string& reason);

  };

    /**
     *  \brief Cannot save GUI resource file.
     *
     *  Such exception is thrown when OKS cannot save GUI resource file.
     */

  class FailedSaveGuiRcFile : public exception {

    public:

        /** The constructor gets reason from nested oks exception. **/
      FailedSaveGuiRcFile(const std::string& name, const exception& reason) noexcept : exception (fill(name, reason.what()), reason.level() + 1) { }

        /** The constructor gets reason from non-oks exception. **/
      FailedSaveGuiRcFile(const std::string& name, const std::string& reason) noexcept : exception (fill(name, reason), 0) { }

      virtual ~FailedSaveGuiRcFile() noexcept { }


    private:

      static std::string fill(const std::string& name, const std::string& reason);

  };

}

  //
  // OksGC describes graphical context of the drawing area
  //

class OksGC {

  public:

      // define some drawing parameters which can be changed by user

    class Parameters {

      public:

        struct StringParameter {
          StringParameter(const std::string& s1, const std::string& s2) : p_name(s1), p_value(s2) {;}
	  StringParameter(const std::string& s1, const std::string& s2, const std::string& s3) : p_name(s1), p_value(s2), p_range(s3) {;}
	  std::string p_name;
	  std::string p_value;
          std::string p_range;
	};

        struct UShortParameter {
	  UShortParameter(const std::string& s, unsigned short i) : p_name(s), p_value(i) {;}
	  std::string p_name;
	  unsigned short p_value;
	};


          // constructors

        Parameters() {*this = def_params();}
        Parameters(const Parameters &p) {*this = p;}

        Parameters& operator=(const Parameters &);

        ~Parameters();


          // set methods

        void set_name(const std::string& s) {p_name = s;}

        void set_font_x_margin(unsigned short i) {p_font_x_margin = i;}
        void set_font_y_margin(unsigned short i) {p_font_y_margin = i;}
        void set_drawing_area_x_margin(unsigned short i) {p_drawing_area_x_margin = i;}
        void set_drawing_area_y_margin(unsigned short i) {p_drawing_area_y_margin = i;}
        void set_font_name(const std::string & s) {p_font_name = s;}

	void set_graphical_parameter(const std::string &, unsigned short);
	void set_graphical_parameter(const std::string &, const std::string &);                      // without range
        void set_graphical_parameter(const std::string &, const std::string &, const std::string &); // copy with range

	void set_generic_parameter(const std::string &, unsigned short);
	void set_generic_parameter(const std::string &, const std::string &);                      // without range
        void set_generic_parameter(const std::string &, const std::string &, const std::string &); // copy with range


          // get methods

        const std::string& get_name() const {return p_name;}

        unsigned short get_font_x_margin() const {return p_font_x_margin;}
        unsigned short get_font_y_margin() const {return p_font_y_margin;}
        unsigned short get_drawing_area_x_margin() const {return p_drawing_area_x_margin;}
        unsigned short get_drawing_area_y_margin() const {return p_drawing_area_y_margin;}
        const std::string& get_font_name() const {return p_font_name;}

        unsigned short get_u16_parameter(const std::string &) const;
	const std::string * get_string_parameter(const std::string &) const;

        const std::list<StringParameter *> & get_graphical_string_parameters() const {return p_graphical_string_parameters;}
        const std::list<UShortParameter *> & get_graphical_u16_parameters() const {return p_graphical_u16_parameters;}

        const std::list<StringParameter *> & get_generic_string_parameters() const {return p_generic_string_parameters;}
        const std::list<UShortParameter *> & get_generic_u16_parameters() const {return p_generic_u16_parameters;}


          // access default parameters

        static Parameters& def_params() {return default_parameters;}


          // initialise default parameters

        static int init_def_params();

        void save() const; // throw oks::FailedSaveGuiRcFile
        void read() ; // throw oks::FailedLoadGuiRcFile


      private:

        std::string p_name;

	unsigned short p_font_x_margin;
	unsigned short p_font_y_margin;
	unsigned short p_drawing_area_x_margin;
	unsigned short p_drawing_area_y_margin;
        std::string p_font_name;

	std::list<StringParameter *> p_graphical_string_parameters;
	std::list<UShortParameter *> p_graphical_u16_parameters;

	std::list<StringParameter *> p_generic_string_parameters;
	std::list<UShortParameter *> p_generic_u16_parameters;

        StringParameter * find_string_parameter(const std::string &) const;
        UShortParameter * find_u16_parameter(const std::string &) const;

        static Parameters default_parameters;

    };


      // class FileContext is used to replace OksGC when print PS file

    class FileContext {

      public:

        struct Line {
          short			x1, y1, x2, y2;

          Line			(short x_1, short y_1, short x_2, short y_2) : x1(x_1), y1(y_1), x2(x_2), y2(y_2) {;}
	  bool			operator==(const Line &l) {return (this == &l);}
        };

        struct Lines {
	  enum LColor {
	    DFColor,
	    BSColor,
	    TSColor
	  }			color;
	  std::list<Line *>	lines;
	  unsigned short        width;
	};

        struct Text {
          short			x, y;
	  std::string		text;
	
	  Text			(short x1, short y1, const char *s) : x(x1), y(y1), text(s) {;}
	  bool			operator==(const Text &t) {return (this == &t);}
        };

        struct Image {

	  enum FType {
	    PS_FType,
	    XWD_FType
	  };
	
          short			x, y, w, h;
	  std::string		p_image;
	  const FType		p_ftype;
	  void *		id;

	  Image			(short x1, short y1, short w1, short h1, FType ft, void * addr) :
                                   x(x1), y(y1), w(w1), h(h1), p_ftype(ft), id(addr) {p_image.reserve((w1+1)*h1);}
	  bool			operator==(const Image &i) {return (this == &i);}

	  std::string&		image() {return p_image;}
	  const std::string&	image() const {return p_image;}

          void			set_image(const XpmImage&, unsigned int);
	  static unsigned int	find_x_color(const XpmImage&, const XColor&);

        };

        enum Orientation {
          Portrait,
	  Landscape
        };

	enum CType {
	  PostScript,
	  MakerInterchangeFormat
	};

        FileContext	        () : file(0), p_scale_factor(1.0), p_header_font_name(0), p_footer_font_name(0) {;}

        bool			good() const {return (file != 0);}

        void			open(const char *, const OksGC&, CType);
        void			print_and_close();

	CType			get_ctype() const {return p_ctype;}

        Dimension&		image_width() {return p_image_width;}
        Dimension		image_width() const {return p_image_width;}

        Dimension&		image_height() {return p_image_height;}
        Dimension		image_height() const {return p_image_height;}

        Dimension&		paper_width() {return p_paper_width;}
        Dimension		paper_width() const {return p_paper_width;}

        Dimension&		paper_height() {return p_paper_height;}
        Dimension		paper_height() const {return p_paper_height;}

        Orientation&		orientation() {return p_orientation;}
        Orientation		orientation() const {return p_orientation;}

        double&			scale_factor() {return p_scale_factor;}
        double			scale_factor() const {return p_scale_factor;}

        Dimension&		left_margin() {return p_left_margin;}
        Dimension		left_margin() const {return p_left_margin;}

        Dimension&		right_margin() {return p_right_margin;}
        Dimension		right_margin() const {return p_right_margin;}

        Dimension&		top_margin() {return p_top_margin;}
        Dimension		top_margin() const {return p_top_margin;}

        Dimension&		bottom_margin() {return p_bottom_margin;}
        Dimension		bottom_margin() const {return p_bottom_margin;}

        int&			white_background() {return p_white_background;}
        int			white_background() const {return p_white_background;}
        const XColor&		white_background_color() const {return white_bg_color;}

        int&			print_image_box() {return p_print_image_box;}
        int			print_image_box() const {return p_print_image_box;}

        std::list<Text *>&	texts() {return p_texts;}
        std::list<Image *>&	images() {return p_images;}
        std::list<Lines *>&	lines() {return p_lines;}

        std::list<std::string *>& header() {return p_header;}
        const std::list<std::string *>& header() const {return p_header;}

        std::list<std::string *>& footer() {return p_footer;}
        const std::list<std::string *>& footer() const {return p_footer;}

        const char *&		header_font_name() {return p_header_font_name;}
        const char *		header_font_name() const {return p_header_font_name;}

        const char *&		footer_font_name() {return p_footer_font_name;}
        const char *		footer_font_name() const {return p_footer_font_name;}

        unsigned int&		header_font_size() {return p_header_font_size;}
        unsigned int		header_font_size() const {return p_header_font_size;}
  
        unsigned int&		footer_font_size() {return p_footer_font_size;}
        unsigned int		footer_font_size() const {return p_footer_font_size;}

        unsigned int		header_size() const;
        unsigned int		footer_size() const; 


     private:

        std::string		p_file_name;
        std::ofstream *		file;
	CType			p_ctype;

        Dimension		p_image_width;
        Dimension		p_image_height;

        Dimension		p_paper_width;
        Dimension		p_paper_height;

        Orientation		p_orientation;

        double			p_scale_factor;

        Dimension		p_left_margin;
        Dimension		p_right_margin;
        Dimension		p_top_margin;
        Dimension		p_bottom_margin;
      
        int			p_white_background;
        XColor			white_bg_color;
      
        int			p_print_image_box;

        XColor			fg_color;	// foreground color
        XColor			bg_color;	// background color
        XColor			ts_color;	// top shadow color
        XColor			bs_color;	// bottom shadow color

        char *			font_name;
        unsigned short		font_size;

        std::list<std::string *> p_header;
        std::list<std::string *> p_footer;

        const char *		p_header_font_name;
        const char *		p_footer_font_name;

        unsigned int		p_header_font_size;
        unsigned int		p_footer_font_size;

        std::list<Text *>	p_texts;
        std::list<Image *>	p_images;
	std::list<Lines *>      p_lines;

        double	 		x(short) const;
        double	 		y(short) const;
        double			size(short) const;

        void			set_color(XColor *);
        void			declare_color(const char *, const XColor *);

        void			print_lines(std::list<Line *>&, short, XColor *);

        void			print_text(const char *, double, double, const char * = 0, double = 0.0, const char * = 0) const;
    };

    friend class FileContext;
  
  
    enum ShadowThickness {
      shadow_thickness = 1
    };
  
    enum HighlightThickness {
      highlight_thickness = 1
    };
  
    enum ShadowType {
      shadow_in,
      shadow_out,
      etched_in,
      etched_out
    };

    OksGC		() : w(0), fc(0), params(Parameters::def_params()), parameters_dlg(0), print_dlg(0) {;}
    ~OksGC		();

    void		init(Widget);
    void		reset();

    const Parameters&   get_params() const {return params;}
    Parameters&         get_params() {return params;}

    Pixel		get_color(const char *) const;
    XColor		get_color(Pixel) const;

    static const char *	get_bottom_shadow_color() {return bs_oks_xm;}
    static const char *	get_top_shadow_color() {return ts_oks_xm;}

    Display *		get_display() const {return display;}
    Drawable		get_xt_window() const {return drawable;}
    Widget		get_widget() const {return w;}
    GC			get_gc() const {return gc;}
    XFontStruct * 	get_font() const {return font;}

    FileContext *	create_fc() {fc = new FileContext(); return fc;}
    FileContext *	get_fc() {return fc;}
    void		destroy_fc() {delete fc; fc = 0;}

      // cursor functions

    void		define_cursor(unsigned int) const;
    void		undefine_cursor() const;


      // rectangle functions

    void		clean_rectangle(Dimension, Dimension, Dimension, Dimension);
    void		draw_rectangle(Dimension, Dimension, Dimension, Dimension, const char * = 0, unsigned short = 1, bool = false);
    void		draw_stippled_rectangle(Dimension, Dimension, Dimension, Dimension, const char * = 0);
    void		draw_shadowed_rectangle(Dimension, Dimension, Dimension, Dimension, ShadowType = shadow_in, unsigned short = 1);


      // line functions

    void		draw_line(Dimension, Dimension, Dimension, Dimension, const char * = 0, unsigned short = 1);
    void		draw_stippled_line(Dimension, Dimension, Dimension, Dimension, const char * = 0);
    void		draw_shadowed_line(Dimension, Dimension, Dimension, Dimension, ShadowType = shadow_in, unsigned short = 1);
    void		draw_selection_line(Dimension, Dimension, Dimension, Dimension, bool);


      // string and text functions

    Dimension		get_font_height() const;
    int			get_string_width(const char *) const;
    std::string		wrap_text(const std::string&, Dimension&, Dimension&) const;
    void		draw_string(Dimension, Dimension, const char *, const char * = 0);
    void		draw_text(const std::string&, Dimension, Dimension, Dimension, const char * = 0);


      // pixmap functions

    void		draw_pixmap(Pixmap, Dimension, Dimension, Dimension, Dimension);


      // create parameters dialog

    typedef void        (*ApplyFN)(XtPointer);

    void		create_parameters_dialog(OksTopDialog *, const std::string&, ApplyFN, XtPointer);
    void		remove_parameters_dialog() {parameters_dlg = 0;}

    void		create_print_dialog(OksTopDialog *, const std::string&, ApplyFN, XtPointer, const OksFile::Map&);
    void		remove_print_dialog() {print_dlg = 0;}


  private:

    Widget		w;
    Display *		display;
    Drawable		drawable;

    XFontStruct * 	font;
    Dimension		font_height;

    FileContext *       fc;

    GC			gc;
    GC			gc2;

    const char *        p_gc2_color;
    unsigned short      p_gc2_line_width;
    bool		p_gc2_fill;

    GC			ts_gc;
    GC			bs_gc;

    unsigned short      shadow_lines_width;

    GC			stipled_gc;
    GC			selection_gc;

    Parameters	        params;
    Widget		parameters_dlg;
    Widget		print_dlg;

    GC			get_gc(const char *, unsigned short = 1, bool = false);

    static const char * bs_oks_xm;
    static const char * ts_oks_xm;

};

std::ostream& operator<<(std::ostream&, const OksGC::Parameters&);

#endif
