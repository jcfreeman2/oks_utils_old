#include <stdlib.h>	// atoi() prototype
#include <X11/XWDFile.h>

#include <sstream>
#include <fstream>

#include <oks/defs.h>
#include <oks/xm_context.h>


void
OksGC::FileContext::open(const char * file_name, const OksGC& gc, CType ct)
{
  p_file_name = file_name;
  p_ctype = ct;

  file = new std::ofstream(file_name);

  if(!file) {
    Oks::error_msg("OksGC::FileContext::open()")
      << "Can't open file \"" << file_name << "\"\n";

    return;
  }

  {
    XGCValues values;

    XGetGCValues(gc.display, gc.gc, GCForeground, &values);
    fg_color = gc.get_color((Pixel)values.foreground);

    XGetGCValues(gc.display, gc.ts_gc, GCForeground, &values);
    ts_color = gc.get_color((Pixel)values.foreground);

    XGetGCValues(gc.display, gc.bs_gc, GCForeground, &values);
    bs_color = gc.get_color((Pixel)values.foreground);

    Pixel pixel;
    XtVaGetValues(XtParent(gc.w), XmNbackground, &pixel, NULL);
    bg_color = gc.get_color(pixel);
    if(white_background()) {
      white_bg_color = bg_color;
      white_bg_color.red   /= 256;
      white_bg_color.green /= 256;
      white_bg_color.blue  /= 256;
    }
  }


    //
    // setup font properties
    //
    // suppose that the following PS fonts are available:
    //  - Courier, Courier-Bold, Courier-BoldOblique, Courier-Oblique,
    //  - Helvetica, Helvetica-Bold, Helvetica-BoldOblique, Helvetica-Narrow,
    //    Helvetica-Narrow-Bold, Helvetica-Narrow-BoldOblique,
    //    Helvetica-Narrow-Oblique, Helvetica-Oblique,
    //  - Symbol,
    //  - Times-Bold, Times-BoldItalic, Times-Italic, Times-Roman
    //  - NewCenturySchlbk-Bold, NewCenturySchlbk-BoldItalic,
    //    NewCenturySchlbk-Italic, NewCenturySchlbk-Roman
    //

  {
      // detect X11 font name

    const char * x_font_name = gc.get_params().get_font_name().c_str();

      // convert X11 font name into PostScript font name

    const char *font_family = (
      strstr(x_font_name, "courier")                ? "Courier" :
      strstr(x_font_name, "helvetica")              ? "Helvetica" :
      strstr(x_font_name, "symbol")                 ? "Symbol" :
      strstr(x_font_name, "times")                  ? "Times" :
      strstr(x_font_name, "new century schoolbook") ? "NewCenturySchlbk" :
      0
    );


      // there can be some unknown font family or something like AxB

    if(!font_family) {
      font_name = new char [strlen(x_font_name) + 1];
      strcpy(font_name, x_font_name);
    }

      // setup more font properties

    else {
      const char *bold = (strstr(x_font_name, "-bold-") ? "Bold" : 0);
      const char *oblique = (strstr(x_font_name, "-o-") ? "Oblique" : 0);
      const char *italic = (strstr(x_font_name, "-i-") ? "Italic" : 0);
      const char *narrow = (strstr(x_font_name, "-r-") ? "Narrow" : 0);

      unsigned short ps_font_name_len = (
        strlen(font_family) +
        (bold ? strlen(bold) + 1 : 0) +
        (oblique ? strlen(oblique) + 1 : 0) +
        (italic ? strlen(italic) + 1 : 0) +
        (narrow ? strlen(narrow) + 1 : 0) +
        1
      );

      font_name = new char [ps_font_name_len];
      strcpy(font_name, font_family);
    
      if(narrow) {
        strcat(font_name, "-");
        strcat(font_name, narrow);
      }
    
      if(bold) {
        strcat(font_name, "-");
        strcat(font_name, bold);
      }

      if(oblique) {
        if(!bold)
          strcat(font_name, "-");

        strcat(font_name, oblique);
      }

      if(italic) {
        strcat(font_name, "-");
        strcat(font_name, italic);
      }
    }

      // detect font size

    const char *font_size_ptr_begin = strstr(x_font_name, "--");
  
    if(font_size_ptr_begin) {
      font_size_ptr_begin += 2;
      const char *font_size_ptr_end = strstr(font_size_ptr_begin, "-");
    
      if(font_size_ptr_end) {
        char font_size_buf[16];
	char *fs_ptr = font_size_buf;

        while(font_size_ptr_begin != font_size_ptr_end)
          *(fs_ptr++) = *(font_size_ptr_begin++);

        *fs_ptr = '\0';
	
	font_size = atoi(font_size_buf);
      }
    }
  }
}

