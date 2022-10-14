#ifndef __DATA_EDITOR_MAIN_DLG_H
#define __DATA_EDITOR_MAIN_DLG_H 

#include <oks/kernel.h>
#include <oks/xm_dialog.h>

#include <string>
#include <list>
#include <set>

#include <boost/regex.hpp>


#include <Xm/Xm.h>

#include "g_window.h"
#include "data_editor_replace_dlg.h"

class OksDataEditorMainDialog;
class OksDataEditorDialog;
class OksDataEditorClassDialog;
class OksDataEditorDataDialog;
class OksDataEditorObjectDialog;
class OksDataEditorQueryDialog;
class OksDataEditorReplaceDialog;
class OksDataEditorSearchPanel;
class OksDataEditorObjectReferencedByDialog;
class OksDataEditorRepositoryVersionsDialog;
class OksDataEditorVersionsQueryDialog;
class OksDataEditorCommentDialog;

class OksXmMessageWindow;
class OksWindowBuffer;
class OksXmHelpWindow;

class OksQuery;
class OksClass;
class OksObject;
class OksFile;

class G_Class;
class G_WindowInfo;

class OksParameterNames {

  public:
  
    static const std::string & get_icon_width_name() {return p_icon_width_name;}
    static const std::string & get_ch_obj_dx_name() {return p_ch_obj_dx_name;}
    static const std::string & get_ch_obj_dy_name() {return p_ch_obj_dy_name;}
    static const std::string & get_ch_max_width_name() {return p_ch_max_width_name;}
    static const std::string & get_obj_dx_name() {return p_obj_dx_name;}
    static const std::string & get_obj_dy_name() {return p_obj_dy_name;}

    static const std::string & get_arrange_objects_name() {return p_arrange_objs_name;}
    static const std::string & get_arrange_obj_as_row_name() {return p_arrange_obj_as_row_name;}
    static const std::string & get_arrange_obj_as_column_name() {return p_arrange_obj_as_column_name;}
    static const std::string & get_arrange_obj_by_classes_name() {return p_arrange_obj_by_classes_name;}
    static const std::string & get_arrange_obj_wrap_any_name() {return p_arrange_obj_wrap_any_name;}
    static const std::string & get_arrange_obj_wrap_by_classes_name() {return p_arrange_obj_wrap_by_classes_name;}

    static const std::string & get_arrange_children_name() {return p_arrange_child_name;}
    static const std::string & get_arrange_one_child_per_line_name() {return p_arrange_one_child_per_line_name;}
    static const std::string & get_arrange_many_children_per_lin_name() {return p_arrange_many_children_per_lin_name;}

    static const std::string & get_ext_modified_files_check_name() {return p_ext_modified_files_check_name;}
    static const std::string & get_ext_modified_files_value_name() {return p_ext_modified_files_value_name;}
    static const std::string & get_backup_modified_files_check_name() {return p_backup_modified_files_check_name;}
    static const std::string & get_backup_modified_files_value_name() {return p_backup_modified_files_value_name;}
    static const std::string & get_ask_comment_name() {return p_ask_comment_name;}
    static const std::string & get_author_name() {return p_author_name;}

    static const std::string & get_main_window_x_pos() {return p_main_window_x_pos_name;}
    static const std::string & get_main_window_y_pos() {return p_main_window_y_pos_name;}
    static const std::string & get_message_window_x_pos() {return p_message_window_x_pos_name;}
    static const std::string & get_message_window_y_pos() {return p_message_window_y_pos_name;}
    static const std::string & get_restore_main_window_pos() {return p_restore_main_window_pos_name;}



  private:
  
    static std::string p_icon_width_name;
    static std::string p_ch_obj_dx_name;
    static std::string p_ch_obj_dy_name;
    static std::string p_ch_max_width_name;
    static std::string p_obj_dx_name;
    static std::string p_obj_dy_name;

