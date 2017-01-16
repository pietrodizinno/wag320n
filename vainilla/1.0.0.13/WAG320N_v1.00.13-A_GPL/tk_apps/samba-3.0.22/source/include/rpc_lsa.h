/* 
   Unix SMB/CIFS implementation.
   SMB parameters and setup
   Copyright (C) Andrew Tridgell               1992-1997
   Copyright (C) Luke Kenneth Casson Leighton  1996-1997
   Copyright (C) Paul Ashton                   1997
   Copyright (C) Gerald (Jerry) Carter         2005
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef _RPC_LSA_H /* _RPC_LSA_H */
#define _RPC_LSA_H 

/* Opcodes available on PIPE_LSARPC */

#if 0	/* UNIMPLEMENTED */

#define LSA_LOOKUPSIDS2		0x39

#endif

#define LSA_CLOSE              0x00
#define LSA_DELETE             0x01
#define LSA_ENUM_PRIVS         0x02
#define LSA_QUERYSECOBJ        0x03
#define LSA_SETSECOBJ          0x04
#define LSA_CHANGEPASSWORD     0x05
#define LSA_OPENPOLICY         0x06
#define LSA_QUERYINFOPOLICY    0x07
#define LSA_SETINFOPOLICY      0x08
#define LSA_CLEARAUDITLOG      0x09
#define LSA_CREATEACCOUNT      0x0a
#define LSA_ENUM_ACCOUNTS      0x0b
#define LSA_CREATETRUSTDOM     0x0c	/* TODO: implement this one  -- jerry */
#define LSA_ENUMTRUSTDOM       0x0d
#define LSA_LOOKUPNAMES        0x0e
#define LSA_LOOKUPSIDS         0x0f
#define LSA_CREATESECRET       0x10	/* TODO: implement this one  -- jerry */
#define LSA_OPENACCOUNT	       0x11
#define LSA_ENUMPRIVSACCOUNT   0x12
#define LSA_ADDPRIVS           0x13
#define LSA_REMOVEPRIVS        0x14
#define LSA_GETQUOTAS          0x15
#define LSA_SETQUOTAS          0x16
#define LSA_GETSYSTEMACCOUNT   0x17
#define LSA_SETSYSTEMACCOUNT   0x18
#define LSA_OPENTRUSTDOM       0x19	/* TODO: implement this one  -- jerry */
#define LSA_QUERYTRUSTDOMINFO  0x1a
#define LSA_SETINFOTRUSTDOM    0x1b
#define LSA_OPENSECRET         0x1c	/* TODO: implement this one  -- jerry */
#define LSA_SETSECRET          0x1d	/* TODO: implement this one  -- jerry */
#define LSA_QUERYSECRET        0x1e
#define LSA_LOOKUPPRIVVALUE    0x1f
#define LSA_LOOKUPPRIVNAME     0x20
#define LSA_PRIV_GET_DISPNAME  0x21
#define LSA_DELETEOBJECT       0x22	/* TODO: implement this one  -- jerry */
#define LSA_ENUMACCTWITHRIGHT  0x23	/* TODO: implement this one  -- jerry */
#define LSA_ENUMACCTRIGHTS     0x24
#define LSA_ADDACCTRIGHTS      0x25
#define LSA_REMOVEACCTRIGHTS   0x26
#define LSA_QUERYTRUSTDOMINFOBYSID  0x27
#define LSA_SETTRUSTDOMINFO    0x28
#define LSA_DELETETRUSTDOM     0x29
#define LSA_STOREPRIVDATA      0x2a
#define LSA_RETRPRIVDATA       0x2b
#define LSA_OPENPOLICY2        0x2c
#define LSA_UNK_GET_CONNUSER   0x2d /* LsaGetConnectedCredentials ? */
#define LSA_QUERYINFO2         0x2e
#define LSA_QUERYTRUSTDOMINFOBYNAME 0x30
#define LSA_OPENTRUSTDOMBYNAME 0x37

/* XXXX these are here to get a compile! */
#define LSA_LOOKUPRIDS      0xFD

/* DOM_QUERY - info class 3 and 5 LSA Query response */
typedef struct dom_query_info
{
  uint16 uni_dom_max_len; /* domain name string length * 2 */
  uint16 uni_dom_str_len; /* domain name string length * 2 */
  uint32 buffer_dom_name; /* undocumented domain name string buffer pointer */
  uint32 buffer_dom_sid; /* undocumented domain SID string buffer pointer */
  UNISTR2 uni_domain_name; /* domain name (unicode string) */
  DOM_SID2 dom_sid; /* domain SID */

} DOM_QUERY;

