/* 
   Unix SMB/CIFS implementation.

   Winbind daemon for ntdom nss module

   Copyright (C) Tim Potter 2000
   
   You are free to use this interface definition in any way you see
   fit, including without restriction, using this header in your own
   products. You do not need to give any attribution.  
*/


#ifndef CONST_DISCARD
#define CONST_DISCARD(type, ptr)      ((type) ((void *) (ptr)))
#endif

#ifndef CONST_ADD
#define CONST_ADD(type, ptr)          ((type) ((const void *) (ptr)))
#endif

#ifndef SAFE_FREE
#define SAFE_FREE(x) do { if(x) {free(x); x=NULL;} } while(0)
#endif

#ifndef _WINBINDD_NTDOM_H
#define _WINBINDD_NTDOM_H

#define WINBINDD_SOCKET_NAME "pipe"            /* Name of PF_UNIX socket */
#define WINBINDD_SOCKET_DIR  "/tmp/.winbindd"  /* Name of PF_UNIX dir */
#define WINBINDD_PRIV_SOCKET_SUBDIR "winbindd_privileged" /* name of subdirectory of lp_lockdir() to hold the 'privileged' pipe */
#define WINBINDD_DOMAIN_ENV  "WINBINDD_DOMAIN" /* Environment variables */
#define WINBINDD_DONT_ENV    "_NO_WINBINDD"

/* Update this when you change the interface.  */

#define WINBIND_INTERFACE_VERSION 11

/* Socket commands */

enum winbindd_cmd {

	WINBINDD_INTERFACE_VERSION,    /* Always a well known value */

	/* Get users and groups */

	WINBINDD_GETPWNAM,
	WINBINDD_GETPWUID,
	WINBINDD_GETGRNAM,
	WINBINDD_GETGRGID,
	WINBINDD_GETGROUPS,

	/* Enumerate users and groups */

	WINBINDD_SETPWENT,
	WINBINDD_ENDPWENT,
	WINBINDD_GETPWENT,
	WINBINDD_SETGRENT,
	WINBINDD_ENDGRENT,
	WINBINDD_GETGRENT,

	/* PAM authenticate and password change */

	WINBINDD_PAM_AUTH,
	WINBINDD_PAM_AUTH_CRAP,
	WINBINDD_PAM_CHAUTHTOK,

	/* List various things */

	WINBINDD_LIST_USERS,         /* List w/o rid->id mapping */
	WINBINDD_LIST_GROUPS,        /* Ditto */
	WINBINDD_LIST_TRUSTDOM,

	/* SID conversion */

	WINBINDD_LOOKUPSID,
	WINBINDD_LOOKUPNAME,

	/* Lookup functions */

	WINBINDD_SID_TO_UID,       
	WINBINDD_SID_TO_GID,
	WINBINDD_UID_TO_SID,
	WINBINDD_GID_TO_SID,
	WINBINDD_ALLOCATE_RID,
	WINBINDD_ALLOCATE_RID_AND_GID,

	/* Miscellaneous other stuff */

	WINBINDD_CHECK_MACHACC,     /* Check machine account pw works */
	WINBINDD_PING,              /* Just tell me winbind is running */
	WINBINDD_INFO,              /* Various bit of info.  Currently just tidbits */
	WINBINDD_DOMAIN_NAME,       /* The domain this winbind server is a member of (lp_workgroup()) */

	WINBINDD_DOMAIN_INFO,	/* Most of what we know from
				   struct winbindd_domain */
	WINBINDD_GETDCNAME,	/* Issue a GetDCName Request */

	WINBINDD_SHOW_SEQUENCE, /* display sequence numbers of domains */

	/* WINS commands */

	WINBINDD_WINS_BYIP,
	WINBINDD_WINS_BYNAME,

	/* this is like GETGRENT but gives an empty group list */
	WINBINDD_GETGRLST,

	WINBINDD_NETBIOS_NAME,       /* The netbios name of the server */

	/* find the location of our privileged pipe */
	WINBINDD_PRIV_PIPE_DIR,

	/* return a list of group sids for a user sid */
	WINBINDD_GETUSERSIDS,

	/* Return the domain groups a user is in */
	WINBINDD_GETUSERDOMGROUPS,

	/* Initialize connection in a child */
	WINBINDD_INIT_CONNECTION,

	/* Blocking calls that are not allowed on the main winbind pipe, only
	 * between parent and children */
	WINBINDD_DUAL_SID2UID,
	WINBINDD_DUAL_SID2GID,
	WINBINDD_DUAL_IDMAPSET,

	/* Wrapper around possibly blocking unix nss calls */
	WINBINDD_DUAL_UID2NAME,
	WINBINDD_DUAL_NAME2UID,
	WINBINDD_DUAL_GID2NAME,
	WINBINDD_DUAL_NAME2GID,

	WINBINDD_DUAL_USERINFO,
	WINBINDD_DUAL_GETSIDALIASES,

	WINBINDD_NUM_CMDS
};

typedef struct winbindd_pw {
	fstring pw_name;
	fstring pw_passwd;
	uid_t pw_uid;
	gid_t pw_gid;
	fstring pw_gecos;
	fstring pw_dir;
	fstring pw_shell;
} WINBINDD_PW;