    static std::string p_arrange_objs_name;
    static std::string p_arrange_obj_as_row_name;
    static std::string p_arrange_obj_as_column_name;
    static std::string p_arrange_obj_by_classes_name;
    static std::string p_arrange_obj_wrap_any_name;
    static std::string p_arrange_obj_wrap_by_classes_name;

    static std::string p_arrange_child_name;
    static std::string p_arrange_one_child_per_line_name;
    static std::string p_arrange_many_children_per_lin_name;
    
    static std::string p_ext_modified_files_check_name;
    static std::string p_ext_modified_files_value_name;
    static std::string p_backup_modified_files_check_name;
    static std::string p_backup_modified_files_value_name;
    static std::string p_ask_comment_name;
    static std::string p_author_name;

    static std::string p_main_window_x_pos_name;
    static std::string p_main_window_y_pos_name;
    static std::string p_message_window_x_pos_name;
    static std::string p_message_window_y_pos_name;

    static std::string p_restore_main_window_pos_name;
};

class OksDataEditorMainDialog : public OksTopDialog, public OksKernel
{
  friend class OksDataEditorFilesDialog;
  friend class OksDataEditorCommentDialog;
  friend class OksDataEditorDataDialog;
  friend class OksDataEditorClassDialog;
  friend class OksDataEditorObjectDialog;
  friend class OksDataEditorQueryDialog;
  friend class OksDataEditorReplaceDialog;
  friend class OksDataEditorSearchResultDialog;
  friend class OksDataEditorObjectReferencedByDialog;
  friend class OksDataEditorRepositoryVersionsDialog;
  friend class OksDataEditorMergeConflictsDialog;
  friend class G_Window;
  friend class G_Object;

  public:

    OksDataEditorMainDialog(int *, char**, const char * fallback_resources[][2], bool, bool);
    ~OksDataEditorMainDialog();

      // pass graphical information and init graphical classes

    void init(bool, const char *, const char *, const char *, const char *);


      // define verbose mode

    void set_verbose(bool b) {p_verbose_mode = b;}
    static bool is_verbose() {return p_verbose_mode;}


      // update information in the main window tables

    void refresh_schemes() const;
    void refresh_data() const;
    void refresh_classes() const;

    inline void refresh_main_window() {
      p_am_rights_cache.clear();
      refresh_schemes();
      refresh_data();
      refresh_classes();
    }


      // ask user confirmation when close unsaved changes

    enum DialogReturn {NoAnswer, NonStop, SaveFile, ReloadFile, SkipFile, CancelExit};

    DialogReturn confirm_changes(const char *, OksFile *);
    DialogReturn file_action_dialog(const char *, OksFile *, const char **, const char *, XtCallbackProc, const char *, XtCallbackProc, const char *, XtCallbackProc);

    bool confirm_dialog(const char * title, const char * text, const char ** image);

    G_Class * get_class(const OksObject *) const;
    const std::list<G_Class *>& get_classes() const {return p_classes;}
    G_Class * get_default_class() const {return p_default_class;}

      // get pointers to message and help windows

    OksXmMessageWindow * get_msg_window() const {return msg_window;}
    OksXmHelpWindow * get_help_window();

    OksDataEditorDataDialog *               find_data_dlg(const OksFile *);
    OksDataEditorClassDialog *              find_class_dlg(const OksClass *);
    OksDataEditorObjectDialog *             find_object_dlg(const OksObject *);
    OksDataEditorObjectReferencedByDialog * find_object_referenced_by_dlg(const OksObject *);
    OksDataEditorCommentDialog *            find_comment_dlg(const oks::Comment *);

    void                                    create_object_dlg(const OksClass *);
    OksDataEditorObjectDialog *             create_object_dlg(const OksObject *);
    OksDataEditorObjectReferencedByDialog * create_object_referenced_by_dlg(const OksObject *);
    OksDataEditorDataDialog *               create_data_dlg(OksFile *);
    OksDataEditorClassDialog *              create_class_dlg(const OksClass *);
    OksDataEditorQueryDialog *              create_query_dlg(const OksClass *, OksQuery * = 0);
    OksDataEditorCommentDialog *            create_comment_dlg(OksFile *, const std::string&, oks::Comment *);