/* level 5 is same as level 3. */
typedef DOM_QUERY DOM_QUERY_3;
typedef DOM_QUERY DOM_QUERY_5;

/* level 2 is auditing settings */
typedef struct dom_query_2
{
	uint32 auditing_enabled;
	uint32 count1; /* usualy 7, at least on nt4sp4 */
	uint32 count2; /* the same */
	uint32 *auditsettings;
} DOM_QUERY_2;

/* level 6 is server role information */
typedef struct dom_query_6
{
	uint16 server_role; /* 2=backup, 3=primary */
} DOM_QUERY_6;

typedef struct seq_qos_info
{
	uint32 len; /* 12 */
	uint16 sec_imp_level; /* 0x02 - impersonation level */
	uint8  sec_ctxt_mode; /* 0x01 - context tracking mode */
	uint8  effective_only; /* 0x00 - effective only */

} LSA_SEC_QOS;

typedef struct obj_attr_info
{
	uint32 len;          /* 0x18 - length (in bytes) inc. the length field. */
	uint32 ptr_root_dir; /* 0 - root directory (pointer) */
	uint32 ptr_obj_name; /* 0 - object name (pointer) */
	uint32 attributes;   /* 0 - attributes (undocumented) */
	uint32 ptr_sec_desc; /* 0 - security descriptior (pointer) */
	uint32 ptr_sec_qos;  /* security quality of service */
	LSA_SEC_QOS *sec_qos;

} LSA_OBJ_ATTR;

/* LSA_Q_OPEN_POL - LSA Query Open Policy */
typedef struct lsa_q_open_pol_info
{
	uint32 ptr;         /* undocumented buffer pointer */
	uint16 system_name; /* 0x5c - system name */
	LSA_OBJ_ATTR attr ; /* object attributes */

	uint32 des_access; /* desired access attributes */

} LSA_Q_OPEN_POL;

/* LSA_R_OPEN_POL - response to LSA Open Policy */
typedef struct lsa_r_open_pol_info
{
	POLICY_HND pol; /* policy handle */
	NTSTATUS status; /* return code */

} LSA_R_OPEN_POL;

/* LSA_Q_OPEN_POL2 - LSA Query Open Policy */
typedef struct lsa_q_open_pol2_info
{
	uint32       ptr;             /* undocumented buffer pointer */
	UNISTR2      uni_server_name; /* server name, starting with two '\'s */
	LSA_OBJ_ATTR attr           ; /* object attributes */

	uint32 des_access; /* desired access attributes */

} LSA_Q_OPEN_POL2;

/* LSA_R_OPEN_POL2 - response to LSA Open Policy */
typedef struct lsa_r_open_pol2_info
{
	POLICY_HND pol; /* policy handle */
	NTSTATUS status; /* return code */

} LSA_R_OPEN_POL2;


#define POLICY_VIEW_LOCAL_INFORMATION    0x00000001
#define POLICY_VIEW_AUDIT_INFORMATION    0x00000002
#define POLICY_GET_PRIVATE_INFORMATION   0x00000004
#define POLICY_TRUST_ADMIN               0x00000008
#define POLICY_CREATE_ACCOUNT            0x00000010
#define POLICY_CREATE_SECRET             0x00000020
#define POLICY_CREATE_PRIVILEGE          0x00000040
#define POLICY_SET_DEFAULT_QUOTA_LIMITS  0x00000080
#define POLICY_SET_AUDIT_REQUIREMENTS    0x00000100
#define POLICY_AUDIT_LOG_ADMIN           0x00000200
#define POLICY_SERVER_ADMIN              0x00000400
#define POLICY_LOOKUP_NAMES              0x00000800