void
OksGC::FileContext::set_color(XColor *color)
{
  if(color) {
    if(p_ctype == PostScript) {
      *file
        << (double)color->red   / (double)65535.0 << ' '
        << (double)color->green / (double)65535.0 << ' '
        << (double)color->blue  / (double)65535.0 << " setrgbcolor\n";
    
    }
    else {
      *file
        << "<ObColor `"
	<< (
	     color == &bg_color ? "Background" :
	     color == &ts_color ? "TopShadow" :
	     color == &bs_color ? "BottomShadow" :
	     "Foreground"
	   )
	<< "\'>\n";
    }
  }
}

void
OksGC::FileContext::declare_color(const char * name, const XColor * color)
{
  if(p_ctype == MakerInterchangeFormat) {
    *file
      << " <Color\n"
         "  <ColorTag       `" << name << "\'>\n"
         "  <ColorCyan      " << (100.0 - (double)color->red   / (double)655.35) << ">\n"
         "  <ColorMagenta   " << (100.0 - (double)color->green / (double)655.35) << ">\n"
         "  <ColorYellow    " << (100.0 - (double)color->blue  / (double)655.35) << ">\n"
         "  <ColorBlack     0.00>\n"
         " > # end of Color\n";
  }
}

void
OksGC::FileContext::print_lines(std::list<Line *>& lines, short line_width, XColor *color)
{
  if(!lines.empty()) {
    if(p_ctype == PostScript) {
      *file << "gsave\n" << size(line_width) << " setlinewidth\n";
    }
    else {
      *file << "<Pen 0>\n"
               "<PenWidth " << size(line_width) << ">\n";
    }

    set_color(color);

    while(!lines.empty()) {
      Line *line = lines.front();
      lines.pop_front();

      if(p_ctype == PostScript) {
        *file << ' '
              << x(line->x1) << ' ' << y(line->y1) << " moveto "
              << x(line->x2) << ' ' << y(line->y2) << " lineto stroke\n";
      }
      else {
        *file << "<PolyLine <GroupID 1> <NumPoints 2> <Point "
	      << x(line->x1) << "\" " << y(line->y1)
	      << "\"> <Point " << x(line->x2) << "\" " << y(line->y2) << "\"> >\n";
      }

      delete line;
    }

    if(p_ctype == PostScript) {
      *file << "grestore\n";
    }
  }
}