    void remove_dlg(OksDataEditorDialog *);

    void close_data_dialogs();
    void refresh_class_dialogs();

    void refresh_object_referenced_by_dialogs();

    const std::list<OksDataEditorDialog *> & get_dialogs() const {return dialogs;}

    void create_find_replace_dialog(OksDataEditorReplaceDialog::ActionType);
    void delete_find_dialog() {findDialog = 0; update_menu_bar();}
    void delete_replace_dialog() {replaceDialog = 0; update_menu_bar();}

    void
    show_versions_query_dialog();

    void
    hide_versions_query_dialog()
    {
      p_versions_query_dlg = nullptr;
      update_menu_bar();
    }

    void
    show_new_repository_versions_dlg(const std::string& version = "");

    void
    show_new_repository_versions_dlg(std::vector<OksRepositoryVersion>, bool add);

    void
    hide_new_repository_versions_dlg()
    {
      p_new_repository_versions_dlg = nullptr;
      update_menu_bar();
    }

    void
    show_archived_repository_versions_dlg(std::vector<OksRepositoryVersion>);

    void
    hide_archived_repository_versions_dlg()
    {
      p_archived_repository_versions_dlg = nullptr;
      update_menu_bar();
    }

    std::string
    get_commit_comments(const std::set<std::string>& created, const std::set<std::string>& updated);


    void add_object_to_data_dlg(OksFile *);
    void remove_object_from_data_dlg(const OksObject *, OksFile *);

    void show_help_window(const char *);

    void update_object_info(const OksObject *, bool);

    static bool is_valid_active_data();

    void get_all_dialogs(std::set<Widget>& result) const;


      // file operations

    void reload(OksFile * f, const std::list<OksFile *> * files = 0);
    void save(OksFile *, bool = false);
    void comment(OksFile *, bool test_parameter = true);
//    void checkout_file(OksFile * f);
//    void release_files(const std::list<OksFile *>& files);

    void create_backup(OksFile *);
    void remove_backup(OksFile *);


      // callbacks

    static void OpenSchemaCB(Widget, XtPointer, XtPointer);

    static void OpenDataCB(Widget, XtPointer, XtPointer);
    static void SaveAsDataCB(Widget, XtPointer, XtPointer);

    static void OpenQueryCB(Widget, XtPointer, XtPointer);

    static void newObjectCB(Widget, XtPointer, XtPointer);

    static void saveChangedCB(Widget, XtPointer, XtPointer);
    static void askUpdatedCB(Widget, XtPointer, XtPointer);
    static void cancelChangedCB(Widget, XtPointer, XtPointer);

    static void check_list_of_files_cb(XtPointer, XtIntervalId*);
    static void check_need_backup_files_cb(XtPointer, XtIntervalId*);

    static bool save_all_files_cb(const std::list<OksFile *>& files, OksDataEditorMainDialog * parent, const char * author, const char * text);
//    static bool release_all_files_cb(const std::list<OksFile *>& files, OksDataEditorMainDialog * parent, const char * author, const char * text);
//    static bool commit_all_files_cb(const std::list<OksFile *>& files, OksDataEditorMainDialog * parent, const char * author, const char * text);
//    static bool update_all_files_cb(const std::list<OksFile *>& files, OksDataEditorMainDialog * parent, const char * author, const char * text);

    static bool commit_all_files_cb(const std::set<std::string>& files, OksDataEditorMainDialog * parent, const char * author, const char * text);

    static void DeleteNotifyFN(OksObject *, void *);
    static void CreateNotifyFN(OksObject *, void *);
    static void ChangeNotifyFN(OksObject *, void *);

    void subscribe()
    {
      subscribe_create_object(CreateNotifyFN, (void *)this);
      subscribe_delete_object(DeleteNotifyFN, (void *)this);
      subscribe_change_object(ChangeNotifyFN, (void *)this);
    }

