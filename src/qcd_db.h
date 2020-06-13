/*============================================================================
  
  qcd  
  
  qcd_db.h

  Copyright (c)1990-2020 Kevin Boone. Distributed under the terms of the
  GNU Public Licence, v3.0

  ==========================================================================*/

#include <klib/klib.h>

struct _QcdDb;
typedef struct _QcdDb QcdDb;

extern QcdDb    *qcd_db_new (const KPath *file);
extern void      qcd_db_destroy (QcdDb *self);

extern KList    *qcd_db_match_dir (QcdDb *self, const char *term, 
                    KString **error);
extern BOOL      qcd_db_open (QcdDb *self, KString **error);
extern BOOL      qcd_db_add_dir (QcdDb *self, const UTF8 *dir, 
                    KString **error);
extern BOOL      qcd_db_del_dir (QcdDb *self, const UTF8 *dir, 
                    KString **error);

