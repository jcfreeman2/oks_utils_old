#define _OksXmBuildDll_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _MSC_VER
#  include <unistd.h>
#  include <netinet/in.h>
#  include <dirent.h>
#else
#  include <direct.h>
#endif

#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <oks/xm_help_window.h>
#include <oks/xm_utils.h>
#include <oks/xm_popup.h>

#include <Xm/Xm.h>
#include <Xm/Text.h>
#include <Xm/SelectioB.h>

extern "C" {
#include <Xm/XmHTML.h>
#include <Xm/HTTP.h>
}

#include <fstream>


std::string
OksXmHelpWindow::set_absolute_url(const char * url, bool change_previous)
{
  static std::string previous_url;
  std::string URL(url);

#ifdef DEBUG
  std::cout << "set_absolute_url(" << url << ")\n";
#endif

  std::string::size_type q_index = URL.find('?');

  if(q_index != std::string::npos) {
    std::string protocol = URL.substr(++q_index, 5);

    if(protocol == "http:") URL.erase(0, q_index);


#ifdef DEBUG
    std::cout << "set URL to \'" << URL << "\'\n";
#endif
  }


  if(URL.length() > 6) {
    std::string protocol = URL.substr(0, 5);

    if(protocol == "file:" || protocol == "http:") {
      if(change_previous) {
        URL.erase(0, 5);

        previous_url = protocol + URL;

        return previous_url;
      }
      else
        return URL;
    }
    else if(!previous_url.empty()) {
      std::string new_url(previous_url);

#ifdef DEBUG
      std::cout << "previous url is " << previous_url << std::endl;
#endif


        // Consider url='/dir1/file1.html' Will be 'previos_host''url'

      if(URL[0] == '/') {
        std::string::size_type index = new_url.find('/');
	
        if(index != std::string::npos) {
          while(new_url[index] == '/') index++;
          while(new_url[index] != '/' && new_url[index] != '\'' && new_url[index] != '\0') index++;
				
          if(new_url[index] != '\0')
            new_url.erase(index);
        }
      }

      do {
        std::string::size_type index = new_url.rfind('/');
        std::string::size_type pcolon = new_url.rfind(':');
		
        if(
         index != std::string::npos &&
         (
          pcolon == std::string::npos ||
          index > pcolon
         ) &&
         index > 7
        ) new_url.erase(index);
      } while (
       (URL.substr(0, 3) == "../" && !URL.erase(0, 3).empty()) ||
       (URL.substr(0, 2) == "./" && !URL.erase(0, 2).empty())
      );

      if(URL[0] != '/') new_url += '/';

      new_url += URL;

      if(change_previous) previous_url = new_url;

      return new_url;
    }
  }

  return URL;
}


bool
OksXmHelpWindow::test_anchor(Widget w, String href)
{
  return (
    ((OksXmHelpWindow *)OksXm::get_user_data(w))->was_visited(set_absolute_url(href, false).c_str())
      ? true
      : false
  );
}