    void unsubscribe()
    {
      subscribe_create_object(nullptr, nullptr);
      subscribe_delete_object(nullptr, nullptr);
      subscribe_change_object(nullptr, nullptr);
    }

    void add_window(G_Window *win) {p_windows.push_back(win);}
    void remove_window(G_Window *win) {p_windows.remove(win);}

    OksObject * find_oks_object(const char *, const char *) const;

    static void report_error(const char * title, const char * msg, Widget parent = 0);
    static void report_exception(const char * title, const oks::exception& ex, Widget parent = 0);

    static void exit_app(const char * message);

    void test_bind_classes_status() const;

  private:

    std::list<OksDataEditorDialog *> dialogs;

    OksDataEditorReplaceDialog * replaceDialog;
    OksDataEditorReplaceDialog * findDialog;

    OksDataEditorRepositoryVersionsDialog * p_new_repository_versions_dlg;
    OksDataEditorRepositoryVersionsDialog * p_archived_repository_versions_dlg;
    std::string p_last_repository_version;
    bool p_readonly_mode;

    OksDataEditorVersionsQueryDialog * p_versions_query_dlg;

    OksXmMessageWindow * msg_window;
    const bool no_msg_window;

    OksXmHelpWindow * helpWindow;

    Widget menuBar;
    Widget windowsPane;
    bool p_skip_menu_update;
    Widget checkPeriodBtn;
    Widget backupPeriodBtn;

    Widget schema_list;
    Widget data_matrix;
    Widget classes_matrix;

    XtIntervalId p_check_updated_timer;
    XtIntervalId p_check_need_backup_timer;

    std::set<OksFile *> p_backup_files;

    std::string p_base_init_dir;
    std::string p_init_schema_file;
    std::string p_init_data_files;
    std::string p_pixmaps_dirs;
    std::string p_bitmaps_dirs;
    std::string p_init_path;

    std::list<G_Window *> p_windows;
    std::list<G_Class *> p_classes;
    G_Class * p_default_class;
    std::list<G_WindowInfo *> p_win_infos;

    std::list<Widget> p_edit_btns;

    G_Class * p_root_class;
    std::string p_root_relationship;
    std::set<OksObject *> p_root_objects;

    bool p_show_used;
    std::set<OksObject*, std::less<OksObject*> > used;
    std::map<OksClass*, size_t*, std::less<OksClass*> > remove_from_used;

    static bool p_verbose_mode;

    bool p_g_classes_created;

    Pixel p_red_pixel;    // dark red
    Pixel p_green_pixel;  // dark green
    Pixel p_blue_pixel;   // dark blue

    OksDataEditorSearchPanel * p_select_classes_panel;  // criteria to find classes
    OksDataEditorSearchPanel * p_select_data_files_panel;  // criteria to find data files

    std::map<const OksFile *, bool> p_am_rights_cache;
    
    std::string p_selected_query;

    void create_g_classes();
    void remove_g_classes();

    void start_check_updated();
    void stop_check_updated();

    void start_check_need_backup();
    void stop_check_need_backup();

    void raise_dlg(OksDialog *) const;


      // functions to work with menu bar

    void start_menu_bar_pane() const;
    void set_edit_menu();
    void fill_menu_bar(Widget);
    void update_menu_bar(void);
    static void add_window_button(Widget, Widget);


      // functions to work with data file table

    void fill_data_file_row(OksFile *, String *) const;
    void update_data_file_row(OksFile *) const;
    void append_data_file_row(OksFile *) const;
    void set_data_file_row_colors(String *, int) const;

    std::string get_conf_param(const char *, const char *) const;

    int init_window_info(const OksObject *, const OksKernel& gk);
    int init_g_classes();
    G_Class * init_g_class(const OksObject *, const char *, const char *);

    bool uses_object(const OksObject *) const;


      // functions to refresh graphical windows

    static void remove_object_parent_link(OksObject * parent, OksObject * child, const char * rel_name);
    static void add_object_parent_link(OksObject * parent, OksObject * child, const char * rel_name);
    static void swap_objects_parent_link(OksObject * parent, const OksObject * o1, const OksObject * o2, const char * rel_name);