void
OksGC::FileContext::print_and_close()
{
  if(!file) {
    Oks::error_msg("OksGC::print_and_close()") << "No open file\n";

    return;
  }

  file->setf(std::ios::fixed, std::ios::floatfield);
  file->precision(2);


    // calculate bounding box parameters
    // (they must not be negative)

  short bb_x1 = (short)x(0);
  short bb_x2 = (short)x(image_width());
  short bb_y1 = (short)y(image_height());
  short bb_y2 = (short)y(0);
  
  if(bb_x1 < 0) bb_x1 = 0;
  if(bb_x2 < 0) bb_x2 = 0;
  if(bb_y1 < 0) bb_y1 = 0;
  if(bb_y2 < 0) bb_y2 = 0;
  
  
    // calculate used fonts

  std::string fonts(font_name);

  if(p_header_font_name && strcmp(font_name, p_header_font_name)) {
    fonts += ' ';
    fonts += p_header_font_name;
  }

  if(p_footer_font_name && strcmp(font_name, p_footer_font_name) && (!p_header_font_name || strcmp(p_header_font_name, p_footer_font_name))) {
    fonts += ' ';
    fonts += p_footer_font_name;
  }


  if(p_ctype == PostScript) {
    *file << "%!PS-Adobe-2.0 EPSF-2.0\n"
             "%%Creator: OKS Library, <Igor.Soloviev@cern.ch>, CERN\n"
             "%%Title: unknown\n"
	     "%%BoundingBox: " << bb_x1 << ' ' << bb_y1 << ' ' << bb_x2 << ' ' << bb_y2 << "\n"
             "%%Pages: 1\n"
             "%%DocumentFonts: " << fonts << "\n"
             "%%EndComments\n\n";

    if(!images().empty()) {
      *file <<
	     "% define space for color conversions\n"
	     "/grays 266 string def  % space for gray scale line\n"
	     "/npixls 0 def\n"
	     "/rgbindx 0 def\n\n"
	     "% define 'colorimage' if it isn't defined\n"
	     "%   ('colortogray' and 'mergeprocs' come from xwd2ps via xgrab)\n"
	     "/colorimage where   % do we know about 'colorimage'?\n"
	     "  { pop }           % yes: pop off the 'dict' returned\n"
	     "  {                 % no:  define one\n"
	     "    /colortogray {  % define an RGB->I function\n"
	     "      /rgbdata exch store    % call input 'rgbdata'\n"
	     "      rgbdata length 3 idiv\n"
	     "      /npixls exch store\n"
	     "      /rgbindx 0 store\n"
	     "      0 1 npixls 1 sub {\n"
	     "        grays exch\n"
	     "        rgbdata rgbindx       get 20 mul    % Red\n"
	     "        rgbdata rgbindx 1 add get 32 mul    % Green\n"
	     "        rgbdata rgbindx 2 add get 12 mul    % Blue\n"
	     "        add add 64 idiv      % I = .5G + .31R + .18B\n"
	     "        put\n"
	     "        /rgbindx rgbindx 3 add store\n"
	     "      } for\n"
	     "      grays 0 npixls getinterval\n"
	     "    } bind def\n\n"
	     "    % Utility procedure for colorimage operator.\n"
	     "    % This procedure takes two procedures off the\n"
	     "    % stack and merges them into a single procedure.\n\n"
	     "    /mergeprocs { % def\n"
	     "      dup length\n"
	     "      3 -1 roll\n"
	     "      dup\n"
	     "      length\n"
	     "      dup\n"
	     "      5 1 roll\n"
	     "      3 -1 roll\n"
	     "      add\n"
	     "      array cvx\n"
	     "      dup\n"
	     "      3 -1 roll\n"
	     "      0 exch\n"
	     "      putinterval\n"
	     "      dup\n"
	     "      4 2 roll\n"
	     "      putinterval\n"
	     "    } bind def\n\n"
	     "    /colorimage { % def\n"
	     "      pop pop     % remove 'false 3' operands\n"
	     "      {colortogray} mergeprocs\n"
	     "      image\n"
	     "    } bind def\n"
	     "  } ifelse          % end of 'false' case\n\n\n";
    }

    *file << "% set image font\n"
             "/" << font_name << " findfont\n"
          << size((short)font_size) << " scalefont\n"
	     "setfont\n\n";


    if(orientation() == Landscape)
      *file << "% draw in landscape mode\n"
               "90 rotate  0 -" << paper_height() << " translate\n\n";
  }
  else {
    *file << "<MIFFile 6.00> # Generated by OKS Library, <Igor.Soloviev@cern.ch>, CERN EP/ATD\n";

      // define document colors

    *file << "<ColorCatalog\n";

    declare_color("Foreground", &fg_color);

    if(white_background()) {
      XColor wcolor;
      wcolor.red = wcolor.green = wcolor.blue = 65535;
      declare_color("Background", &wcolor);
    }
    else {
      declare_color("Background", &bg_color);
    }

    declare_color("TopShadow", &ts_color);
    declare_color("BottomShadow", &bs_color);

    *file << "> # end of ColorCatalog\n";


      // define document size

    {
      double dh = (double)paper_height() / 72.0;
      double dw = (double)paper_width()  / 72.0;

      if(orientation() == Landscape) {
        double dd = dh;
	dh = dw;
	dw = dd;
      }

      *file << "<Document\n"
               " <DPageSize " << dh << "\" " << dw << "\">\n"
               ">\n";
    }
  }

  if(!white_background()) {
    if(p_ctype == PostScript) {
      *file << "% draw outline rectangle\ngsave\n";
    }
    else {
      *file << "# draw outline rectangle\n<Fill 0>\n";
    }

    set_color(&bg_color);

    if(p_ctype == PostScript) {
      *file << ' ' << x(0) << ' ' << y(p_image_height) << ' '
            << size(p_image_width) << ' ' << size(p_image_height) << " rectfill\ngrestore\n\n";
    }
    else {
      double x1 = x(0), y1 = y(0);
      *file << "<Rectangle <GroupID 1>\n <ShapeRect " << x1 << "\" " << y1 << "\" "
	    << (x(p_image_width) - x1) << "\" " << (y(p_image_height) - y1) << "\">\n>\n";
    }
  }

  if(print_image_box()) {
    std::list<Line *> box_lines;

    box_lines.push_back(new Line(0, 0, 0, p_image_height));
    box_lines.push_back(new Line(0, p_image_height, p_image_width, p_image_height));
    box_lines.push_back(new Line(p_image_width, p_image_height, p_image_width, 0));
    box_lines.push_back(new Line(0, 0, p_image_width, 0));

    if(p_ctype == PostScript) {
      *file << "% draw image box\n";
    }
    else {
      *file << "# draw image box\n";
    }

    print_lines(box_lines, 2, &fg_color);
  }


    // draw images

  std::list<void *> image_files;

  while(!images().empty()) {
    Image *image = images().front();
    images().pop_front();

    if(p_ctype == PostScript) {
      *file << "gsave\n/picstr " << 3 * image->w << " string def\n"
            << x(image->x) << ' ' << y(image->y + image->h) << " translate\n"
            << size(image->w) << ' ' << size(image->h) << " scale\n"
            << image->w << ' ' << image->h << ' ' << "8 % dimensions of data\n"
               "[" << image->w << " 0 0 -" << image->h << " 0 " << image->h << "]\n"
               "{ currentfile picstr readhexstring pop }\n"
               "false 3 colorimage\n"
	    << image->image()
	    << "grestore\n";
    }
    else {
      unsigned long id = 0;
      unsigned long count = 1;
      bool is_new_file = true;
      for(std::list<void *>::iterator j = image_files.begin(); j != image_files.end(); ++j) {
	if(*j == image->id) {
	  id = count;
	  is_new_file = false;
	  break;
	}
        count++;
      }

      if(id == 0) {
        id = count;
	image_files.push_back(image->id);
      }

      std::string new_file_name(p_file_name);

      std::string::size_type idx = new_file_name.find_last_of('.');
      if(idx != std::string::npos) {
        new_file_name.erase(idx);
      }

      {
        std::ostringstream s;
        s << new_file_name << '-' << id << ".xwd" << std::ends;
        new_file_name = s.str();
      }


      if(is_new_file) {
        std::cout << " - convert pixmap into file \'" << new_file_name << "\'\n";

        std::ofstream f2(new_file_name.c_str());
	
	if(f2) {
	  for(size_t j = 0; j < image->image().size() && f2.good(); ++j) f2.put(image->image()[j]);
	}
      }

      double x1 = x(image->x), y1 = y(image->y);

      *file << "<ImportObject\n"
               " <GroupID 1>\n"
               " <ObColor `Background\'>\n"
               " <RunaroundType Contour>\n"
               " <ImportObFile `" << new_file_name << "\'>\n"
               " <ShapeRect  " << x1 << "\" " << y1 << "\" " << (size(image->w) / 72.0) << "\" " << (size(image->h) / 72.0) << "\">\n"
               " <BitMapDpi " << (72.0 / p_scale_factor) << ">\n"
               " <FlipLR No>\n"
               "> # end of ImportObject\n";
    }

    delete image;
  }


    // draw lines

  while(!lines().empty()) {
    Lines * l = lines().front();
    lines().pop_front();

    print_lines(
      l->lines,
      l->width,
      (l->color == Lines::DFColor ? &fg_color : l->color == Lines::BSColor ? &bs_color : &ts_color)
    );

    delete l;
  }


    // draw text strings

  bool is_first_text = true;

  while(!texts().empty()) {
    Text *text = texts().front();
    texts().pop_front();

    const char * fn = 0;
    double fs = 0;

    if(is_first_text) {
      is_first_text = false;

      if(p_ctype == MakerInterchangeFormat) {
        fn = font_name;
        fs = (double)font_size * scale_factor();
      }
    }

    print_text(text->text.c_str(), x(text->x), y(text->y), fn, fs, 0);

    delete text;
  }

     // calculate positions of header and footer

  long header_pos = (
    (p_ctype == PostScript)
      ? paper_height() - top_margin() - header_font_size()
      : top_margin() + header_font_size()
  );

  long footer_pos = (
    (p_ctype == PostScript)
      ? (unsigned int)y(p_image_height) - 2 * footer_font_size()
      : (long)(y(p_image_height) * 72.0) + 2 * header_font_size()
  );


    // draw header

  is_first_text = true;

  if(!header().empty()) {
    while(!header().empty()) {
      std::string *s = header().front();
      header().pop_front();

      const char * fn = 0;
      double fs = 0;
      const char * fc = 0;

      if(is_first_text) {
        is_first_text = false;

        fn = header_font_name();
        fs = header_font_size();
        fc = "set header font";
      }

      if(p_ctype == PostScript)
        print_text(s->c_str(), left_margin(), header_pos, fn, fs, fc);
      else
        print_text(s->c_str(), x(0), (double)header_pos / 72.0, fn, fs, fc);

      delete s;

      unsigned long dh = (3 * header_font_size()) / 2;
      if(p_ctype == PostScript) header_pos -= dh;
      else header_pos += dh;
    }
  }


    // draw footer

  is_first_text = true;

  if(!footer().empty()) {
    while(!footer().empty()) {
      std::string *s = footer().front();
      footer().pop_front();

      const char * fn = 0;
      double fs = 0;
      const char * fc = 0;

      if(is_first_text) {
        is_first_text = false;

        fn = footer_font_name();
        fs = footer_font_size();
        fc = "set footer font";
      }

      if(p_ctype == PostScript)
        print_text(s->c_str(), left_margin(), footer_pos, fn, fs, fc);
      else
        print_text(s->c_str(), x(0), (double)footer_pos / 72.0, fn, fs, fc);

      delete s;

      unsigned long dh = (3 * header_font_size()) / 2;
      if(p_ctype == PostScript) footer_pos -= dh;
      else footer_pos += dh;
    }
  }

  if(p_ctype == PostScript) {
    *file << "showpage\n";
  }
  else {
    *file << "\n<Group <ID 1>\n";
  }

  delete file;
  file = 0;

  delete [] font_name;
}


