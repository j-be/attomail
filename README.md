femtomail - minimal MDA with Maildir support
============================================

femtomail is a minimal Mail Delivery Agent (MDA) for local mail. Mail is
accepted from standard input and placed in a Maildir box of a user. This
software is intended for use on a single-user machine.

Remote delivery, daemonizing, sender verification, etc. is not implemented and
won't be implemented due to its complexity. femtomail is not written because
mail software did not exist, but because existing software were too large for
the simple task of [delivering cron mail to the local user][1].

The workflow of femtomail:

 1. Change the process user and group.
 2. Create a new file with a [unique filename][2] in the mail directory.
 3. Write a `Received` header to the file.
 4. Pass data from standard input to the file.
 5. Exit.


Installation
------------
The user to deliver mail to has to be specified at compile time:

    make USERNAME=peter

By default, the Maildir directory is `~/.local/share/local-mail/inbox`. It can
be changed to `~/.Maildir/inbox` as follows:

    make USERNAME=peter MAILBOX_PATH=.Maildir/inbox

To install femtomail on your system with the appropriate capabilities:

    make install install-link-sendmail setcap

Note: the femtomail binary must be installed with file capabilities set
(recommended). Alternatively, the program can run with setuid-root. Either way,
the user and groups are changed before the mail is read and written.


Usage
-----
If you do not have appropriate privileges to install femtomail (you are not
root) or if you want to try it out before installing, then you can specify
the program as sendmail program for the `mail` program (from
[`heirloom-mailx`][2]).

Example (assuming that `femtomail` is built and available in the current working
directory):

    echo Testing... | mail -S sendmail=femtomail -s Subject peter


Bugs
----
This program does not parse any sendmail option. All arguments are ignored,
except the (optional) first address (which is written in the `Received` mail
header). No validation is performed at this address, the mail headers and its
body. If femtomail is invoked without specifying mail contents, an empty message
will be created.

Other bugs can be reported at <lekensteyn@gmail.com>.


Copyright
---------
Copyright (c) 2013 Peter Wu <lekensteyn@gmail.com>

License: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.  This is
free software: you are free to change and redistribute it. There is NO WARRANTY,
to the extent permitted by law.


 [1]: http://unix.stackexchange.com/q/82093/8250
 [2]: http://heirloom.sourceforge.net/mailx.html
