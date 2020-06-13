/*============================================================================
  
  qcd 
  
  qcd_list_sel.c

  Copyright (c)1990-2020 Kevin Boone. Distributed under the terms of the
  GNU Public Licence, v3.0

  ==========================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <errno.h> 
#include <getopt.h> 
#include <unistd.h> 
#include <time.h> 
#include <klib/klib.h> 
#include "qcd_list_sel.h" 
#include "qcd_db.h" 
#include "qcd_ops.h" 

#define KLOG_CLASS "qcd.term"

/*============================================================================
  
  QcdListSel

  ==========================================================================*/

struct _QcdListSel
  {
  KTerminal *term;
  const KList *dirs; // A list of char *
  int top_row_file; // The index into files that is shown in the top line 
  int current_file; // Currently-selected index into files
  };


/*============================================================================
  
  qcd_list_sel_new

  ==========================================================================*/
QcdListSel  *qcd_list_sel_new (const KList *dirs)
  {
  KLOG_IN
  QcdListSel *self = malloc (sizeof (QcdListSel));
  self->top_row_file = 0;
  self->dirs = dirs;
  KLOG_OUT
  return self;
  }

/*============================================================================
  
  qcd_list_sel_destroy

  ==========================================================================*/
void qcd_list_sel_destroy (QcdListSel *self)
  {
  if (self)
    {
    free (self);
    }
  }

/*============================================================================
  
  qcd_list_sel_deinit

  ==========================================================================*/
void qcd_list_sel_deinit (QcdListSel *self)
  {
  KLOG_IN
  kterminal_deinit (self->term, NULL);
  KLOG_OUT
  }

/*============================================================================
  
  qcd_list_sel_init

  ==========================================================================*/
BOOL qcd_list_sel_init (QcdListSel *self, KTerminal *terminal, KString **error)
  {
  KLOG_IN
  BOOL ret = FALSE;
  self->term = terminal;
  if (kterminal_init (terminal, error))
    {
    int rows, cols;
    kterminal_get_size (terminal, &rows, &cols, NULL);
    klog_debug (KLOG_CLASS, "Initial terminal size: %d, %d", rows, cols);
    ret = TRUE;
    }
  KLOG_OUT
  return ret;
  }

/*============================================================================
  
  qcd_make_line

  ==========================================================================*/
KString *qcd_make_line (const QcdListSel *self, const char *dir)
  {
  KLOG_IN
  KString *ret = kstring_new_from_utf8 ((UTF8*)dir);
  KLOG_OUT
  return ret;
  }

/*============================================================================
  
  qcd_refresh_display

  ==========================================================================*/
void qcd_refresh_display (QcdListSel *self)
  {
  KLOG_IN
  kterminal_clear (self->term);
  int rows = 25; int cols = 80;
  int nfiles = klist_length (self->dirs);
  kterminal_get_size (self->term, &rows, &cols, NULL);
  for (int i = 0; i < rows - 1 && i + self->top_row_file < nfiles; i++)
    {
    KString *line = qcd_make_line (self, klist_get (self->dirs, 
        i + self->top_row_file));
    if (i + self->top_row_file == self->current_file)
      {
      kterminal_set_attributes (self->term, KTATTR_REVERSE, KTATTR_ON);
      kterminal_write_at (self->term, i, 0, line, TRUE);
      kterminal_set_attributes (self->term, KTATTR_REVERSE, KTATTR_OFF);
      }
    else
      {
      kterminal_write_at (self->term, i, 0, line, TRUE);
      }
    kstring_destroy (line);
    }

  KString *bar = kstring_new_from_utf8((UTF8 *)
    "Select(Enter)/Quit(Q)/Up/Down/PgUp/PgDn");
  kterminal_write_at (self->term, rows - 1, 0, bar, TRUE);
  kstring_destroy (bar);
  kterminal_set_cursor (self->term, rows, 39); 
  KLOG_OUT
  }

/*============================================================================
  
  qcd_list_sel_del

  ==========================================================================*/
void qcd_list_sel_del (QcdListSel *self, const KPath *db_path, char *dir)
  {
  int rows = 25; int cols;
  kterminal_get_size (self->term, &rows, &cols, NULL);
  KString *s = kstring_new_empty();
  kstring_append_printf (s, "Remove %s? (y/n)", dir);
  kterminal_erase_line (self->term, rows - 1);
  kterminal_write_at (self->term, rows - 1, 0, s, TRUE);
  kstring_destroy (s);
  int key = kterminal_read_key (self->term);
  if (key == 'y' || key == 'Y')
    {
    qcd_ops_del (db_path, dir);
    }
  }

/*============================================================================
  
  qcd_list_sel_loop

  ==========================================================================*/
BOOL qcd_list_sel_loop (QcdListSel *self, const KPath *db_path, char **dir)
  {
  KLOG_IN
  qcd_refresh_display (self);
  kterminal_set_raw_mode (self->term, TRUE);
  BOOL ret = FALSE;

  BOOL quit = FALSE;
  do
    {
    int rows = 25; int cols = 80;
    int nfiles = klist_length (self->dirs);
    kterminal_get_size (self->term, &rows, &cols, NULL);
    int key = kterminal_read_key (self->term);
    switch (key)
      {
      case VK_DEL:
        qcd_list_sel_del (self, db_path, 
            klist_get (self->dirs, self->current_file));
        quit = TRUE;
	break;

      case 'q': case 'Q':
        quit = TRUE;
	break;

      case VK_PGUP:
        if (self->current_file > 0)
          {
	  self->current_file -= rows - 2;
	  self->top_row_file -= rows - 2;

	  if (self->current_file < 0) 
	    self->current_file = 0; 

	  if (self->top_row_file < 0) 
	    self->top_row_file = 0; 

	  if (self->top_row_file > self->current_file)
	    self->top_row_file = self->current_file;

	  qcd_refresh_display (self);
          }
        break;

      case VK_PGDN:
        if (self->current_file < nfiles - rows)
          {
	  self->current_file += rows - 2;
	  self->top_row_file += rows - 2;

	  if (self->current_file > nfiles - 1)
	    self->current_file = nfiles - 1;
	  // TODO 

	  qcd_refresh_display (self);
          }
        break;

      case VK_DOWN:
        if (self->current_file < nfiles - 1)
          {
	  self->current_file ++;
	  if (self->current_file - self->top_row_file  >= rows - 2)
	    self->top_row_file = self->current_file - (rows - 2);

	  qcd_refresh_display (self);
          }
        break;

      case 10:
        {
        *dir = strdup (klist_get (self->dirs, self->current_file));
        quit = TRUE;
        ret = TRUE;
	}
        break;

      case VK_UP:
        if (self->current_file > 0)
          {
	  self->current_file --;
	  if (self->current_file < self->top_row_file)
	    self->top_row_file = self->current_file;

	  qcd_refresh_display (self);
          }
        break;
      }
    
    } while (!quit);

  kterminal_set_raw_mode (self->term, FALSE);
  KLOG_OUT
  return ret;
  }

/*============================================================================
  
  qcd_list_sel_run

  ==========================================================================*/
BOOL qcd_list_sel_run (QcdListSel *self, const KPath *db_path, char **dir)
  {
  KLOG_IN
  self->top_row_file = 0;
  self->current_file = 0;
  BOOL ret = qcd_list_sel_loop (self, db_path, dir);
  KLOG_OUT
  return ret;
  }