typedef struct winbindd_gr {
	fstring gr_name;
	fstring gr_passwd;
	gid_t gr_gid;
	size_t num_gr_mem;
	size_t gr_mem_ofs;   /* offset to group membership */
	char **gr_mem;
} WINBINDD_GR;


#define WBFLAG_PAM_INFO3_NDR  		0x0001
#define WBFLAG_PAM_INFO3_TEXT 		0x0002
#define WBFLAG_PAM_USER_SESSION_KEY     0x0004
#define WBFLAG_PAM_LMKEY      		0x0008
#define WBFLAG_PAM_CONTACT_TRUSTDOM 	0x0010
#define WBFLAG_QUERY_ONLY		0x0020
#define WBFLAG_ALLOCATE_RID		0x0040
#define WBFLAG_PAM_UNIX_NAME            0x0080
#define WBFLAG_PAM_AFS_TOKEN            0x0100
#define WBFLAG_PAM_NT_STATUS_SQUASH     0x0200

/* This is a flag that can only be sent from parent to child */
#define WBFLAG_IS_PRIVILEGED            0x0400
/* Flag to say this is a winbindd internal send - don't recurse. */
#define WBFLAG_RECURSE			0x0800

#define WINBINDD_MAX_EXTRA_DATA (128*1024)

/* Winbind request structure */

struct winbindd_request {
	uint32 length;
	enum winbindd_cmd cmd;   /* Winbindd command to execute */
	pid_t pid;               /* pid of calling process */
	uint32 flags;            /* flags relavant to a given request */
	fstring domain_name;	/* name of domain for which the request applies */

	union {
		fstring winsreq;     /* WINS request */
		fstring username;    /* getpwnam */
		fstring groupname;   /* getgrnam */
		uid_t uid;           /* getpwuid, uid_to_sid */
		gid_t gid;           /* getgrgid, gid_to_sid */
		struct {
			/* We deliberatedly don't split into domain/user to
                           avoid having the client know what the separator
                           character is. */	
			fstring user;
			fstring pass;
		        fstring require_membership_of_sid;
		} auth;              /* pam_winbind auth module */
                struct {
                        unsigned char chal[8];
			uint32 logon_parameters;
                        fstring user;
                        fstring domain;
                        fstring lm_resp;
                        uint16 lm_resp_len;
                        fstring nt_resp;
                        uint16 nt_resp_len;
			fstring workstation;
		        fstring require_membership_of_sid;
                } auth_crap;
                struct {
                    fstring user;
                    fstring oldpass;
                    fstring newpass;
                } chauthtok;         /* pam_winbind passwd module */
		fstring sid;         /* lookupsid, sid_to_[ug]id */
		struct {
			fstring dom_name;       /* lookupname */
			fstring name;       
		} name;
		uint32 num_entries;  /* getpwent, getgrent */
		struct {
			fstring username;
			fstring groupname;
		} acct_mgt;
		struct {
			BOOL is_primary;
			fstring dcname;
		} init_conn;
		struct {
			fstring sid;
			fstring name;
			BOOL alloc;
		} dual_sid2id;
		struct {
			int type;
			uid_t uid;
			gid_t gid;
			fstring sid;
		} dual_idmapset;
	} data;
	char *extra_data;
	size_t extra_len;
	char null_term;
};

/* Response values */

enum winbindd_result {
	WINBINDD_ERROR,
	WINBINDD_PENDING,
	WINBINDD_OK
};

/* Winbind response structure */

struct winbindd_response {
    
	/* Header information */

	uint32 length;                        /* Length of response */
	enum winbindd_result result;          /* Result code */

	/* Fixed length return data */
	
	union {
		int interface_version;  /* Try to ensure this is always in the same spot... */
		
		fstring winsresp;		/* WINS response */

		/* getpwnam, getpwuid */
		
		struct winbindd_pw pw;

		/* getgrnam, getgrgid */

		struct winbindd_gr gr;

		uint32 num_entries; /* getpwent, getgrent */
		struct winbindd_sid {
			fstring sid;        /* lookupname, [ug]id_to_sid */
			int type;
		} sid;
		struct winbindd_name {
			fstring dom_name;       /* lookupsid */
			fstring name;       
			int type;
		} name;
		uid_t uid;          /* sid_to_uid */
		gid_t gid;          /* sid_to_gid */
		struct winbindd_info {
			char winbind_separator;
			fstring samba_version;
		} info;
		fstring domain_name;
		fstring netbios_name;
		fstring dc_name;

		struct auth_reply {
			uint32 nt_status;
			fstring nt_status_string;
			fstring error_string;
			int pam_error;
			char user_session_key[16];
			char first_8_lm_hash[8];
		} auth;
		uint32 rid;	/* create user or group or allocate rid */
		struct {
			uint32 rid;
			gid_t gid;
		} rid_and_gid;
		struct {
			fstring name;
			fstring alt_name;
			fstring sid;
			BOOL native_mode;
			BOOL active_directory;
			BOOL primary;
			uint32 sequence_number;
		} domain_info;
		struct {
			fstring acct_name;
			fstring full_name;
			fstring homedir;
			fstring shell;
			uint32 group_rid;
		} user_info;
	} data;

	/* Variable length return data */

	void *extra_data;               /* getgrnam, getgrgid, getgrent */
};

#endif
