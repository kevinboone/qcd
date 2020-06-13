/*============================================================================
  
  qcd 
  
  qcd_db.c

  Copyright (c)1990-2020 Kevin Boone. Distributed under the terms of the
  GNU Public Licence, v3.0

  ==========================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <errno.h> 
#include <getopt.h> 
#include <unistd.h> 
#include <assert.h> 
#include <klib/klib.h> 
#include "qcd_db.h" 
#include "sqlite3.h" 

#define KLOG_CLASS "qcd.db"

void qcd_db_close (QcdDb *self); // FWD
static BOOL qcd_db_exec (QcdDb *self, UTF8 *sql, KString **error); // FWD
static char *qcd_db_escape_sql (const char *sql);
KList *qcd_db_query (QcdDb *self, const char *sql, BOOL include_empty,
        int limit, KString **error); //FWD

/*============================================================================
  
  QcdDb 

  ==========================================================================*/
struct _QcdDb
  {
  sqlite3 *sqlite;
  char *file;
  };

/*============================================================================
  
  qcd_db_new

  ==========================================================================*/
QcdDb *qcd_db_new (const KPath *file)
  {
  KLOG_IN
  QcdDb *self = malloc (sizeof (QcdDb));
  self->file = (char *) kstring_to_utf8 ((KString *)file); 
  self->sqlite = NULL;
  KLOG_OUT
  return self;
  }

/*============================================================================
  
  qcd_db_destroy

  ==========================================================================*/
void qcd_db_destroy (QcdDb *self)
  {
  KLOG_IN
  if (self)
    {
    qcd_db_close (self);
    if (self->file) free (self->file);
    free (self);
    }
  KLOG_OUT
  }

/*============================================================================
  
  qcd_db_get_count

  ==========================================================================*/
BOOL qcd_db_get_count (QcdDb *self, const UTF8 *dir, int *count, 
      KString **error)
  {
  KLOG_IN
  assert (self != NULL);
  assert (dir != NULL);
  assert (self->sqlite != NULL);

  *count = 0;  
  char *escaped_dir = qcd_db_escape_sql ((char *)dir); 

  KString *sql = kstring_new_empty();
  kstring_append_printf (sql, 
      "select count from dirs where dir='%s'", escaped_dir);
  UTF8 *_sql = kstring_to_utf8 (sql);

  BOOL ret = FALSE;
  KList *results = qcd_db_query (self, (char*)_sql, FALSE,
        1, error); 
  
  if (results)
    {
    if (klist_length (results) == 1)
      {
      const char *s = klist_get (results, 0);
      *count = atoi (s);
      }
    klist_destroy (results);
    ret = TRUE;
    }

  free (_sql);
  kstring_destroy (sql);

  free (escaped_dir);
  KLOG_OUT
  return ret;
  }

/*============================================================================
  
  qcd_db_del_dir

  ==========================================================================*/
BOOL qcd_db_del_dir (QcdDb *self, const UTF8 *dir, KString **error)
  {
  KLOG_IN
  assert (self != NULL);
  assert (dir != NULL);
  assert (self->sqlite != NULL);
  BOOL ret = TRUE;
  char *escaped_dir = qcd_db_escape_sql ((char *)dir); 

  KString *sql = kstring_new_empty();
  kstring_append_printf (sql, 
      "delete from dirs where dir='%s'", escaped_dir);
  UTF8 *_sql = kstring_to_utf8 (sql);
  ret = qcd_db_exec (self, _sql, error);
  free (_sql);
  kstring_destroy (sql);

  free (escaped_dir);
  KLOG_OUT
  return ret;
  }

/*============================================================================
  
  qcd_db_add_dir

  ==========================================================================*/
BOOL qcd_db_add_dir (QcdDb *self, const UTF8 *dir, KString **error)
  {
  KLOG_IN
  assert (self != NULL);
  assert (dir != NULL);
  assert (self->sqlite != NULL);
  BOOL ret = TRUE;
  char *escaped_dir = qcd_db_escape_sql ((char *)dir); 

  int count = 0;
  if (qcd_db_get_count (self, dir, &count, error))
    {
    if (count < 1)
      { 
      KString *sql = kstring_new_empty();
      kstring_append_printf (sql, 
      "insert into dirs (dir, count) values ('%s',1)", escaped_dir);
      UTF8 *_sql = kstring_to_utf8 (sql);
      ret = qcd_db_exec (self, _sql, error);
      free (_sql);
      kstring_destroy (sql);
      }
    else
      {
      // Increment dir count
      KString *sql = kstring_new_empty();
      kstring_append_printf (sql, 
      "update dirs set count=%d where dir='%s'", count+1, escaped_dir);
      UTF8 *_sql = kstring_to_utf8 (sql);
      ret = qcd_db_exec (self, _sql, error);
      free (_sql);
      kstring_destroy (sql);
      }
    }
  else
    {
    ret = FALSE;
    }

  free (escaped_dir);
  KLOG_OUT
  return ret;
  }

/*============================================================================
  
  qcd_db_close

  ==========================================================================*/
void qcd_db_close (QcdDb *self)
  {
  KLOG_IN
  assert (self != NULL);
  if (self->sqlite) sqlite3_close (self->sqlite);
  self->sqlite = NULL;
  KLOG_OUT
  // TODO
  }

