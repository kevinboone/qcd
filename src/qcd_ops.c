/*============================================================================
  
  qcd 
  
  qcd_ops.c

  This file contains functions that wrap the sqlite operations 
  for manipulating the directory list database.

  Copyright (c)1990-2020 Kevin Boone. Distributed under the terms of the
  GNU Public Licence, v3.0

  ==========================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <errno.h> 
#include <unistd.h> 
#include <klib/klib.h> 
#include "qcd_db.h" 
#include "qcd_ops.h" 

#define KLOG_CLASS "qcd.ops"

/*============================================================================
  
  qcd_ops_add

  Add an entry to the database

  ==========================================================================*/
void qcd_ops_add (const KPath *db_path, const char *dir)
  {
  KLOG_IN
  // TODO we should remove all multiple and unnecessary / characters
  char *trimmed_dir = strdup (dir);
  if (strcmp (dir, "/") != 0)
    {
    if (trimmed_dir[strlen (trimmed_dir) - 1] == '/')
      {
      trimmed_dir[strlen (trimmed_dir) - 1] = 0;  
      }
    }

  // TODO canonicalize?
   
  QcdDb *qcd_db = qcd_db_new (db_path);
  KString *error = NULL;
  if (qcd_db_open (qcd_db, &error))
    {
    if (qcd_db_add_dir (qcd_db, (UTF8 *)trimmed_dir, &error))
      {
      }
    else
      {
      char *s = (char *)kstring_to_utf8 (error);
      klog_error (KLOG_CLASS, "Can't add directory to database: %s", s); 
      free (s);
      kstring_destroy (error);
      }
    }
  else
    {
    char *s = (char *)kstring_to_utf8 (error);
    klog_error (KLOG_CLASS, "Can't open database: %s", s); 
      free (s);
    kstring_destroy (error);
    }
  qcd_db_destroy(qcd_db);

  free (trimmed_dir);
  KLOG_OUT
  }

/*============================================================================
  
  qcd_ops_del

  Remove an entry to the database

  ==========================================================================*/
void qcd_ops_del (const KPath *db_path, const char *dir)
  {
  KLOG_IN
  QcdDb *qcd_db = qcd_db_new (db_path);
  KString *error = NULL;
  if (qcd_db_open (qcd_db, &error))
    {
    if (qcd_db_del_dir (qcd_db, (UTF8 *)dir, &error))
      {
      }
    else
      {
      char *s = (char *)kstring_to_utf8 (error);
      klog_error (KLOG_CLASS, "Can't add directory to database: %s", s); 
      // TODO put this error where the user can actually see it
      free (s);
      kstring_destroy (error);
      }
    }
  else
    {
      // TODO put this error where the user can actually see it
    char *s = (char *)kstring_to_utf8 (error);
    klog_error (KLOG_CLASS, "Can't open database: %s", s); 
    free (s);
    kstring_destroy (error);
    }
  qcd_db_destroy(qcd_db);
  KLOG_OUT
  }