#define POLICY_ALL_ACCESS ( STANDARD_RIGHTS_REQUIRED_ACCESS  |\
                            POLICY_VIEW_LOCAL_INFORMATION    |\
                            POLICY_VIEW_AUDIT_INFORMATION    |\
                            POLICY_GET_PRIVATE_INFORMATION   |\
                            POLICY_TRUST_ADMIN               |\
                            POLICY_CREATE_ACCOUNT            |\
                            POLICY_CREATE_SECRET             |\
                            POLICY_CREATE_PRIVILEGE          |\
                            POLICY_SET_DEFAULT_QUOTA_LIMITS  |\
                            POLICY_SET_AUDIT_REQUIREMENTS    |\
                            POLICY_AUDIT_LOG_ADMIN           |\
                            POLICY_SERVER_ADMIN              |\
                            POLICY_LOOKUP_NAMES )


#define POLICY_READ       ( STANDARD_RIGHTS_READ_ACCESS      |\
                            POLICY_VIEW_AUDIT_INFORMATION    |\
                            POLICY_GET_PRIVATE_INFORMATION)

#define POLICY_WRITE      ( STD_RIGHT_READ_CONTROL_ACCESS     |\
                            POLICY_TRUST_ADMIN               |\
                            POLICY_CREATE_ACCOUNT            |\
                            POLICY_CREATE_SECRET             |\
                            POLICY_CREATE_PRIVILEGE          |\
                            POLICY_SET_DEFAULT_QUOTA_LIMITS  |\
                            POLICY_SET_AUDIT_REQUIREMENTS    |\
                            POLICY_AUDIT_LOG_ADMIN           |\
                            POLICY_SERVER_ADMIN)

#define POLICY_EXECUTE    ( STANDARD_RIGHTS_EXECUTE_ACCESS   |\
                            POLICY_VIEW_LOCAL_INFORMATION    |\
                            POLICY_LOOKUP_NAMES )

/* LSA_Q_QUERY_SEC_OBJ - LSA query security */
typedef struct lsa_query_sec_obj_info
{
	POLICY_HND pol; /* policy handle */
	uint32 sec_info;

} LSA_Q_QUERY_SEC_OBJ;

/* LSA_R_QUERY_SEC_OBJ - probably an open */
typedef struct r_lsa_query_sec_obj_info
{
	uint32 ptr;
	SEC_DESC_BUF *buf;

	NTSTATUS status;         /* return status */

} LSA_R_QUERY_SEC_OBJ;

/* LSA_Q_QUERY_INFO - LSA query info policy */
typedef struct lsa_query_info
{
	POLICY_HND pol; /* policy handle */
    uint16 info_class; /* info class */

} LSA_Q_QUERY_INFO;

/* LSA_INFO_UNION */
typedef union lsa_info_union
{
	DOM_QUERY_2 id2;
	DOM_QUERY_3 id3;
	DOM_QUERY_5 id5;
	DOM_QUERY_6 id6;
} LSA_INFO_UNION;

/* LSA_R_QUERY_INFO - response to LSA query info policy */
typedef struct lsa_r_query_info
{
    uint32 undoc_buffer; /* undocumented buffer pointer */
    uint16 info_class; /* info class (same as info class in request) */
   
	LSA_INFO_UNION dom; 

	NTSTATUS status; /* return code */

} LSA_R_QUERY_INFO;

/* LSA_DNS_DOM_INFO - DNS domain info - info class 12*/
typedef struct lsa_dns_dom_info
{
	UNIHDR  hdr_nb_dom_name; /* netbios domain name */
	UNIHDR  hdr_dns_dom_name;
	UNIHDR  hdr_forest_name;

	struct uuid dom_guid; /* domain GUID */

	UNISTR2 uni_nb_dom_name;
	UNISTR2 uni_dns_dom_name;
	UNISTR2 uni_forest_name;

	uint32 ptr_dom_sid;
	DOM_SID2   dom_sid; /* domain SID */
} LSA_DNS_DOM_INFO;

typedef union lsa_info2_union
{
	LSA_DNS_DOM_INFO dns_dom_info;
} LSA_INFO2_UNION;

/* LSA_Q_QUERY_INFO2 - LSA query info */
typedef struct lsa_q_query_info2
{
	POLICY_HND pol;    /* policy handle */
	uint16 info_class; /* info class */
} LSA_Q_QUERY_INFO2;

typedef struct lsa_r_query_info2
{
	uint32 ptr;    /* pointer to info struct */
	uint16 info_class;
	LSA_INFO2_UNION info; /* so far the only one */
	NTSTATUS status;
} LSA_R_QUERY_INFO2;

/*******************************************************/