/*============================================================================
  
  qcd_db_create_tables

  ==========================================================================*/
static BOOL qcd_db_exec (QcdDb *self, UTF8 *sql, KString **error)
  {
  KLOG_IN
  BOOL ret = FALSE;
  assert (self != NULL);
  assert (sql != NULL);

  char *e = NULL;
  klog_debug (KLOG_CLASS, "%s: executing SQL %s", __PRETTY_FUNCTION__, sql);
  sqlite3_exec (self->sqlite, (char *)sql, NULL, 0, &e);
  if (e)
    {
    if (error) (*error) = kstring_new_from_utf8 ((UTF8 *)e); 
    sqlite3_free (e);
    ret = FALSE;
    }
  else
    ret = TRUE;

  KLOG_OUT
  return ret;
  }


/*============================================================================
  
  qcd_db_create_tables

  ==========================================================================*/
static BOOL qcd_db_create_tables (QcdDb *self, KString **error)
  {
  KLOG_IN
  BOOL ret = FALSE;
  assert (self != NULL);
  ret = qcd_db_exec (self, (UTF8 *)"create table dirs "
       "(dir varchar not null primary key, count integer)",
        error);

  if (ret)
    ret = qcd_db_exec (self, (UTF8*)"create index dirindex on dirs(dir)",
       error);

  KLOG_OUT
  return ret;
  }

/*============================================================================
  
  qcd_db_escape_sql

  ==========================================================================*/
static char *qcd_db_escape_sql (const char *sql)
  {
  KLOG_IN
  char *ret;
  if (strchr (sql, '\''))
    {
    int l = strlen (sql);
    int newlen = l;
    ret = malloc (newlen + 1);
    int p = 0;

    for (int i = 0; i < l; i++)
      {
      char c = sql[i];
      if (c == '\'')
        {
        newlen+=2;
        ret = realloc (ret, newlen + 1);
        ret[p++] = '\'';
        ret[p++] = '\'';
        }
      else
        {
        ret[p++] = c;
        }
      }
    ret[p] = 0;
    }
  else
    {
    ret = strdup (sql);
    }
  KLOG_OUT
  return ret;
  }

/*============================================================================
  
  qcd_db_match_dir

  ==========================================================================*/
KList *qcd_db_match_dir (QcdDb *self, const char *term, KString **error)
  {
  KLOG_IN
  assert (self != NULL);
  assert (term != NULL);

  char *escaped_term = qcd_db_escape_sql ((char *)term); 
  KString *sql = kstring_new_empty();
  kstring_append_printf (sql, 
      "select dir from dirs where dir like '%%%s%%' order by count desc", 
      escaped_term);

  UTF8 *_sql = kstring_to_utf8 (sql);
  KList *results = qcd_db_query (self, (char*)_sql, FALSE, 0, error); 

  free (_sql);
  kstring_destroy (sql);

  free (escaped_term);

  KLOG_OUT
  return results;
  }

/*============================================================================
  
  qcd_db_open

  ==========================================================================*/
extern BOOL qcd_db_open (QcdDb *self, KString **error)
  {
  KLOG_IN
  assert (self != NULL);
  klog_debug (KLOG_CLASS, "Opening database file %s", self->file);
  BOOL ret = FALSE;
  BOOL create_tables = FALSE;
 
  if (access (self->file, F_OK) != 0)
    {
    // DB files does not yet exist -- we will need to create tables.
    create_tables = TRUE;
    }

  int err = sqlite3_open (self->file, &self->sqlite);
  if (err == 0)
    {
    if (create_tables)
      {
      if (qcd_db_create_tables (self, error))
        ret = TRUE;
      }
    else
      ret = TRUE;
    }
  else
    {
    // TODO -- better error message
    if (error)
      *error = kstring_new_from_utf8 ((UTF8 *)"Can't open database");
    }

  KLOG_OUT
  return ret;
  }

/*==========================================================================

  qcd_db_query 

  Returns a List of char *, which may be empty. The list returns includes
  all rows and columns in row order, all concetenated into one long list.
  If the query selects a single column, then the list returned will be
  the same length as the number of matches, or 'limit' if that is
  smaller. If limit == 0, then no limit is applied

==========================================================================*/
KList *qcd_db_query (QcdDb *self, const char *sql, BOOL include_empty,
        int limit, KString **error)
  {
  KLOG_IN
  KList *ret = NULL;
  char *e = NULL;
  char **result = NULL;
  int hits;
  int cols;
  klog_debug (KLOG_CLASS, "%s: executing SQL %s", __PRETTY_FUNCTION__, sql);
  sqlite3_get_table (self->sqlite, sql, &result, &hits, &cols, &e); 
  if (e)
    {
    if (error) (*error) = kstring_new_from_utf8 ((UTF8 *)e);
    sqlite3_free (e);
    ret = NULL;
    }
  else
    {
    ret = klist_new_empty (free); 

    for (int i = 0; i < hits && ((i < limit) || (limit == 0)); i++)
      {
      for (int j = 0; j < cols ; j++)
        {
        char *res = result[(i + 1) * cols + j];
        if (res[0] != 0 || include_empty)
          {
          klist_append (ret, strdup (res));
          }
        }
      }
    sqlite3_free_table (result);
    }
  KLOG_OUT
  return ret;
  }



