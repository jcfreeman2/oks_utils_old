#ifndef __SCHEMA_EDITOR_MAIN_DLG_H
#define __SCHEMA_EDITOR_MAIN_DLG_H 

#include <string>
#include <list>

#include <oks/kernel.h>
#include <oks/xm_dialog.h>
#include <oks/xm_popup.h>

class OksMethodImplementation;

class OksSchemaEditorDialog;
class OksSchemaEditorSchemaDialog;
class OksSchemaEditorViewDialog;
class OksSchemaEditorClassDialog;
class OksSchemaEditorAttributeDialog;
class OksSchemaEditorRelationshipDialog;
class OksSchemaEditorMethodDialog;
class OksSchemaEditorMethodImplementationDialog;
class OksXmMessageWindow;
class OksWindowBuffer;


class OksSchemaEditorMainDialog : public OksTopDialog, public OksKernel
{

  public:

    enum DialogReturn {NoAnswer, Save, Skip, CancelExit, NonStop};

    OksSchemaEditorMainDialog		(int *, char**, const char *, const char * fallback_resources[][2], bool, bool, bool);
    ~OksSchemaEditorMainDialog		();
	
    void				refresh();
    void				showClassList();
    void				showSchemaList();

    void				create_menu_bar(Widget);
    void				update_menu_bar();
    Widget				get_menu_bar() const {return p_menu_bar;}

    DialogReturn			confirm_changed(const char *, OksFile *);

    OksXmMessageWindow*			get_msg_window() const {return p_msg_window;}

    OksSchemaEditorSchemaDialog *	find_schema_dlg(const OksFile *);
    OksSchemaEditorClassDialog *	find_class_dlg(const OksClass *);
    OksSchemaEditorViewDialog *		find_view_dlg(const std::string&);
    OksSchemaEditorAttributeDialog *	find_attribute_dlg(const OksAttribute *, const OksClass *);
    OksSchemaEditorRelationshipDialog *	find_relationship_dlg(const OksRelationship *, const OksClass *);
    OksSchemaEditorMethodDialog *	find_method_dlg(const OksMethod *, const OksClass *);
    OksSchemaEditorMethodImplementationDialog *	find_method_implementation_dlg(const OksMethodImplementation *);

    void				create_schema_dlg(OksFile *);
    OksSchemaEditorClassDialog *	create_class_dlg(OksClass *);
    OksSchemaEditorViewDialog *		create_view_dlg(const std::string &);
    OksSchemaEditorClassDialog *	create_new_class_dlg();
    OksSchemaEditorAttributeDialog *	create_attribute_dlg(OksAttribute *, OksClass *);
    OksSchemaEditorRelationshipDialog *	create_relationship_dlg(OksRelationship *, OksClass *);
    OksSchemaEditorMethodDialog *	create_method_dlg(OksMethod *, OksClass *);
    OksSchemaEditorMethodImplementationDialog *	create_method_implementation_dlg(OksMethodImplementation *, OksMethod *, OksClass *);
    void				create_select_class_dlg(const char *, const std::list<OksClass *> *, XtCallbackProc, XtPointer);
    static void				destroy_select_class_dlg(Widget w);

    void				remove_dlg(OksSchemaEditorDialog *);

    void				add_class_to_schema_dlg(OksFile *);
    void				remove_class_from_schema_dlg(const OksClass *, OksFile *);

    static bool				is_valid_active_schema();

    void				fill_schema_row(OksFile *, String *) const;
    void				update_schema_row(OksFile *) const;
    void				append_schema_row(OksFile *) const;

    bool				is_verbouse_out() const {return p_verbouse_out;}

    int					get_multi_click_time() const {return p_mt_click_time;}

    void                                create_backup(OksFile *);
    void                                remove_backup(OksFile *);

    static void 			exit_app(const char * message);

    void                                test_bind_classes_status() const;


  private:

    std::list<OksSchemaEditorDialog *>	dialogs;

    Widget				p_menu_bar;
    Widget				p_windows_pane;
    Widget				p_schema_matrix;
    Widget                              backupPeriodBtn;

    OksXmMessageWindow *		p_msg_window;
    bool				p_no_msg_window;
    
    bool				p_verbouse_out;
    
    int					p_mt_click_time;

    int                                 p_check_need_backup_timeout;
    XtIntervalId                        p_check_need_backup_timer;
    std::set<OksFile *>                 p_backup_files;

    static void				deleteNF(OksClass *c);
    static void				createNF(OksClass *c);
    static void				changeNF(OksClass *c, OksClass::ChangeType, const void *);

    void				update();
    void				raise_dlg(OksSchemaEditorDialog *) const;

    void                                start_check_need_backup();
    void                                stop_check_need_backup();

    static void                         check_need_backup_files_cb(XtPointer, XtIntervalId*);

    static void				cancelCB(Widget, XtPointer, XtPointer);
    static void				classCB(Widget, XtPointer, XtPointer);
    static void				classDefaultCB(Widget, XtPointer, XtPointer);
    static void				labelCB(Widget, XtPointer, XtPointer);
    static void				menuCB(Widget, XtPointer, XtPointer);
    static void				openCB(Widget, XtPointer, XtPointer);
    static void				raiseCB(Widget, XtPointer, XtPointer);
    static void				saveCB(Widget, XtPointer, XtPointer);
    static void				schemaCB(Widget, XtPointer, XtPointer);
    static void				schemaDefaultCB(Widget, XtPointer, XtPointer);

    static void				classAC(Widget, XtPointer, XEvent *, Boolean *);
    static void				schemaAC(Widget, XtPointer, XEvent *, Boolean *);
    static void                         configureAC (Widget, XtPointer, XEvent *, Boolean *);


    enum {
      idAcMainSchemaNew = 100,
      idAcMainSchemaOpen,
      idAcMainSchemaClose,
      idAcMainSchemaCloseAll,
      idAcMainSchemaSave,
      idAcMainSchemaForceSave,
      idAcMainSchemaSaveAs,
      idAcMainSchemaSaveAll,
      idAcMainSchemaSetActive,
      idAcMainSchemaUnsetActive,
      idAcMainSchemaShowClasses,

      idAcMainClassNew,
      idAcMainClassShow,
      idAcMainClassDelete,

      idAcMainViewNew,
      idAcMainViewOpen,
      idAcMainViewMakeSchema,

      idMenuOptionCheckNeedBackupMode,
      idMenuOptionCheckNeedBackupPeriod,

      idMenuFileExit,
      idMenuWindowMessages,
      idMenuHelpAbout,
      idMenuHelpHelp
    };

};


#endif