void *
OksXmHelpWindow::load_image(Widget w, const char * url)
{
  XmImageInfo *image = 0;

  if (url != 0) {
    std::string URL = set_absolute_url(url, false);
	
    OksXmHelpWindow *helpWnd = (OksXmHelpWindow *)OksXm::get_user_data(w);
    std::string InfoText("Loading image \'");
  
    InfoText += URL + "\'...";

    helpWnd->set_value(idHelpWndInfoBar, InfoText.c_str());

#ifdef DEBUG
    std::cout << "OksXmHelpWindow::load_image(\'" << URL << "\')\n";
#endif

    if(URL.substr(0, 5) == "file:") {
      std::string filename = URL.substr(5, URL.length() - 5);

      helpWnd->set_value(idHelpWndInfoBar, "");

      return (void *)XmHTMLImageDefaultProc(w, (char *)filename.c_str(), 0, 0);
    }
    else {
      HTTPRequest *req = newHTTPRequest();
      std::string filename = helpWnd->cache_dir();
      struct stat buf;
      char *hostname, *file;

      filename += '/';

      req->url = NewString(URL.c_str());

      parseURL(req->url, PARSE_HOSTNAME | PARSE_FILENAME, 0, 0, 0, &hostname, 0, &file);
	
      if(hostname)
      	filename += hostname;
      else
      	filename += "local-host/";
      filename.append(file);

      freeURL(PARSE_HOSTNAME | PARSE_FILENAME, 0, 0, 0, hostname, 0, file);

      for(size_t i = helpWnd->cache_dir().length() + 1; i < filename.length(); i++)
      	if(filename[i] == '/') filename[i] = '@';

      if(stat(filename.c_str(), &buf) != 0 && errno == ENOENT) {
      	req->type = HTTPLoadToFile;
      	req->in_data = NewString(filename.c_str());
      	loadHTTPURL(NULL, req, NULL);

      	if(req->ret != HTTPSuccess) {
            	/* last ditch attempt to try and load it locally */
          image = XmHTMLImageDefaultProc(w, req->url, NULL, 0);

          if(!image)
            std::cerr << "Request failed for image \'" << req->url << "\':\n"
                      << HTTPErrorString(req->ret) << std::endl;
      	}
      	else
          image = XmHTMLImageDefaultProc(w, (char *)filename.c_str(), 0, 0);
      }
      else
      	image = XmHTMLImageDefaultProc(w, (char *)filename.c_str(), 0, 0);

      deleteHTTPRequest(req);
    }

    helpWnd->set_value(idHelpWndInfoBar, "");
  }

  return (void *)image;
}


void
OksXmHelpWindow::trackCB(Widget w, XtPointer, XtPointer data)
{
  static std::string anchor_label;
  
  XmHTMLAnchorPtr href_data = (XmHTMLAnchorPtr)data;

  if(href_data->reason != XmCR_HTML_ANCHORTRACK) return;
  
  if(href_data->href) {

      // first see if an alternate title has been given

    if(!anchor_label.empty()) {
      if(href_data->title && anchor_label == href_data->title) return;
      if(anchor_label == href_data->href) return;
    }
	
    anchor_label = ( href_data->title ? href_data->title : href_data->href );
  }
  else if(anchor_label.empty()) return;
  else anchor_label = "";

  OksXmHelpWindow *helpWnd = (OksXmHelpWindow *)OksXm::get_user_data(w);

  helpWnd->set_value(idHelpWndInfoBar, anchor_label.c_str());
}


