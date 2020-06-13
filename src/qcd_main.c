/*============================================================================
  
  qcd 
  
  qcd_main.c

  This file contains the top-level logic for qcd.

  Copyright (c)1990-2020 Kevin Boone. Distributed under the terms of the
  GNU Public Licence, v3.0

  ==========================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <errno.h> 
#include <getopt.h> 
#include <unistd.h> 
#include <klib/klib.h> 
#include "qcd_db.h" 
#include "qcd_list_sel.h" 
#include "qcd_ops.h" 

#define QCD_RC_FILE ".qcd.rc"
#define QCD_DB_FILE ".qcd.db"

#define KLOG_CLASS "qcd.main"

void qcd_check_and_add (const KPath *db_path, const char *dir); // FWD

/*============================================================================
  
  qcd_log_handler

  Note that logging, in the broadest sense, is unhelpful when the program
  is in full-screen mode. It will probably be necessary to implement
  a logfile if detailed debugging is necessary

  ==========================================================================*/
void qcd_log_handler (KLogLevel level, const char *cls, 
                  void *user_data, const char *msg)
  {
  fprintf (stderr, "%s %s: %s\n", klog_level_to_utf8 (level), cls, msg);
  }

/*============================================================================
  
  qcd_show_usage 

  ==========================================================================*/
void qcd_show_usage (const char *argv0, FILE *f) 
  {
  fprintf (f, "Usage: %s [options] {directory}\n", argv0);
  fprintf (f, "    -v, --version  Show version\n");
  fprintf (f, "    -a, --add      Add the current directory to the list\n");
  fprintf 
    (f, "    -d, --delete   Delete the current directory from the list\n");
  fprintf (f, "    -l, --list     Show/edit the complete directory list\n");
  fprintf (f, "        --purge    Remove all stored directories\n");
  }

/*============================================================================
  
  qcd_show_version

  ==========================================================================*/
void qcd_show_version (void) 
  {
  fprintf (stderr, NAME " version " VERSION "\n");
  fprintf (stderr, "Copyright (c)2020 Kevin Boone\n");
  fprintf (stderr, 
     "Distributed according to the terms of the GNU Public Licence, v3.0\n");
  }

/*============================================================================
  
  qcd_select_from_list

  This function must output a single string to stdout if a match is found,
  or the user selects one. Otherwise, it must return FALSE to indicate
  no match. There is no error return from this function, whether there
  are matches or not

  ==========================================================================*/
BOOL qcd_select_from_list (const KPath *db_path, const KList *list)
  {
  BOOL ret = FALSE;
  QcdListSel *qcd_list_sel = qcd_list_sel_new (list);

  KString *error = NULL;
  KTerminal *terminal = (KTerminal *)klinux_terminal_new();
  if (qcd_list_sel_init (qcd_list_sel, terminal, &error))
    {
    char *dir = NULL;
    ret = qcd_list_sel_run (qcd_list_sel, db_path, &dir);
    if (dir)
      {
      printf ("%s\n", dir); 
      qcd_check_and_add (db_path, dir);
      free (dir);
      }
    qcd_list_sel_deinit (qcd_list_sel);
    }

  kterminal_destroy (terminal);
  qcd_list_sel_destroy (qcd_list_sel);
  return ret;
  }

/*============================================================================
  
  qcd_match

  This function must output a single string to stdout if a match is found,
  or the user selects one. Otherwise, it must return FALSE to indicate
  no match. There is no error return from this function, whether there
  are matches or not

  ==========================================================================*/