typedef struct {
	POLICY_HND pol; 
	uint32 enum_context; 
	uint32 preferred_len; 	/* preferred maximum length */
} LSA_Q_ENUM_TRUST_DOM;

typedef struct {
	UNISTR4	name;
	DOM_SID2 *sid;
} DOMAIN_INFO;

typedef struct {
	uint32 count;
	DOMAIN_INFO *domains;
} DOMAIN_LIST;

typedef struct {
	uint32 enum_context;
	uint32 count;
	DOMAIN_LIST *domlist;
	NTSTATUS status; 
} LSA_R_ENUM_TRUST_DOM;

/*******************************************************/

/* LSA_Q_CLOSE */
typedef struct lsa_q_close_info
{
	POLICY_HND pol; /* policy handle */

} LSA_Q_CLOSE;

/* LSA_R_CLOSE */
typedef struct lsa_r_close_info
{
	POLICY_HND pol; /* policy handle.  should be all zeros. */

	NTSTATUS status; /* return code */

} LSA_R_CLOSE;


#define MAX_REF_DOMAINS 32

/* DOM_TRUST_HDR */
typedef struct dom_trust_hdr
{
	UNIHDR hdr_dom_name; /* referenced domain unicode string headers */
	uint32 ptr_dom_sid;

} DOM_TRUST_HDR;
	
/* DOM_TRUST_INFO */
typedef struct dom_trust_info
{
	UNISTR2  uni_dom_name; /* domain name unicode string */
	DOM_SID2 ref_dom     ; /* referenced domain SID */

} DOM_TRUST_INFO;
	
/* DOM_R_REF */
typedef struct dom_ref_info
{
    uint32 num_ref_doms_1; /* num referenced domains */
    uint32 ptr_ref_dom; /* pointer to referenced domains */
    uint32 max_entries; /* 32 - max number of entries */
    uint32 num_ref_doms_2; /* num referenced domains */

    DOM_TRUST_HDR  hdr_ref_dom[MAX_REF_DOMAINS]; /* referenced domains */
    DOM_TRUST_INFO ref_dom    [MAX_REF_DOMAINS]; /* referenced domains */

} DOM_R_REF;

/* the domain_idx points to a SID associated with the name */

/* LSA_TRANS_NAME - translated name */
typedef struct lsa_trans_name_info
{
	uint16 sid_name_use; /* value is 5 for a well-known group; 2 for a domain group; 1 for a user... */
	UNIHDR hdr_name; 
	uint32 domain_idx; /* index into DOM_R_REF array of SIDs */

} LSA_TRANS_NAME;

/* This number is based on Win2k and later maximum response allowed */
#define MAX_LOOKUP_SIDS 20480

/* LSA_TRANS_NAME_ENUM - LSA Translated Name Enumeration container */
typedef struct lsa_trans_name_enum_info
{
	uint32 num_entries;
	uint32 ptr_trans_names;
	uint32 num_entries2;
	
	LSA_TRANS_NAME *name; /* translated names  */
	UNISTR2 *uni_name;

} LSA_TRANS_NAME_ENUM;

/* LSA_SID_ENUM - LSA SID enumeration container */
typedef struct lsa_sid_enum_info
{
	uint32 num_entries;
	uint32 ptr_sid_enum;
	uint32 num_entries2;
	
	uint32 *ptr_sid; /* domain SID pointers to be looked up. */
	DOM_SID2 *sid; /* domain SIDs to be looked up. */

} LSA_SID_ENUM;

/* LSA_Q_LOOKUP_SIDS - LSA Lookup SIDs */
typedef struct lsa_q_lookup_sids
{
	POLICY_HND          pol; /* policy handle */
	LSA_SID_ENUM        sids;
	LSA_TRANS_NAME_ENUM names;
	uint16              level;
	uint32              mapped_count;

} LSA_Q_LOOKUP_SIDS;

/* LSA_R_LOOKUP_SIDS - response to LSA Lookup SIDs */
typedef struct lsa_r_lookup_sids
{
	uint32              ptr_dom_ref;
	DOM_R_REF           *dom_ref; /* domain reference info */

	LSA_TRANS_NAME_ENUM *names;
	uint32              mapped_count;

	NTSTATUS            status; /* return code */

} LSA_R_LOOKUP_SIDS;