void
OksXmHelpWindow::load_url(Widget html, const char *url)
{
#ifdef DEBUG
  std::cerr << "CALL OksXmHelpWindow::load_url(\'" << url << "\'\n";
#endif

  if(!url) return;

  OksXmHelpWindow *helpWnd = (OksXmHelpWindow *)OksXm::get_user_data(html);
  std::string URL = set_absolute_url(url).c_str();
  std::string URL2 = URL;
  std::string InfoText("Loading \'");
  
  InfoText += URL;
  InfoText += "\'...";

  helpWnd->set_value(idHelpWndInfoBar, InfoText.c_str());
  helpWnd->set_value(idHelpWndCurrentURL, URL2.c_str());

  std::string loc;
  std::string::size_type loc_index = URL.find('#');
		
  if(loc_index != std::string::npos) {
    loc = URL.substr(loc_index, URL.length() - loc_index);
    URL.erase(loc_index);
  }

  if(URL.length() > 5) {
    if(URL.substr(0, 5) == "file:") {
      URL.erase(0, 5);

      std::ifstream f(URL.c_str());

      if(f.good()) {
        std::string str;

	while(f.good() && !f.eof()) {
	  char c;
	  f.get(c);
	  str += c;
	}
	
        if(!str.empty()) XmHTMLTextSetString(html, (char *)str.c_str());
      }
      else {
        Oks::error_msg("OksXmHelpWindow::load_url()") << "Can not read file \'" << URL << "\'\n";

        return;
      }
    }
    else {
      HTTPRequest *req = newHTTPRequest();

      req->url		= NewString(URL.c_str());
      req->type		= HTTPLoadToString;
      req->method	= HTTPGET;
      req->form_data	= 0;
      req->timeout	= helpWnd->timeout();
      req->retry	= helpWnd->number_of_attempts();

      loadHTTPURL(0, req, 0);
		
      if(req->ret == HTTPSuccess) {
        if(req->url) {
          HTTPUnescapeResponse((char *)req->out_data);
          XmHTMLTextSetString(html, (String)req->out_data);
        }
      }
      else {
        std::cerr << "Request failed for URL \'" << req->url << "\':\n"
                  << HTTPErrorString(req->ret) << std::endl;
      }

      deleteHTTPRequest(req);
    }
  }

  if(!loc.empty())
    XmHTMLAnchorScrollToName(html, (char *)loc.c_str());
  else
    XmHTMLTextScrollToLine(html, 0);
 
  OksXm::AutoCstring title(XmHTMLGetTitle(html));
  helpWnd->addToHistory(URL2.c_str(), (title.get() ? title.get() : "<Untitled>"));

  helpWnd->refresh_nav_buttons();
  helpWnd->set_value(idHelpWndInfoBar, "");
  helpWnd->add_visisted(URL2.c_str());
}


void
OksXmHelpWindow::anchorCB(Widget, XtPointer html_w, XtPointer data)
{
  XmHTMLAnchorPtr href_data = (XmHTMLAnchorPtr)data;

  if(href_data->reason != XmCR_ACTIVATE) return;

  switch (href_data->url_type) {
    case ANCHOR_FILE_LOCAL:
    case ANCHOR_HTTP:
      load_url((Widget)html_w, href_data->href);
      break;

    case ANCHOR_JUMP:

        // let XmHTML jump and mark as visited

      if(XmHTMLAnchorGetId((Widget)html_w, href_data->href) != -1) {
        href_data->doit = True;
        href_data->visited = True;
		
        OksXmHelpWindow *helpWnd = (OksXmHelpWindow *)OksXm::get_user_data((Widget)html_w);

        helpWnd->addToHistory(href_data->href, "<Untitled>");
        helpWnd->refresh_nav_buttons();

        return;
      }

      break;

    case ANCHOR_UNKNOWN:
      if(strstr(href_data->href, "?http") != 0) {
        OksXmHelpWindow::load_url((Widget)html_w, href_data->href);
        break;
      }

    default:
      std::cerr << "don't handle this type of url: " << (long)href_data->url_type << ' ' << (long)href_data->href << std::endl;
  }
}


void
OksXmHelpWindow::unmapAC(Widget, XtPointer d, XEvent *e, Boolean *)
{
  if(e->type != UnmapNotify) return;
  
  ((OksXmHelpWindow *)d)->p_help_window_is_shown = false;
}


void
OksXmHelpWindow::navigateCB(Widget w, XtPointer client_data, XtPointer call_data) 
{
  OksXmHelpWindow *dlg = (OksXmHelpWindow *)client_data;
  
  if(w == dlg->get_widget(idBack))
    dlg->back();
  else if(w == dlg->get_widget(idForward))
    dlg->forward();
  else if(w == dlg->get_widget(idGo))
    dlg->makeHistoryPopup(
      XtParent(w),
      (XButtonPressedEvent *)(((XmPushButtonCallbackStruct *)call_data)->event)
    );
  else if(w == dlg->get_widget(idReloadDocument))
    dlg->reload_document();
  else if(w == dlg->get_widget(idReload_images))
    dlg->reload_images();
  else if(w == dlg->get_widget(idRefresh))
    dlg->redisplay();
  else if(w == dlg->get_widget(idInfo))
    dlg->display_doc_info();
}