void
OksGC::FileContext::print_text(const char *s, double x1, double y1, const char * f_name, double f_size, const char * comment) const
{
  if(comment) {
    if(p_ctype == PostScript) {
      *file << "\n% ";
    }
    else {
      *file << "\n# ";
    }

    *file << comment << std::endl;
  }

  if(f_name) {
    if(p_ctype == PostScript) {
      *file << '/' << f_name << " findfont\n" << (unsigned short)f_size << " scalefont\nsetfont\n\n";
    }
    else {
      *file << "<TextLine <GroupID 1>\n"
            << " <Font <FFamily `" << f_name << "\'> <FSize " << f_size << "> <FPlain Yes>>\n";
    }
  }
  
  if(p_ctype == PostScript) {
    *file << ' ' << x1 << ' ' << y1 << " moveto (";
  }
  else {
    if(!f_name) {
      *file << "<TextLine <GroupID 1>\n";
    }
    
    *file << " <TLOrigin " << x1 << "\" " << y1 << "\"> <TLAlignment Left> <String `";
  }

  while(*s != '\0') {
    if(p_ctype == PostScript) {
      if(*s == '(' || *s == ')') *file << '\\';
      *file << *(s++);
    }
    else {
      if(*s == '\\') *file << '\\' << '\\';
      else if(*s == '\"') *file << '\\' << "xd2 ";
      else if(*s == '`') *file << '\\' << "xd4 ";
      else if(*s == '\'') *file << '\\' << "xd5 ";
      else if(*s == '>') *file << '\\' << '>';
      else *file << *s;

      s++;
    }
  }

  if(p_ctype == PostScript) {
    *file << ") show\n";
  }
  else {
    *file << "\'>\n>\n";
  }
}