/* LSA_Q_LOOKUP_NAMES - LSA Lookup NAMEs */
typedef struct lsa_q_lookup_names
{
	POLICY_HND pol; /* policy handle */
	uint32 num_entries;
	uint32 num_entries2;
	UNIHDR  *hdr_name; /* name buffer pointers */
	UNISTR2 *uni_name; /* names to be looked up */

	uint32 num_trans_entries;
	uint32 ptr_trans_sids; /* undocumented domain SID buffer pointer */
	uint32 lookup_level;
	uint32 mapped_count;

} LSA_Q_LOOKUP_NAMES;

/* LSA_R_LOOKUP_NAMES - response to LSA Lookup NAMEs by name */
typedef struct lsa_r_lookup_names
{
	uint32 ptr_dom_ref;
	DOM_R_REF *dom_ref; /* domain reference info */

	uint32 num_entries;
	uint32 ptr_entries;
	uint32 num_entries2;
	DOM_RID2 *dom_rid; /* domain RIDs being looked up */

	uint32 mapped_count;

	NTSTATUS status; /* return code */
} LSA_R_LOOKUP_NAMES;

typedef struct lsa_enum_priv_entry
{
	UNIHDR hdr_name;
	uint32 luid_low;
	uint32 luid_high;
	UNISTR2 name;
	
} LSA_PRIV_ENTRY;

/* LSA_Q_ENUM_PRIVS - LSA enum privileges */
typedef struct lsa_q_enum_privs
{
	POLICY_HND pol; /* policy handle */
	uint32 enum_context;
	uint32 pref_max_length;
} LSA_Q_ENUM_PRIVS;

typedef struct lsa_r_enum_privs
{
	uint32 enum_context;
	uint32 count;
	uint32 ptr;
	uint32 count1;

	LSA_PRIV_ENTRY *privs;

	NTSTATUS status;
} LSA_R_ENUM_PRIVS;

/* LSA_Q_ENUM_ACCT_RIGHTS - LSA enum account rights */
typedef struct
{
	POLICY_HND pol; /* policy handle */
	DOM_SID2 sid;
} LSA_Q_ENUM_ACCT_RIGHTS;

/* LSA_R_ENUM_ACCT_RIGHTS - LSA enum account rights */
typedef struct
{
	uint32 count;
	UNISTR4_ARRAY *rights;
	NTSTATUS status;
} LSA_R_ENUM_ACCT_RIGHTS;


/* LSA_Q_ADD_ACCT_RIGHTS - LSA add account rights */
typedef struct
{
	POLICY_HND pol; /* policy handle */
	DOM_SID2 sid;
	uint32 count;
	UNISTR4_ARRAY *rights;
} LSA_Q_ADD_ACCT_RIGHTS;

/* LSA_R_ADD_ACCT_RIGHTS - LSA add account rights */
typedef struct
{
	NTSTATUS status;
} LSA_R_ADD_ACCT_RIGHTS;


/* LSA_Q_REMOVE_ACCT_RIGHTS - LSA remove account rights */
typedef struct
{
	POLICY_HND pol; /* policy handle */
	DOM_SID2 sid;
	uint32 removeall;
	uint32 count;
	UNISTR4_ARRAY *rights;
} LSA_Q_REMOVE_ACCT_RIGHTS;

/* LSA_R_REMOVE_ACCT_RIGHTS - LSA remove account rights */
typedef struct
{
	NTSTATUS status;
} LSA_R_REMOVE_ACCT_RIGHTS;


/* LSA_Q_PRIV_GET_DISPNAME - LSA get privilege display name */
typedef struct lsa_q_priv_get_dispname
{
	POLICY_HND pol; /* policy handle */
	UNIHDR hdr_name;
	UNISTR2 name;
	uint16 lang_id;
	uint16 lang_id_sys;
} LSA_Q_PRIV_GET_DISPNAME;

typedef struct lsa_r_priv_get_dispname
{
	uint32 ptr_info;
	UNIHDR hdr_desc;
	UNISTR2 desc;
	/* Don't align ! */
	uint16 lang_id;
	/* align */
	NTSTATUS status;
} LSA_R_PRIV_GET_DISPNAME;

/* LSA_Q_ENUM_ACCOUNTS */
typedef struct lsa_q_enum_accounts
{
	POLICY_HND pol; /* policy handle */
	uint32 enum_context;
	uint32 pref_max_length;
} LSA_Q_ENUM_ACCOUNTS;