void
OksXmHelpWindow::add_nav_button(short id, const char *name)
{
  XtAddCallback(
    add_push_button(id, name),
    XmNactivateCallback,
    navigateCB,
    (XtPointer)this
  );
}

void
OksXmHelpWindow::add_info_text(short id, const char *name)
{
  OksXm::TextField::set_not_editable(add_text_field(id, name));
  attach_right(id);
}


OksXmHelpWindow::OksXmHelpWindow(const char *t, OksTopDialog * top) :
  OksDialog		(t, top),
  p_cache_dir		("."),
  p_help_window_is_shown(true),
  visited		(),
  current_pos		(0),
  p_timeout		(10),
  p_number_of_attempts	(1),
  p_top_win             (top)
{
  XtVaSetValues(
    XtParent(get_form_widget()),
    XmNdeleteResponse, XmUNMAP,
    NULL
  );

  const char * cache_dir_env_vars[] = {"OKS_HELP_CACHE_DIR", "HOME", "HOMEDIR"};
  
  for(size_t j=0; j<sizeof(cache_dir_env_vars)/sizeof(const char *);++j) {
    const char *cache_dir_by_env = getenv(cache_dir_env_vars[j]);
    if(cache_dir_by_env && *cache_dir_by_env != 0) {
      p_cache_dir = cache_dir_by_env;
      break;
    }
  }
  
  p_cache_dir += "/.oks-help-cache";


   // create directory if it does not exist

  bool dir_exists = false;

  {
    struct stat buf2;
    if(stat(p_cache_dir.c_str(), &buf2) == 0 && (buf2.st_mode & S_IFDIR)) {
      dir_exists = true;
    }
  }
  

  if(dir_exists == false) {
    if(mkdir(p_cache_dir.c_str(), S_IRWXU)) {
      Oks::error_msg("OKS HELP WINDOW CONSTRUCTOR")
        << "Can not create cache directory \'" << p_cache_dir << "\'\n"
	<< "Reason: \"" << strerror(errno) << "\"\n"
	   "Help may not work\n";
    }
  }


  add_nav_button(idBack, "Back");

  add_nav_button(idForward, "Forward");
  attach_previous(idForward);

  add_nav_button(idGo, "Go");
  attach_previous(idGo);

  add_nav_button(idReloadDocument, "Reload");
  attach_previous(idReloadDocument);

  add_nav_button(idReload_images, "Images");
  attach_previous(idReload_images);

  add_nav_button(idRefresh, "Refresh");
  attach_previous(idRefresh);

  add_nav_button(idInfo, "Info");
  attach_previous(idInfo);

  add_separator();

  add_info_text(idHelpWndCurrentURL, "Location");

  add_separator();

  add_simple_form(idHelpWnd);
  attach_right(idHelpWnd);
  
  html_w = XmCreateHTML(get_form(idHelpWnd)->get_form_widget(), (char *)"name", (ArgList)0, 0);

  XtVaSetValues(html_w,
    XmNtopAttachment, XmATTACH_FORM,
    XmNtopOffset, 2,
    XmNbottomAttachment, XmATTACH_FORM,
    XmNbottomOffset, 2,
    XmNleftAttachment, XmATTACH_FORM,
    XmNleftOffset, 2,
    XmNrightAttachment, XmATTACH_FORM,
    XmNrightOffset, 2,
    XtNwidth, 500,
    XtNheight, 640,
    XmNimageProc, load_image,
    XmNanchorVisitedProc, test_anchor,
    XmNenableBadHTMLWarnings, False,
    XmNuserData, (XtPointer)this,
    NULL
  );

  XtAddCallback(html_w, XmNanchorTrackCallback, trackCB, 0);
  XtAddCallback(html_w, XmNactivateCallback, anchorCB, (XtPointer)html_w);

  XtManageChild(html_w);

  add_separator();
  
  add_info_text(idHelpWndInfoBar, "");

  attach_bottom(idHelpWnd);
    
  XtAddEventHandler(get_form_widget(), StructureNotifyMask, False, OksXmHelpWindow::unmapAC, (XtPointer)this);

  refresh_nav_buttons();

  show();
}


