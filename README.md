# qcd -- a quick-select extension for the bash 'cd' command

`qcd` extends the built-in `cd` command in the bash shell to allow
changing to a previously-used directory based on 
pattern matching. Whenever the user
runs `cd` with a full pathname, the pathname is stored. It's also possible to
store the current directory, however it was arrived at.

Thereafter, any argument to `cd` is matched against the stored list of
directories and, if there is a single match, that directory is selected.
If there are multiple matches, and interactive selector is displayed.

Directories are ranked in the list, so more commonly-used directories
are presented at the top. There's no limit -- apart from efficiency
and storage -- to the number of directories that can be stored.

`qcd` accepts SQL wild-cards in its arguments, so you can enter

  $ cd d%load

and this will match a directory name `/home/bob/download` or 
`/home/bob/Download`.
In fact `load` will also match; but `load` will also match `loader` 
or `reload`.

In practice, the ability to use SQL wild-cards is probably not all
that useful but, as the list of directories is stored in an SQLite3
database, we get this functionality for free. 

`qcd` is by no means the only utility available with this
functionality.  However, `qcd` has the advantage of being completely
self-contained -- it's written in plain C, and has no dependencies
other the standard C library. It does its own console manipulation,
so it doesn't depend on `ncurses` or any other external library. The
SQLite3 database handler is also included in the source, rather than
being included from a library. Too many utilties of this sort
are written using languages that have slow-starting run-time
environments, like Java and Python.

Moreover, `qcd` is written specifically for the bash shell, and
specifically for Linux. It's trivially easy to set up, because of
these restricted aspirations.

The use of SQLite3 to store the list of directories, rather than
just an ordinary text file, makes searching the list very fast, even
when it is long. 

## How it works 

To activate `qcd` execute the following bash script in the current
bash session. 

    cd()
      {
      CD=`qcd "$@"`
      builtin cd "$CD"
      }

This script replaces the built-in `cd` with `qcd`. The output of
`qcd` -- which will be a directory name -- is passed to the built-in
`cd` function.  Note the double-quotes around the `$CD` environment
variable -- we have to ensure that we don't pass multiple arguments
to the `cd` built-in function if the selected directory name
contains spaces.

The script shown above is installed along with the `qcd` program as
`/usr/bin/qcd_init.sh`. To enable `qcd` 
permanently for a specific user, add

    . /usr/bin/qcd_init.sh

to the user's `.bashrc`.

## Building

`qcd` is designed to be built using `gcc`, for Linux. Whether it works
on other platforms, or with other compilers, is uncertain. Build and
install is the usual:

    $ make
    $ sudo make install

To activate `qcd`, you'll need to arrange for the `qcd_init.sh` script
to be executed, as explained above.

## Command-line options

`cd -a, cd --add`

Add the current working directory to the list of stored directories.
If the directory is already in the list, increase its count.

`cd -d, cd --del`

Delete the current working directory from the list of stored
directory.

`cd -l, cd --list`

Select a directory from the list of stored directories. This list
can also be used to delete stored directories.

`cd --purge`

Delete all stored directories.

## Limitations

It isn't clear whether `qcd` can be made to work with any shell other
than bash. 

`qcd` does its own terminal manipulation, writing ANSI/VT100 escape
codes directly to `/dev/tty`. It might not work properly
with some remote terminal emulators.

`qcd` does not canonicalize directory names or resolve symlinks. So it's
possible to end up with the same directory in the list with different
formats. Perhaps there ought to be an option to canonicalize, but the
directory name stored should probably be the name the user typed, rather
than some other variant

If `qcd` is installed as described, its extended functionality won't take
effect in shell scripts. This is by design -- we really want scripts to be
deterministic. If, for some reason, you really want to extend the function
of `cd` in a script, the script can source `/usr/bin/qcd_init.sh` at the
start.

If the argument to `cd` is the complete name of a
subdirectory of the current
directory, then it will be used as such. The name will not be matched
against stored directories, even if it might match. It would make no sense
to run "cd foo", where "foo" is a subdirectory, and be catapulted off
to some remote part of the filesystem. If you want to `cd` to a 
directory whose name is stored, but which matches a subdirectory, you
can still use a partial name, or use `cd -l` to select from a list.

`qcd` is case-insensitive in its matching -- the argument is matched 
against the stored
directory list without regard for case. However, it's possible to
store directory names that differ only in case.

`qcd` does not impose any limit on the number of directories that
can be stored. However, it becomes decreasingly useful when more than
one screen-full of directories is stored. The full-screen selector
will allow the user to page through a long list of directories but, frankly,
at that point it's probably quicker just to type the name.

`qcd` does not, by itself, interpret `cd` requests for user home directories
(`cd ~bob`). However, the bash built-in `cd` generally does, so this shouldn't
be a problem.

After showing the directory selector, `qcd` tries to restore the original
contents of the console. Not all terminal emulators support this.

## Notes

`qcd` does not provide a way to add arbitrary directories to the stored
list. It stores all directories that are `cd`'d to using a full pathname,
or the current directory by running `cd --add`. If you want to add 
arbitrary directories, you can do this by editing the database directly
using `sqlite3`. There's only one table, and its format is self-explanatory.

The database file is stored at

    $HOME/.qcd.db

Arbitrary directories can be deleted, however -- run `cd -l` to show the
list, and press 'del' to delete.

The directory selector list is displayed in order of popularity, that is,
in order of the number of times the directory has been selected using
`cd`. The actual counts aren't displayed, but you can use `sqlite3` to
see them, if required.

## Author and copyright

`qcd` is maintained by Kevin Boone and distributed under the terms of
the GNU Public Licence v3.0. I wrote it for my own use and I'm making it
available in the hope that it will be useful to other people. However, 
there is no warranty of any kind.

`qcd` contains code from many different authors -- see the individual
source files for details.


