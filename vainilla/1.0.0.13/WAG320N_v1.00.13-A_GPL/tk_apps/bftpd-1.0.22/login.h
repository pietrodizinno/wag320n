#ifndef _LOGIN_H_
#define _LOGIN_H_

#include <config.h>
#include <pwd.h>
#ifdef USE_PAM
  #include <security/pam_appl.h>
extern pam_handle_t *pamh;
#endif

extern struct passwd userinfo;
extern char userinfo_set;

int bftpd_setuid(uid_t uid);
#ifdef HAVE_UTMP_H
void bftpd_logwtmp(char type);
#endif
char *mygetpwuid(int uid, FILE *file, char *name);
int checkuser();
int checkshell();
int mygetpwnam(char *name, FILE *file);
void login_init();
int bftpd_login(char *password);
void login_end();
int checkpass(char *password);
int checkpass_pwd(char *password);
int checkpass_pam(char *password);
void end_pam();
#endif