OksXmHelpWindow::~OksXmHelpWindow()
{
  for(size_t i=0; i < history.size(); ++i) delete history[i];

// FIXME: check how it works
//  visited.clearAndDestroy();
}


void
OksXmHelpWindow::showHelpWindow()
{
  if(p_help_window_is_shown) {
	XMapRaised(XtDisplay(get_form_widget()), XtWindow(XtParent(get_form_widget())));
  }
  else {
	XtManageChild(get_form_widget());
	p_help_window_is_shown = true;
  }
}


void
OksXmHelpWindow::displayURL(const char *url)
{
  load_url(html_w, url);
}

void
OksXmHelpWindow::addToHistory(const char *url, const char *title)
{
  if(!url || *url == '\0') return;

  std::string URL;	// will be used if 'url' starts from #
  std::string TITLE;

  if(*url == '#') {
    const HistoryPair *last_hp = history[current_pos];
	
    URL = last_hp->url();
    TITLE = last_hp->title();

    std::string::size_type loc_index = URL.find('#');


      // std::string::erase method on sun/g+=-2.7.2 is called std::string::remove

    if(loc_index != std::string::npos) URL.erase(loc_index);

    URL.append(url);

    url = URL.c_str();
    title = TITLE.c_str();
  }

  if(!history.empty()) {
    if(current_pos < history.size() && (history[current_pos]->url() == url))
      return;
    else if((current_pos + 1) < history.size() && (history[current_pos + 1]->url() == url)) {
      current_pos++;
      return;
    }
    else if(history[current_pos]->url() != url) {
      while(history.size() > (current_pos + 1)) {
        HistoryPair *hp = history.back();
        history.pop_back();
        delete hp;
      }
    }
  }

  history.push_back(new HistoryPair(url, title));
  current_pos = history.size() - 1;

#ifdef DEBUG
  std::cout << "HELP WINDOW HISTORY CONTAINS\n";
  
  std::vector<HistoryPair *>::const_iterator i = history.begin();
  
  for(;i != history.end();++i)
    std::cout << "  \'" << (*i)->url() << "\' :: " << (*i)->title() << std::endl;
	
  std::cout << " AND HISTORY' CURRENT POS = " << current_pos << std::endl;
#endif
}

void
OksXmHelpWindow::refresh_nav_buttons() const
{
  XtSetSensitive(
  	get_widget(idBack),
	current_pos > 0 ? true : false
  );

  XtSetSensitive(
  	get_widget(idForward),
	current_pos == (history.size() - 1) ? false : true
  );

  XtSetSensitive(
  	get_widget(idGo),
	history.size() < 2 ? false : true
  );
}

void
OksXmHelpWindow::back()
{
  if(history.size() && current_pos > 0) {
    current_pos--;
    displayURL(history[current_pos]->url().c_str());
  }
}

void
OksXmHelpWindow::forward()
{
  if(current_pos < (history.size()-1)) {
    current_pos++;
    displayURL(history[current_pos]->url().c_str());
  }
}


void
OksXmHelpWindow::set_position(size_t pos)
{
  if(history.size() < 2) return;
  
  if(pos != current_pos) {
    current_pos = pos;
    displayURL(history[current_pos]->url().c_str());
  }
}


void
OksXmHelpWindow::add_info_item(OksDialog *dlg, const char *name, const char *value, short id)
{
  XtVaSetValues(
    dlg->add_text_field(id, name),
    XmNcursorPositionVisible, false,
    XmNeditable, false,
    XmNverifyBell, false,
    NULL
  );

  while(strchr(value, '\n') != 0) *strchr(const_cast<char *>(value), '\n') = ' ';
  while(strchr(value, '\r') != 0) *strchr(const_cast<char *>(value), '\r') = ' ';
  while(strchr(value, '\t') != 0) *strchr(const_cast<char *>(value), '\t') = ' ';

  dlg->set_value(id, value);
  dlg->attach_right(id);
}