/* LSA_R_ENUM_ACCOUNTS */
typedef struct lsa_r_enum_accounts
{
	uint32 enum_context;
	LSA_SID_ENUM sids;
	NTSTATUS status;
} LSA_R_ENUM_ACCOUNTS;

/* LSA_Q_UNK_GET_CONNUSER - gets username\domain of connected user
                  called when "Take Ownership" is clicked -SK */
typedef struct lsa_q_unk_get_connuser
{
  uint32 ptr_srvname;
  UNISTR2 uni2_srvname;
  uint32 unk1; /* 3 unknown uint32's are seen right after uni2_srvname */
  uint32 unk2; /* unk2 appears to be a ptr, unk1 = unk3 = 0 usually */
  uint32 unk3; 
} LSA_Q_UNK_GET_CONNUSER;

/* LSA_R_UNK_GET_CONNUSER */
typedef struct lsa_r_unk_get_connuser
{
  uint32 ptr_user_name;
  UNIHDR hdr_user_name;
  UNISTR2 uni2_user_name;
  
  uint32 unk1;
  
  uint32 ptr_dom_name;
  UNIHDR hdr_dom_name;
  UNISTR2 uni2_dom_name;

  NTSTATUS status;
} LSA_R_UNK_GET_CONNUSER;


typedef struct lsa_q_createaccount
{
	POLICY_HND pol; /* policy handle */
	DOM_SID2 sid;
	uint32 access; /* access */
} LSA_Q_CREATEACCOUNT;

typedef struct lsa_r_createaccount
{
	POLICY_HND pol; /* policy handle */
	NTSTATUS status;
} LSA_R_CREATEACCOUNT;


typedef struct lsa_q_openaccount
{
	POLICY_HND pol; /* policy handle */
	DOM_SID2 sid;
	uint32 access; /* desired access */
} LSA_Q_OPENACCOUNT;

typedef struct lsa_r_openaccount
{
	POLICY_HND pol; /* policy handle */
	NTSTATUS status;
} LSA_R_OPENACCOUNT;

typedef struct lsa_q_enumprivsaccount
{
	POLICY_HND pol; /* policy handle */
} LSA_Q_ENUMPRIVSACCOUNT;

typedef struct lsa_r_enumprivsaccount
{
	uint32 ptr;
	uint32 count;
	PRIVILEGE_SET set;
	NTSTATUS status;
} LSA_R_ENUMPRIVSACCOUNT;

typedef struct lsa_q_getsystemaccount
{
	POLICY_HND pol; /* policy handle */
} LSA_Q_GETSYSTEMACCOUNT;

typedef struct lsa_r_getsystemaccount
{
	uint32 access;
	NTSTATUS status;
} LSA_R_GETSYSTEMACCOUNT;


typedef struct lsa_q_setsystemaccount
{
	POLICY_HND pol; /* policy handle */
	uint32 access;
} LSA_Q_SETSYSTEMACCOUNT;

typedef struct lsa_r_setsystemaccount
{
	NTSTATUS status;
} LSA_R_SETSYSTEMACCOUNT;

typedef struct {
	UNIHDR hdr;
	UNISTR2 unistring;
} LSA_STRING;

typedef struct {
	POLICY_HND pol; /* policy handle */
	LSA_STRING privname;
} LSA_Q_LOOKUP_PRIV_VALUE;

typedef struct {
	LUID luid;
	NTSTATUS status;
} LSA_R_LOOKUP_PRIV_VALUE;

typedef struct lsa_q_addprivs
{
	POLICY_HND pol; /* policy handle */
	uint32 count;
	PRIVILEGE_SET set;
} LSA_Q_ADDPRIVS;

typedef struct lsa_r_addprivs
{
	NTSTATUS status;
} LSA_R_ADDPRIVS;


typedef struct lsa_q_removeprivs
{
	POLICY_HND pol; /* policy handle */
	uint32 allrights;
	uint32 ptr;
	uint32 count;
	PRIVILEGE_SET set;
} LSA_Q_REMOVEPRIVS;

typedef struct lsa_r_removeprivs
{
	NTSTATUS status;
} LSA_R_REMOVEPRIVS;

/*******************************************************/
#if 0 /* jerry, I think this not correct - gd */
typedef struct {
	POLICY_HND	handle;
	uint32		count;	/* ??? this is what ethereal calls it */
	DOM_SID		sid;
} LSA_Q_OPEN_TRUSTED_DOMAIN;
#endif