unsigned int
OksGC::FileContext::Image::find_x_color(const XpmImage& xpm_image, const XColor& xcolor)
{
  unsigned int num = 0;

  while(num < xpm_image.ncolors) {
    const char *color = xpm_image.colorTable[num].c_color;
    char bufR[3] = {color[1], color[2], '\0'};
    char bufG[3] = {color[5], color[6], '\0'};
    char bufB[3] = {color[9], color[10], '\0'};

    if(
      strtol(bufR, 0, 16) == xcolor.red   &&
      strtol(bufG, 0, 16) == xcolor.green &&
      strtol(bufB, 0, 16) == xcolor.blue
    ) break;

    num++;
  }

  return num;
}

static void
_swaplong (char *bp, unsigned n)
{
  char c;
  char *ep = bp + n;

  while (bp < ep) {
    c = bp[3];
    bp[3] = bp[0];
    bp[0] = c;
    c = bp[2];
    bp[2] = bp[1];
    bp[1] = c;
    bp += 4;
  }
}

void
OksGC::FileContext::Image::set_image(const XpmImage& xpm_image, unsigned int white_num)
{
  if(p_ftype == PS_FType) {
    for(unsigned int y1 = 0; y1 < xpm_image.height; y1++) {
      for(unsigned int x1 = 0; x1 < xpm_image.width; x1++) {
        unsigned int num = xpm_image.data[x1 + y1*xpm_image.width];

        if(num != white_num) {
          const char * color = (xpm_image.colorTable[num]).c_color;
          char buf[7] = {color[1], color[2], color[5], color[6], color[9], color[10], '\0'};
          image().append(buf);
        }
        else
          image().append("FFFFFF");
      }

      image().append("\n");
    }
  }
  else {
    std::ostringstream s;

    const char win_name[] = "OKS";
    size_t win_name_size = strlen(win_name) + sizeof(char);

    XWDFileHeader header;

    header.header_size =      (CARD32) (sizeof(XWDFileHeader) + win_name_size);
    header.file_version =     (CARD32) XWD_FILE_VERSION;
    header.pixmap_format =    (CARD32) ZPixmap;
    header.pixmap_depth =     (CARD32) 24;
    header.pixmap_width =     (CARD32) xpm_image.width;
    header.pixmap_height =    (CARD32) xpm_image.height;
    header.xoffset =          (CARD32) 0;
    header.byte_order =       (CARD32) MSBFirst;
    header.bitmap_unit =      (CARD32) 32;
    header.bitmap_bit_order = (CARD32) MSBFirst;
    header.bitmap_pad =       (CARD32) 32;
    header.bits_per_pixel =   (CARD32) 32;
    header.bytes_per_line =   xpm_image.width * 4;
    header.visual_class =     (CARD32) DirectColor;
    header.ncolors =          (CARD32) 0;
    header.red_mask =         (CARD32) 0xff0000;
    header.green_mask =       (CARD32) 0xff00;
    header.blue_mask =        (CARD32) 0xff;
    header.bits_per_rgb =     header.pixmap_depth;
    header.colormap_entries = (CARD32) 256;
    header.window_width =     (CARD32) xpm_image.width;
    header.window_height =    (CARD32) xpm_image.height;
    header.window_x =         (CARD32) 0;
    header.window_y =         (CARD32) 0;
    header.window_bdrwidth =  (CARD32) 0;


    unsigned long swaptest = 1;

    if(*(char *) &swaptest) {
      _swaplong((char *) &header, sizeof(header));
    }

    {
      const char * buf = (char *)&header;
      size_t i = 0; while(i < sizeof(XWDFileHeader)) s.put(buf[i++]);
      i=0; while(i < win_name_size) s.put(win_name[i++]);
    }

    for(unsigned int y1 = 0; y1 < xpm_image.height; y1++) {
      for(unsigned int x1 = 0; x1 < xpm_image.width; x1++) {
        unsigned int num = xpm_image.data[x1 + y1*xpm_image.width];

        if(num != white_num) {
          const char * color = (xpm_image.colorTable[num]).c_color;
	
          char bufR[3] = {color[1], color[2], '\0'};
          char bufG[3] = {color[5], color[6], '\0'};
          char bufB[3] = {color[9], color[10], '\0'};

	  s.put((unsigned char)0);
	  s.put((unsigned char)(strtol(bufR, 0, 16)));
	  s.put((unsigned char)(strtol(bufG, 0, 16)));
	  s.put((unsigned char)(strtol(bufB, 0, 16)));
        }
	else {
	  s.put((unsigned char)0);
	  s.put((unsigned char)255);
	  s.put((unsigned char)255);
	  s.put((unsigned char)255);
	}
      }
    }

    s << std::ends;

    std::string cs = s.str();
    p_image += cs;

//    int j = 0; while(j < cs.length()) p_image += cs[j++];
  }
}


double
OksGC::FileContext::x(short p_x) const
{
  double x1 = (scale_factor() * p_x + (double)left_margin());
  if(p_ctype == MakerInterchangeFormat) x1 /= 72.0;
  return x1;
}


double
OksGC::FileContext::y(short p_y) const
{
  if(p_ctype == MakerInterchangeFormat) {
    return ((scale_factor() * p_y + (double)top_margin() + (double)header_size()) / 72.0);
  }
  else {
    return ((double)paper_height() - scale_factor() * p_y - (double)top_margin() - (double)header_size());
  }
}


double
OksGC::FileContext::size(short p_w) const
{
  return (scale_factor() * p_w);
}


unsigned int
OksGC::FileContext::header_size() const
{
  return (
    header().empty()
      ? 0
      : ((header().size() * 3 + 1) * header_font_size() / 2)
  );
}


unsigned int
OksGC::FileContext::footer_size() const
{
  return (
    footer().empty()
      ? 0
      : ((footer().size() * 3 + 1) * footer_font_size() / 2)
  );
}