void
OksXmHelpWindow::info_closeCB(Widget, XtPointer d, XtPointer)
{
  delete (OksDialog *)d;
}


void
OksXmHelpWindow::display_doc_info() const
{
  static XmHTMLHeadAttributes head_info;
  bool have_info = XmHTMLGetHeadAttributes(
    html_w,
    &head_info,
    HeadDocType|HeadTitle|HeadBase|HeadMeta
  );

  if(!have_info) {
    std::cerr << "No info\n";
    return;
  }

  OksDialog *info_dlg = new OksDialog("URL info", p_top_win);

  int id = 100;

  info_dlg->add_label(id++, "Basic Properties:");

  add_info_item(
    info_dlg,
    "Document Title:",
    head_info.title ? head_info.title : "<Untitled>",
    id++
  );  
  
  add_info_item(
    info_dlg,
    "Document Info:",
    head_info.doctype ? head_info.doctype : "<Unspecified>",
    id++
  );  
  
  add_info_item(
    info_dlg,
    "Document Base:",
    head_info.base ? head_info.base : "<Not specified>",
    id++
  );  

  if(head_info.num_meta) {
    info_dlg->add_separator();
    info_dlg->add_label(id++, "Optional Properties:");

    for(int i = 0; i < head_info.num_meta; i++)
      add_info_item(
        info_dlg,
        head_info.meta[i].http_equiv ? head_info.meta[i].http_equiv : head_info.meta[i].name,
        head_info.meta[i].content,
        id++
      );  
  }

  info_dlg->add_separator();

  XtAddCallback(info_dlg->addCloseButton(), XmNactivateCallback, info_closeCB, (XtPointer)info_dlg);
  OksXm::set_close_cb(XtParent(info_dlg->get_form_widget()), info_closeCB, (void *)info_dlg);

  info_dlg->attach_bottom(--id);

  info_dlg->show();

  Dimension height;

  XtVaGetValues(XtParent(info_dlg->get_form_widget()), XmNheight, &height, NULL);
  XtVaSetValues(XtParent(info_dlg->get_form_widget()), XmNminHeight, height, XmNmaxHeight, height, NULL);
}


void
OksXmHelpWindow::reload_document() const
{
  std::cout << "Reload Document not implemented!\n";
}

void
OksXmHelpWindow::reload_images() const
{
  std::cout << "Reload Images not implemented!\n";
}

void
OksXmHelpWindow::historyCB(Widget w, XtPointer client_data, XtPointer)
{
  ((OksXmHelpWindow *)client_data)->set_position((size_t)OksXm::get_user_data(w));
}

void
OksXmHelpWindow::makeHistoryPopup(Widget parent, XButtonPressedEvent *event) const
{
  if(history.size() < 2) return;

  OksPopupMenu popup(parent);

  std::vector<HistoryPair *>::const_iterator i = history.begin();

  for(size_t j=0; i != history.end();++i, ++j) {
    if(j != current_pos)
      popup.addItem((*i)->title().c_str(), j, historyCB, (XtPointer)this);
    else {
      if(j != 0) popup.add_separator();

      popup.addDisableItem((*i)->title().c_str());

      if(j != (history.size() - 1)) popup.add_separator();
    }
  }

  popup.show(event);
}


void
OksXmHelpWindow::redisplay() const
{
  XmHTMLRedisplay(html_w);
}


void
OksXmHelpWindow::add_visisted(const std::string& s)
{
  if(visited.find(s) == visited.end()) visited.insert(s);
}


bool
OksXmHelpWindow::was_visited(const std::string& s) const
{
  return (visited.find(s) != visited.end());
}
