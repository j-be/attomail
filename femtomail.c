/**
 * femtomail - Minimal sendmail replacement for forwarding mail to a single
 * Maildir box.
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

/* Maildir; either absolute (starting with a forward slash) or
 * a directory relative to home dir of USERNAME (see above) */
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

    if ((MAILBOX_PATH)[0] == '/') {
        snprintf(maildir, maildir_len, "%s/new", MAILBOX_PATH);
    } else {
        snprintf(maildir, maildir_len, "%s/%s/new", pwd->pw_dir, MAILBOX_PATH);
    }
    return 0;
}

char *
xstrdup(char *str) {
    char *s;

    s = strdup(str);
    if (!s)
        abort();

    return s;
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

void
get_timestr(const time_t *tp, char *timestr, size_t timestr_len) {
    struct tm *tmp;

    tmp = localtime(tp);
    if (tmp == NULL) {
        perror("localtime");
        return;
    }

    if (!strftime(timestr, timestr_len, "%a, %d %b %Y %T %z", tmp)) {
        fprintf(stderr, "strftime() failed!\n");
    }
}

/* test whether a line contains a header */
bool
is_header(const char *line, const char *hdr) {
    size_t hdr_len = strlen(hdr);

    while (isspace(*line))
        line++;

    if (strncasecmp(line, hdr, hdr_len)) {
        return false;
    }
    line += hdr_len;

    while (isspace(*line))
        line++;

    return *line == ':';
}

/* write Received header to file and append Date/From if missing */
int
handle_headers(FILE *mail_fp, const char *from_addr, const char *to_addr, time_t tm) {
    bool line_cont = false; /* line continuation or new line? */
    char timestr[200], buf[4096];
    bool has_from = false, has_date = false;

    get_timestr(&tm, timestr, sizeof(timestr));

    /* always begin with a Received header */
    fprintf(mail_fp, "Received: for %s with local (femtomail)", to_addr);
    if (from_addr != NULL) {
        fprintf(mail_fp, " (envelope-from %s)", from_addr);
    }
    fprintf(mail_fp, "; %s\n", timestr);

    /* process headers from input */
    while (fgets(buf, sizeof(buf), stdin) != NULL) {
        if (!line_cont) {
            if (is_header(buf, "From")) has_from = true;
            if (is_header(buf, "Date")) has_date = true;
            if (!strcmp(buf, "\n") || !strcmp(buf, "\r\n")) {
                break; /* end of headers */
            }
        }
        /* if line does not end with LF, then line is longer than buffer */
        line_cont = buf[strlen(buf) - 1] != '\n';

        if (fputs(buf, mail_fp) == EOF) {
            return 1;
        }
    }

    if (!has_date) {
        fprintf(mail_fp, "Date: %s\n", timestr);
    }
    if (!has_from) {
        fprintf(mail_fp, "From: %s\n", from_addr);
    }

    /* insert end of headers */
    fputc('\n', mail_fp);

    return 0;
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
    char *from_address = NULL, *to_address = NULL;
    char file_path[256];
    time_t tm;
    int opt, mail_fd, ret;
    FILE *mail_fp;

    if (argc > 0) {
        char *prog_name = basename(argv[0]);
        if (!prog_name) {
            fprintf(stderr, "argv[0] may not be NULL!\n");
            return (EXIT_FAILURE);
        }

        if (!strcmp(prog_name, "newaliases") || !strcmp(prog_name, "mailq")) {
            /* ignore program */
            return (EXIT_SUCCESS);
        }
    }

    while ((opt = getopt(argc, argv, "+f:B:b:C:F:h:iN:nO:o:p:q:R:r:tUV:v:X:")) != -1) {
        switch (opt) {
        case 'f':
            from_address = xstrdup(optarg);
            break;
        case 'b':
            if (*optarg != 'm') {
                /* ignore modes other than "read mail from stdin" */
                return (EXIT_SUCCESS);
            }
            break;
        case '?': /* unrecognized argument */
            return (EXIT_FAILURE);
        default:
            /* ignore option */
            break;
        }
    }

    if (optind < argc) {
        if (!valid_address(argv[optind])) {
            fprintf(stderr, "Illegal characters in address!\n");
            return (EXIT_FAILURE);
        }

        to_address = xstrdup(argv[optind]);
    }

    if (!to_address) {
        fprintf(stderr, "Missing recipient address.\n");
        return (EXIT_FAILURE);
    }

    if (!from_address) {
        struct passwd *pwd = getpwuid(getuid());
        if (pwd) {
            from_address = xstrdup(pwd->pw_name);
        }
    }

    if (from_address) {
        if (!valid_address(from_address)) {
            fprintf(stderr, "Illegal characters in From address!\n");
            return (EXIT_FAILURE);
        }
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

    ret = handle_headers(mail_fp, from_address, to_address, tm);
    if (ret == 0) {
        ret = read_and_write(mail_fp);
    }
    if (ret) {
            perror("fwrite");
    }

    fclose(mail_fp);
    free(to_address);
    free(from_address);
    return ret;
}

/* vim: set sw=4 ts=4 et: */