/* LSA_Q_OPEN_TRUSTED_DOMAIN - LSA Query Open Trusted Domain */
typedef struct lsa_q_open_trusted_domain
{
	POLICY_HND 	pol; 	/* policy handle */
	DOM_SID2 	sid;	/* domain sid */
	uint32 	access_mask;	/* access mask */
	
} LSA_Q_OPEN_TRUSTED_DOMAIN;

/* LSA_R_OPEN_TRUSTED_DOMAIN - response to LSA Query Open Trusted Domain */
typedef struct {
	POLICY_HND	handle;	/* trustdom policy handle */
	NTSTATUS	status; /* return code */
} LSA_R_OPEN_TRUSTED_DOMAIN;


/*******************************************************/

typedef struct {
	POLICY_HND	handle;	
	UNISTR4		secretname;
	uint32		access;
} LSA_Q_OPEN_SECRET;

typedef struct {
	POLICY_HND	handle;
	NTSTATUS	status;
} LSA_R_OPEN_SECRET;


/*******************************************************/

typedef struct {
	POLICY_HND	handle;
} LSA_Q_DELETE_OBJECT;

typedef struct {
	NTSTATUS 	status;
} LSA_R_DELETE_OBJECT;


/*******************************************************/

typedef struct {
	POLICY_HND      handle;
	UNISTR4         secretname;
	uint32          access;
} LSA_Q_CREATE_SECRET;

typedef struct {
	POLICY_HND      handle;
	NTSTATUS        status;
} LSA_R_CREATE_SECRET;


/*******************************************************/

typedef struct {
	POLICY_HND	handle;	
	UNISTR4		secretname;
	uint32		access;
} LSA_Q_CREATE_TRUSTED_DOMAIN;

typedef struct {
	POLICY_HND	handle;
	NTSTATUS	status;
} LSA_R_CREATE_TRUSTED_DOMAIN;


/*******************************************************/

typedef struct {
	uint32	size;	/* size is written on the wire twice so I 
			   can only assume that one is supposed to 
			   be a max length and one is a size */
	UNISTR2	*data;	/* not really a UNICODE string but the parsing 
			   is the same */
} LSA_DATA_BLOB;

typedef struct {
	POLICY_HND	handle;	
	LSA_DATA_BLOB   *old_value;
	LSA_DATA_BLOB	*new_value;
} LSA_Q_SET_SECRET;

typedef struct {
	NTSTATUS	status;
} LSA_R_SET_SECRET;

/* LSA_Q_QUERY_TRUSTED_DOMAIN_INFO - LSA query trusted domain info */
typedef struct lsa_query_trusted_domain_info
{
	POLICY_HND	pol; 		/* policy handle */
	uint16		info_class; 	/* info class */

} LSA_Q_QUERY_TRUSTED_DOMAIN_INFO;

/* LSA_Q_QUERY_TRUSTED_DOMAIN_INFO_BY_SID - LSA query trusted domain info */
typedef struct lsa_query_trusted_domain_info_by_sid
{
	POLICY_HND 	pol; 		/* policy handle */
	DOM_SID2 	dom_sid;	/* domain sid */
	uint16		info_class; 	/* info class */
	
} LSA_Q_QUERY_TRUSTED_DOMAIN_INFO_BY_SID;

/* LSA_Q_QUERY_TRUSTED_DOMAIN_INFO_BY_NAME - LSA query trusted domain info */
typedef struct lsa_query_trusted_domain_info_by_name
{
	POLICY_HND 	pol; 		/* policy handle */
	LSA_STRING 	domain_name;	/* domain name */
	uint16 		info_class; 	/* info class */
	
} LSA_Q_QUERY_TRUSTED_DOMAIN_INFO_BY_NAME;

typedef struct trusted_domain_info_name {
	LSA_STRING 	netbios_name; 
} TRUSTED_DOMAIN_INFO_NAME;

typedef struct trusted_domain_info_posix_offset {
	uint32 		posix_offset;
} TRUSTED_DOMAIN_INFO_POSIX_OFFSET;

typedef struct lsa_data_buf {
	uint32 size;
	uint32 offset;
	uint32 length;
	uint8 *data;
} LSA_DATA_BUF;

