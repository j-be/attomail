/**
 * femtomail - Minimal sendmail replacement for forwarding mail to a single
 * Maildir box. Note: this program does not try to implement sendmail.
 *
 * Installation commands:
 * $ cc -DUSERNAME=\"$USER\" femtomail.c -o femtomail
 * (Optional override: -DMAILBOX_PATH=\".Maildir/inbox\")
 * # install -m 755 femtomail /usr/bin/sendmail
 * # setcap cap_setuid,cap_setgid=ep /usr/bin/sendmail
 *
 * Copyright (C) 2013 Peter Wu <lekensteyn@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define _GNU_SOURCE /* for setresgid/setresuid */
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/prctl.h>

#ifndef USERNAME
#    error Please define the user to deliver mail to with USERNAME
#endif

/* Maildir directory relative to home dir of USERNAME (see above) */
#ifndef MAILBOX_PATH
#    define MAILBOX_PATH ".local/share/local-mail/inbox"
#endif

/* change user/group context to username and fill in maildir path */
int
init_user(const char *username, char *maildir, size_t maildir_len) {
    struct passwd *pwd;
    uid_t uid;
    gid_t gid;

    if ((pwd = getpwnam(username)) == NULL) {
        fprintf(stderr, "Unknown user %s, cannot locate Maildir.\n", username);
        return 1;
    }

    uid = pwd->pw_uid;
    gid = pwd->pw_gid;

    if (setresgid(gid, gid, gid) || setresuid(uid, uid, uid)) {
        perror("Failed to change uid/gid");
        return 1;
    }

    snprintf(maildir, maildir_len, "%s/" MAILBOX_PATH "/new", pwd->pw_dir);
    return 0;
}

/* get a random number between 0 and 999 (inclusive) */
unsigned
get_random(void) {
    FILE *fp;
    int rnd;

    fp = fopen("/dev/urandom", "r");
    if (fp != NULL) {
        unsigned char buf[2];

        fread(buf, 2, 1, fp);
        rnd = (buf[0] << 8) | buf[1];

        fclose(fp);
    } else {
        rnd = rand();
    }

    return rnd % 1000;
}

/* get the current hostname, sanitizing '/' and ':' chars  */
void
get_hostname(char *hostname, size_t hostname_len) {
    if (gethostname(hostname, hostname_len) >= 0) {
        char *p;
        /* sanitized per http://cr.yp.to/proto/maildir.html */
        for (p = hostname; *p; p++) {
            if (*p == '/') *p = '\057';
            if (*p == ':') *p = '\072';
        }
    } else {
        strncpy(hostname, "unknown", hostname_len);
    }
}

/* generate a filename suitable for Maildir consisting of a timestamp, a random
 * middle part and a hostname (per http://cr.yp.to/proto/maildir.html) */
void
make_name(char *name, size_t name_len, time_t tm) {
    char hostname[128];

    get_hostname(hostname, sizeof(hostname));
    snprintf(name, name_len, "%llu.R%u.%s",
        (unsigned long long) tm, get_random(), hostname);
}

/* write Received header to file */
void
write_headers(FILE *mail_fp, const char *address, time_t tm) {
    char timestr[200];
    struct tm *tmp;

    tmp = localtime(&tm);
    if (tmp == NULL) {
        perror("localtime");
        return;
    }

    if (!strftime(timestr, sizeof(timestr), "%a, %d %b %Y %T %z", tmp)) {
        fprintf(stderr, "strftime() failed!\n");
        return;
    }

    fprintf(mail_fp, "Received: for %s with local (femtomail); %s\n", address, timestr);
}

/* read text from stdin and write to file */
int
read_and_write(FILE *mail_fp) {
    size_t r;
    char buf[4096];

    while ((r = fread(buf, 1, sizeof(buf), stdin)) > 0) {
        if (fwrite(buf, 1, r, mail_fp) != r) {
            /* disk full? */
            return 1;
        }
    }

    return 0;
}

/* validate the recipient address as in `sendmail [address]` */
bool
valid_address(const char *address) {
    char c;

    while ((c = *address++) != 0) {
        /* reject unprintable chars, including newlines */
        if (!isgraph(c)) {
            return false;
        }
    }

    return true;
}

int
main(int argc, char **argv) {
    char maildir[256];
    char *address = NULL, file_path[256];
    time_t tm;
    int i, mail_fd, ret;
    FILE *mail_fp;

    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            /* ignore arg, hopefully next arg does not contain a value */
            continue;
        }

        if (!valid_address(argv[i])) {
            fprintf(stderr, "Illegal characters in address!\n");
            return (EXIT_FAILURE);
        }

        address = argv[i];
        break;
    }

    if (!address) {
        fprintf(stderr, "Missing recipient address.\n");
        return (EXIT_FAILURE);
    }

    if (init_user((USERNAME), maildir, sizeof(maildir))) {
        return (EXIT_FAILURE);
    }

    if (chdir(maildir)) {
        fprintf(stderr, "chdir(%s): %s\n", maildir, strerror(errno));
        return (EXIT_FAILURE);
    }

    tm = time(NULL);
    make_name(file_path, sizeof(file_path), tm);

    /* ensure that no file gets overwritten */
    mail_fd = open(file_path, O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (mail_fd < 0) {
        perror("open");
        return (EXIT_FAILURE);
    }

    mail_fp = fdopen(mail_fd, "w");

    write_headers(mail_fp, address, tm);
    ret = read_and_write(mail_fp);
    if (ret) {
            perror("fwrite");
    }

    fclose(mail_fp);
    return ret;
}

/* vim: set sw=4 ts=4 et: */