      // functions to make actions

    static void swap_rel_objects(OksObject *, const char *, const OksObject *, const OksObject *);
    static void link_rel_objects(const char *, G_Object *, OksObject *);

    static void show_references(const G_Object *);
    static void show_relationships(const G_Object *);
    static void update_object_id(const OksObject *);

    static std::list<const std::string *> * get_list_of_new_objects(const G_Object *);
    static void create_child_object(G_Object *, const std::string&, const std::string&);

    void add_used(const OksData *);
    void find_used();
    void reset_used();

    void close_graphical_windows();


    void check_objects_and_files() const;
    
    static std::string iso2simple(const std::string&);


      // callbacks

    static void classesLabelCB(Widget, XtPointer, XtPointer);
    static void findClassesCB(Widget, XtPointer, XtPointer);
    static void findDataFilesCB(Widget, XtPointer, XtPointer);
    static void dataLabelCB(Widget, XtPointer, XtPointer);
    static void dataCB(Widget, XtPointer, XtPointer);
    static void classCB(Widget, XtPointer, XtPointer);
    static void raiseCB(Widget, XtPointer, XtPointer);
    static void anyCB(Widget, XtPointer, XtPointer);
    static void MenuCB(Widget, XtPointer, XtPointer);


      // actions

    static void dataAC(Widget, XtPointer, XEvent *, Boolean *);
    static void classAC(Widget, XtPointer, XEvent *, Boolean *);
    static void schemaAC(Widget, XtPointer, XEvent *, Boolean *);
    static void configureAC (Widget, XtPointer, XEvent *, Boolean *);


};


	//
	// Identifiers of action callbacks for pop-up menu
	//

enum {
  idAcMainSchemaOpen = 101,
  idAcMainSchemaClose,
  idAcMainSchemaCloseAll,

  idAcMainDataNew,
  idAcMainDataOpen,
  idAcMainDataReload,
  idAcMainDataClose,
  idAcMainDataCloseAll,
  idAcMainDataSave,
  idAcMainDataSaveForce,
  idAcMainDataSaveAs,
  idAcMainDataSaveAll,
  idAcMainDataSetActive,
  idAcMainDataUnsetActive,
  idAcMainDataShowDetails,

  idAcMainClassShow,
  idAcMainClassQueryShow,
  idAcMainClassQueryLoad,
  idAcMainClassNew,

  idMenuFileSaveAllModified,
  idMenuFileShowVersions,
  idMenuFileUpdateRepository,
  idMenuFileCommitRepository,
  idMenuFileCheckConsistency,
  idMenuFileExit,

  idMenuEditFind,
  idMenuEditReplace,

  idMenuEditRefreshMainWindow,

  idMenuEdit,

  idMenuOptionSilenceMode,
  idMenuOptionVerboseMode,
  idMenuOptionProfilingMode,
  idMenuOptionCheckFileMode,
  idMenuOptionCheckFilePeriod,
  idMenuOptionCheckNeedBackupMode,
  idMenuOptionCheckNeedBackupPeriod,
  idMenuOptionAskCommentOnSave,
  idMenuOptionRestorePosition,
  idMenuOptionSaveOptions,

  idMenuWindowHelp,
  idMenuWindowMessages,

  idMenuHelpAbout,
  idMenuHelpHelp,
  idMenuHelpGeneral,
  idMenuHelpIndex,
  idMenuHelpMainWindow,
  idMenuHelpMessageLog,
  idMenuHelpDataFileWindow,
  idMenuHelpClassWindow,
  idMenuHelpObjectWindow,
  idMenuHelpQueryWindow,
  idMenuHelpGraphicalWindow,
  idMenuHelpReplaceWindow

};

inline bool
OksDataEditorMainDialog::uses_object(const OksObject *o) const
{
  return (used.find(const_cast<OksObject *>(o)) != used.end() ? true : false);
}

#endif
