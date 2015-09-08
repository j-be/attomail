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
 3. Write a `Received` header to the file. If `From` and `Date` headers are
    missing, then they will also be appended to the headers.
 4. Pass data from standard input to the file.
 5. Exit.

femtomail can replace the sendmail binary, but note that delivery is only
possible for a single user. When invoked as `newaliases` or `mailq`, the program
exits with a zero status code. Most [options of sendmail][3] are ignored except
for the `-fname` and `address` arguments. Only the `-bm` mode (read from stdin and
deliver the usual way) is supported, femtomail will exit in other modes.


Installation
------------
The user to deliver mail to has to be specified at compile time:

    make USERNAME=peter

By default, the Maildir directory is `~/.local/share/local-mail/inbox`. It can
be changed to `~/.Maildir/inbox` as follows:

    make USERNAME=peter MAILBOX_PATH=.Maildir/inbox

Absolute paths are also supported. The following configuration will put mail in
`/var/mail/new/(filename)`:

    make USERNAME=nobody MAILBOX_PATH=/var/mail

To install femtomail on your system with the appropriate capabilities:

    make install install-link-sendmail setcap
    # You must manually create the mailbox if it does not exist yet
    mkdir -p ~/.local/share/local-mail/inbox

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
Not much validation is done for the address specified with the `-f` option or
the recipient address. The mail body is passed unprocessed. If femtomail is
invoked without specifying mail contents, an empty message will be created. If
the mail does not contain headers, `Date` and `From` headers will be appended
anyway.

Other bugs can be reported at &lt;peter@lekensteyn.nl&gt;.


Copyright
---------
Copyright (c) 2013-2015 Peter Wu &lt;peter@lekensteyn.nl&gt;

License: GNU GPL version 3 or later &lt;http://gnu.org/licenses/gpl.html&gt;.
This is free software: you are free to change and redistribute it. There is NO
WARRANTY, to the extent permitted by law.


 [1]: http://unix.stackexchange.com/q/82093/8250
 [2]: http://heirloom.sourceforge.net/mailx.html
 [3]: http://www.sendmail.org/~ca/email/man/sendmail.html
