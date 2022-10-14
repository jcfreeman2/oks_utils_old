#ifndef __DATA_EDITOR_OBJECT_DLG_H
#define __DATA_EDITOR_OBJECT_DLG_H 

#include <string>

#include "data_editor_dialog.h"

struct OksData;
class OksObject;
class OksPopupMenu;

class OksDataEditorObjectDialog : public OksDataEditorDialog
{
  friend class OksDataEditorMainDialog;


  public:

    OksDataEditorObjectDialog	(OksObject*, OksDataEditorMainDialog*);

    ~OksDataEditorObjectDialog	();
    OksDataEditorDialog::Type	type() const {return OksDataEditorDialog::ObjectDlg;}

    OksObject*			getObject() const {return o;}
    void			setObject(OksObject *obj) {o = obj;}

    void			refresh() const;
    void			refresh_single_value_attribute(unsigned short, const OksAttribute *) const;
    void			refresh_multi_value_attribute(unsigned short, const OksAttribute *) const;
    void			refresh_single_value_relationship(unsigned short) const;
    void			refresh_multi_value_relationship(unsigned short) const;
    void			refresh_file_name();

    static void			add_resize_list_menu(Widget, OksPopupMenu&);


  private:

    OksObject			*o;
    std::list<std::string *>	toBeDeleted;
    std::list<OksXm::TipsInfo *> p_label_tips;

    void			createObjectByName(char *);

    OksRelationship *		getRelationship(int) const;
    OksAttribute *		getAttribute(int) const;

    void			updateValue(OksAttribute *, const char *);

    static std::string		printValueWithBase(const char *, OksData::Type, int, int);

    static void			processItemMove(Widget, int, bool, OksData *, OksData *&, OksData *&);
    static void			swap(OksData *, OksData *);


      // show information dialogs

    void			show_description(const OksAttribute *, const OksRelationship *) const;


      // callbacks

    static void			changeListHeightCB(Widget, XtPointer, XtPointer);
    static void			closeCB(Widget, XtPointer, XtPointer);
    static void			createNewObjectCB(Widget, XtPointer, XtPointer);
    static void			renameObjectCB(Widget, XtPointer, XtPointer);
    static void			actionCB(Widget, XtPointer, XtPointer);
    static void			objAloneCB(Widget, XtPointer, XtPointer);
    static void			objsListCB(Widget, XtPointer, XtPointer);
    static void			showObjectCB(Widget, XtPointer, XtPointer);
    static void			setValueCB(Widget, XtPointer, XtPointer);
    static void			changeBaseCB(Widget, XtPointer, XtPointer);
    static void			valueAloneCB(Widget, XtPointer, XtPointer);
    static void			addValueCB (Widget, XtPointer, XtPointer);
    static void			enumValueCB(Widget, XtPointer, XtPointer);
    static void			valuesListCB(Widget, XtPointer, XtPointer);
    static void			changeEnumTypeCB(Widget, XtPointer, XtPointer);
    static void                 changeClassTypeCB(Widget, XtPointer, XtPointer);
//    static void                 fileCB(Widget, XtPointer, XtPointer);

    static OksData *            get_by_pos(OksData *, int);
    static void                 delete_by_pos(OksData *, int);

    static void                 process_mv_cb(int, const char *, OksObject *, OksAttribute *, Widget, OksDataEditorObjectDialog *, int);


      // actions

    static void                 objectAC(Widget, XtPointer, XEvent *, Boolean *);
    static void                 objectsAC(Widget, XtPointer, XEvent *, Boolean *);
    static void                 dlgAC(Widget, XtPointer, XEvent *, Boolean *);
    static void                 valueAC(Widget, XtPointer, XEvent *, Boolean *);
    static void                 valuesAC(Widget, XtPointer, XEvent *, Boolean *);
//    static void                 fileAC(Widget, XtPointer, XEvent *, Boolean *);
    static void                 singleValueSetSensAC(Widget, XtPointer, XEvent *, Boolean *);

      // identifiers of action callbacks for pop-up menu

    enum {
      idAcObjectSelect = 111,
      idAcObjectDelete,
      idAcObjectCopy,
      idAcObjectRename,
      idAcObjectMoveTo,
      idAcObjectShowReferencedBy,
      idAcObjectValuesListAdd,
      idAcObjectValuesListModify,
      idAcObjectValuesListDelete,
      idAcObjectValuesListUp,
      idAcObjectValuesListDown,
      idAcObjectValuesListSortAscending,
      idAcObjectValuesListSortDescending,
      idAcObjectValueAloneSet,
      idAcObjectValueAloneClear,
      idAcObjectsListShow,
      idAcObjectsListAdd,
      idAcObjectsListRemove,
      idAcObjectsListUp,
      idAcObjectsListDown,
      idAcObjectAloneSet,
      idAcObjectAloneClear,
      idAcObjectAloneShow,
      idAcObjectAny,
      idAcObjectShowDescription,
      idAcObjectFileCheckout
    };


      // supported base names for integers

    static const char * dec_type;
    static const char * hex_type;
    static const char * oct_type;
};



#endif
