attomail - minimal MDA with Maildir support
============================================

attomail is a minimal Mail Delivery Agent (MDA) for local mail. Mail is
accepted from standard input and placed in a Maildir box of a user. This
software is intended for use on a single-user machine.

Remote delivery, daemonizing, sender verification, etc. is not implemented and
won't be implemented due to its complexity. attomail is not written because
mail software did not exist, but because existing software were too large for
the simple task of [delivering cron mail to the local user][1].

The workflow of attomail:

 1. Create a new file with a [unique filename][2] in the mail directory.
 2. Write a `Received` header to the file. If `From` and `Date` headers are
    missing, then they will also be appended to the headers.
 3. Pass data from standard input to the file.
 4. Exit.

attomail can replace the sendmail binary, but note that delivery is only
possible for a single user. When invoked as `newaliases` or `mailq`, the program
exits with a zero status code. Most [options of sendmail][3] are ignored except
for the `-fname` and `address` arguments. Only the `-bm` mode (read from stdin and
deliver the usual way) is supported, attomail will exit in other modes.


Installation
------------
The user to deliver mail to has to be specified at compile time:

    make USERNAME=peter attomail

By default, the Maildir directory is `~/Maildir`. It can
be changed to `~/.Maildir/inbox` as follows:

    make USERNAME=peter MAILBOX_PATH=.Maildir/inbox attomail

Absolute paths are also supported. The following configuration will put mail in
`/var/mail/new/(filename)`:

    make USERNAME=nobody MAILBOX_PATH=/var/mail attomail

To install attomail on your system with the appropriate capabilities:

    # Copy attomail binary to its destination
    # Make sure attomail is owned by the correct user (i.e. the same as used for USERNAME variable while building it)
    chmod 4111 attomail

Note: The attomail binary must be installed with setuid enabled. Else, it won't work.


Usage
-----
If you do not have appropriate privileges to install attomail (you are not
root) or if you want to try it out before installing, then you can specify
the program as sendmail program for the `mail` program (from
[`heirloom-mailx`][2]).

Example (assuming that `attomail` is built and available in the current working
directory):

    echo Testing... | mail -S sendmail=attomail -s Subject peter


Bugs
----
Not much validation is done for the address specified with the `-f` option or
the recipient address. The mail body is passed unprocessed. If attomail is
invoked without specifying mail contents, an empty message will be created. If
the mail does not contain headers, `Date` and `From` headers will be appended
anyway.

Other bugs can be reported in the "Issues" of this Github project.


Copyright
---------
Copyright (c) 2013-2015 Peter Wu &lt;peter@lekensteyn.nl&gt;

License: GNU GPL version 3 or later &lt;http://gnu.org/licenses/gpl.html&gt;.
This is free software: you are free to change and redistribute it. There is NO
WARRANTY, to the extent permitted by law.


 [1]: http://unix.stackexchange.com/q/82093/8250
 [2]: http://heirloom.sourceforge.net/mailx.html
 [3]: http://www.sendmail.org/~ca/email/man/sendmail.html
