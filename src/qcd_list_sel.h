/*============================================================================
  
  qcd  
  
  qcd_list_sel.h

  Copyright (c)1990-2020 Kevin Boone. Distributed under the terms of the
  GNU Public Licence, v3.0

  ==========================================================================*/

#include <klib/klib.h>

struct _QcdListSel;
typedef struct _QcdListSel QcdListSel;

extern QcdListSel  *qcd_list_sel_new (const KList *list);
extern void      qcd_list_sel_destroy (QcdListSel *self);

extern BOOL      qcd_list_sel_init (QcdListSel *self, KTerminal *terminal, 
                   KString **error);
extern BOOL      qcd_list_sel_run (QcdListSel *self, const KPath *db_path, 
                   char **dir);
extern void      qcd_list_sel_deinit (QcdListSel *self);