BOOL qcd_match (const KPath *db_path, const char *term)
  {
  KLOG_IN
  BOOL ret = FALSE;
  QcdDb *qcd_db = qcd_db_new (db_path);
  KString *error = NULL;
  if (qcd_db_open (qcd_db, &error))
    {
    KList *matches = qcd_db_match_dir (qcd_db, term, &error);
    if (matches)
      {
      int l = klist_length (matches);
    
      if (l == 1)
        {
        char *dir = klist_get (matches, 0);
	qcd_check_and_add (db_path, dir);
        printf ("%s\n", dir);
        ret = TRUE;
        }
      else if (l > 1)
        {
        ret = qcd_select_from_list (db_path, matches);
        }
      else
        ret = FALSE;

      klist_destroy (matches);
      }
    else
      {
      char *s = (char *)kstring_to_utf8 (error);
      klog_error (KLOG_CLASS, "Can't query database: %s", s); 
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

  qcd_db_destroy (qcd_db);
  KLOG_OUT
  return ret;
  }

/*============================================================================
  
  qcd_check_and_add

  ==========================================================================*/
void qcd_check_and_add (const KPath *db_path, const char *dir)
  {
  if (access (dir, X_OK) == 0)
    qcd_ops_add (db_path, dir);
  }

/*============================================================================
  
  qcd_is_complete

  Returns TRUE if the directory is complete, that is, something that
  should not be subject to matching. If it's a full pathname, add
  it to the database

  ==========================================================================*/
BOOL qcd_is_complete (const KPath *db_path, const char *term)
  {
  if (term[0] == '/') 
    {
    qcd_check_and_add (db_path, term);
    // We don't try to match full pathnames
    return TRUE;
    }
  else if (strcmp (term, ".") == 0)
    {
    // Don't add this
    return TRUE;
    }
  else if (strcmp (term, "..") == 0)
    {
    // Don't add this
    return TRUE;
    }
  // TODO should we canonicalize relative directories and add them? It 
  //   might be helpful, but we run the risk of ending up with 
  //   every directory in the filesystem in the list.
  struct stat sb;
  if (stat (term, &sb) == 0)
    {
    if (S_ISDIR (sb.st_mode))
      return TRUE;
    }
  return FALSE;
  }

/*============================================================================
  
  qcd_main 

  ==========================================================================*/
int qcd_main (int argc, char **argv)
  {
  // Note that this function _must_ print a directory name to stdout,
  //  whatever else it does. stdout from this program becomes stdin
  //  for the shell built-in cd, so it can't just stop with no output.
  // If the effect of the program is that the current directory should
  //   not change, or there is an error, then the program should print
  //   ".", so that the built-in cd changes to the current directory.

  klog_init (KLOG_ERROR, NULL, NULL);

  BOOL show_version = FALSE;
  BOOL show_usage = FALSE;
  BOOL show_list = FALSE;
  BOOL add_cwd = FALSE;
  BOOL del_cwd = FALSE;
  BOOL purge = FALSE;

  int log_level = KLOG_ERROR;

  // TODO process RC file -- it is not used yet
  KPath *user_rc_path = kpath_new_home();
  kpath_append_utf8 (user_rc_path, (UTF8 *)QCD_RC_FILE);
  kpath_destroy (user_rc_path);

  static struct option long_options[] =
    {
      {"help", no_argument, NULL, 'h'},
      {"version", no_argument, NULL, 'v'},
      {"add", no_argument, NULL, 'a'},
      {"del", no_argument, NULL, 'd'},
      {"list", no_argument, NULL, 'l'},
      {"purge", no_argument, NULL, 0},
      {"log-level", required_argument, NULL, 0},
      {0, 0, 0, 0}
    };

   int opt;
   int ret = 0;
   while (ret == 0)
     {
     int option_index = 0;
     opt = getopt_long (argc, argv, "hvlad",
     long_options, &option_index);

     if (opt == -1) break;

     switch (opt)
       {
       case 0:
         if (strcmp (long_options[option_index].name, "help") == 0)
           show_usage = TRUE;
         else if (strcmp (long_options[option_index].name, "version") == 0)
           show_version = TRUE;
         else if (strcmp (long_options[option_index].name, "log-level") == 0)
           log_level = atoi (optarg); 
         else if (strcmp (long_options[option_index].name, "add") == 0)
           add_cwd = TRUE; 
         else if (strcmp (long_options[option_index].name, "del") == 0)
           del_cwd = TRUE; 
         else if (strcmp (long_options[option_index].name, "list") == 0)
           show_list = TRUE; 
         else if (strcmp (long_options[option_index].name, "purge") == 0)
           purge = TRUE; 
         else
           ret = EINVAL; 
         break;
       case 'h': case '?': 
         show_usage = TRUE; break;
       case 'v': 
         show_version = TRUE; break;
       case 'a': 
         add_cwd = TRUE; break;
       case 'd': 
         del_cwd = TRUE; break;
       case 'l': 
           show_list = TRUE; break;
       default:
           ret = EINVAL;
       }
    }
  
  if (show_version)
    {
    qcd_show_version(); 
    printf (".\n");
    exit (0);
    }

  if (show_usage)
    {
    qcd_show_usage (argv[0], stderr); 
    printf (".\n");
    exit (0);
    }

  klog_set_log_level (log_level);
  klog_set_handler (qcd_log_handler);

  // Find the databse file. Note that db_path must be free'd from 
  // this point on, however the program exits.
  KPath *db_path = kpath_new_home();
  kpath_append_utf8 (db_path, (UTF8 *)QCD_DB_FILE);

  if (purge)
    {
    // Remove the DB file. It will be created again when required
    kpath_unlink (db_path);
    printf (".\n");
    if (db_path) kpath_destroy (db_path);
    exit (0);
    }

  if (add_cwd)
    {
    // Add to the database, if the path is valid. We don't want to add
    //   broken directories to the database
    char cwd[PATH_MAX];
    if (getcwd (cwd, PATH_MAX - 1))
      qcd_check_and_add (db_path, cwd); 
    else
      fprintf (stderr, "Can't add current directory: %s\n", strerror (errno));
    printf (".\n");
    if (db_path) kpath_destroy (db_path);
    exit (0);
    }

  if (del_cwd)
    {
    char cwd[PATH_MAX];
    if (getcwd (cwd, PATH_MAX - 1))
      qcd_ops_del (db_path, cwd); 
    else
      fprintf (stderr, "Can't delete current directory: %s\n", 
       strerror (errno));
    printf (".\n");
    if (db_path) kpath_destroy (db_path);
    exit (0);
    }

  if (show_list)
    {
    if (!qcd_match (db_path, "%"))
      printf (".\n");
    if (db_path) kpath_destroy (db_path);
    exit (0);
    }

  if (argc - optind == 0)
    {
    // We got no arguments. We want to change to the home
    //   directory. So return getenv ("HOME"). This will
    //   crash if HOME is not set. But when does that happen on
    //   Linux?
    printf ("%s\n", getenv ("HOME"));
    if (db_path) kpath_destroy (db_path);
    exit (0);
    }

  if (argc - optind == 1)
    {
    // We have one argument, so this is an attempt to change
    //  directory
    const char *orig_dir = argv[optind]; 
    // is_complete will implicitly add the directory to the database if it is
    //  of a format that makes it suitable to be added. In any event,
    //  we just echo the original directory so the built-in cd can 
    //  pick it up
    if (qcd_is_complete (db_path, orig_dir))
      {
      printf ("%s\n", orig_dir);
      if (db_path) kpath_destroy (db_path);
      exit (0);
      }
    else if (qcd_match (db_path, orig_dir))
      {
      // If this isn't a complete, valid directory, call qcd_match
      //  to process further. qcd_match will either find a matching
      //  directory in the list, or prompt the user. In either case,
      //  we take no further action here, as the selected directory
      //  will have been printed by qcd_match.
      }
    else
      {
      // The argument was neither a valid directory, nor found in the
      //  list, even after prompting the user. Just echo it -- there's
      //  nothing more we can do here.
      printf ("%s\n", orig_dir);
      }
    }
  else
    {
    fprintf (stderr, "cd: too many arguments\n");
    printf (".\n");
    }
  
  if (db_path) kpath_destroy (db_path);
  exit (0); 
  }