typedef struct lsa_data_buf_hdr {
	uint32 length;
	uint32 size;
	uint32 data_ptr;
} LSA_DATA_BUF_HDR;


typedef struct lsa_data_buf2 {
	uint32 size;
	uint8 *data;
} LSA_DATA_BUF2;

typedef struct trusted_domain_info_password {
	uint32 ptr_password;
	uint32 ptr_old_password;
	LSA_DATA_BUF_HDR password_hdr;
	LSA_DATA_BUF_HDR old_password_hdr;
	LSA_DATA_BUF password;
	LSA_DATA_BUF old_password;
} TRUSTED_DOMAIN_INFO_PASSWORD;

typedef struct trusted_domain_info_basic {
	LSA_STRING 	netbios_name;
	DOM_SID2 	sid;
} TRUSTED_DOMAIN_INFO_BASIC;

typedef struct trusted_domain_info_ex {
	LSA_STRING 	domain_name;
	LSA_STRING 	netbios_name;
	DOM_SID2 	sid;
	uint32 		trust_direction;
	uint32 		trust_type;
	uint32 		trust_attributes;
} TRUSTED_DOMAIN_INFO_EX;

typedef struct trust_domain_info_buffer {
	NTTIME 		last_update_time;
	uint32 		secret_type;
	LSA_DATA_BUF2 	data;
} LSA_TRUSTED_DOMAIN_INFO_BUFFER;

typedef struct trusted_domain_info_auth_info {
	uint32 incoming_count;
	LSA_TRUSTED_DOMAIN_INFO_BUFFER incoming_current_auth_info;
	LSA_TRUSTED_DOMAIN_INFO_BUFFER incoming_previous_auth_info;
	uint32 outgoing_count;
	LSA_TRUSTED_DOMAIN_INFO_BUFFER outgoing_current_auth_info;
	LSA_TRUSTED_DOMAIN_INFO_BUFFER outgoing_previous_auth_info;
} TRUSTED_DOMAIN_INFO_AUTH_INFO;

typedef struct trusted_domain_info_full_info {
	TRUSTED_DOMAIN_INFO_EX 		info_ex;
	TRUSTED_DOMAIN_INFO_POSIX_OFFSET posix_offset;
	TRUSTED_DOMAIN_INFO_AUTH_INFO 	auth_info;
} TRUSTED_DOMAIN_INFO_FULL_INFO;

typedef struct trusted_domain_info_11 {
	TRUSTED_DOMAIN_INFO_EX 		info_ex;
	LSA_DATA_BUF2 			data1;
} TRUSTED_DOMAIN_INFO_11;

typedef struct trusted_domain_info_all {
	TRUSTED_DOMAIN_INFO_EX 		info_ex;
	LSA_DATA_BUF2 			data1;
	TRUSTED_DOMAIN_INFO_POSIX_OFFSET posix_offset;
	TRUSTED_DOMAIN_INFO_AUTH_INFO 	auth_info;
} TRUSTED_DOMAIN_INFO_ALL;

/* LSA_TRUSTED_DOMAIN_INFO */
typedef union lsa_trusted_domain_info
{
	uint16 					info_class;
	TRUSTED_DOMAIN_INFO_NAME		name;
	/* deprecated - gd
	TRUSTED_DOMAIN_INFO_CONTROLLERS_INFO	controllers; */
	TRUSTED_DOMAIN_INFO_POSIX_OFFSET	posix_offset;
	TRUSTED_DOMAIN_INFO_PASSWORD		password;
	TRUSTED_DOMAIN_INFO_BASIC		basic;
	TRUSTED_DOMAIN_INFO_EX			info_ex;
	TRUSTED_DOMAIN_INFO_AUTH_INFO		auth_info;
	TRUSTED_DOMAIN_INFO_FULL_INFO		full_info;
	TRUSTED_DOMAIN_INFO_11			info11;
	TRUSTED_DOMAIN_INFO_ALL			info_all;

} LSA_TRUSTED_DOMAIN_INFO;

/* LSA_R_QUERY_TRUSTED_DOMAIN_INFO - LSA query trusted domain info */
typedef struct r_lsa_query_trusted_domain_info
{
	LSA_TRUSTED_DOMAIN_INFO *info;
	NTSTATUS status;
} LSA_R_QUERY_TRUSTED_DOMAIN_INFO;

#endif /* _RPC_LSA_H */
