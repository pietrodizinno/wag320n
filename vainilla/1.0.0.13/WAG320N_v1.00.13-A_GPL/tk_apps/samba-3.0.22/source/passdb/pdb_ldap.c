/* 
   Unix SMB/CIFS implementation.
   LDAP protocol helper functions for SAMBA
   Copyright (C) Jean François Micouleau	1998
   Copyright (C) Gerald Carter			2001-2003
   Copyright (C) Shahms King			2001
   Copyright (C) Andrew Bartlett		2002-2003
   Copyright (C) Stefan (metze) Metzmacher	2002-2003
    
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

/* TODO:
*  persistent connections: if using NSS LDAP, many connections are made
*      however, using only one within Samba would be nice
*  
*  Clean up SSL stuff, compile on OpenLDAP 1.x, 2.x, and Netscape SDK
*
*  Other LDAP based login attributes: accountExpires, etc.
*  (should be the domain of Samba proper, but the sam_password/SAM_ACCOUNT
*  structures don't have fields for some of these attributes)
*
*  SSL is done, but can't get the certificate based authentication to work
*  against on my test platform (Linux 2.4, OpenLDAP 2.x)
*/

/* NOTE: this will NOT work against an Active Directory server
*  due to the fact that the two password fields cannot be retrieved
*  from a server; recommend using security = domain in this situation
*  and/or winbind
*/

#include "includes.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_PASSDB

#include <lber.h>
#include <ldap.h>

/*
 * Work around versions of the LDAP client libs that don't have the OIDs
 * defined, or have them defined under the old name.  
 * This functionality is really a factor of the server, not the client 
 *
 */

#if defined(LDAP_EXOP_X_MODIFY_PASSWD) && !defined(LDAP_EXOP_MODIFY_PASSWD)
#define LDAP_EXOP_MODIFY_PASSWD LDAP_EXOP_X_MODIFY_PASSWD
#elif !defined(LDAP_EXOP_MODIFY_PASSWD)
#define LDAP_EXOP_MODIFY_PASSWD "1.3.6.1.4.1.4203.1.11.1"
#endif

#if defined(LDAP_EXOP_X_MODIFY_PASSWD_ID) && !defined(LDAP_EXOP_MODIFY_PASSWD_ID)
#define LDAP_TAG_EXOP_MODIFY_PASSWD_ID LDAP_EXOP_X_MODIFY_PASSWD_ID
#elif !defined(LDAP_EXOP_MODIFY_PASSWD_ID)
#define LDAP_TAG_EXOP_MODIFY_PASSWD_ID        ((ber_tag_t) 0x80U)
#endif

#if defined(LDAP_EXOP_X_MODIFY_PASSWD_NEW) && !defined(LDAP_EXOP_MODIFY_PASSWD_NEW)
#define LDAP_TAG_EXOP_MODIFY_PASSWD_NEW LDAP_EXOP_X_MODIFY_PASSWD_NEW
#elif !defined(LDAP_EXOP_MODIFY_PASSWD_NEW)
#define LDAP_TAG_EXOP_MODIFY_PASSWD_NEW       ((ber_tag_t) 0x82U)
#endif


#ifndef SAM_ACCOUNT
#define SAM_ACCOUNT struct sam_passwd
#endif

#include "smbldap.h"

/**********************************************************************
 Free a LDAPMessage (one is stored on the SAM_ACCOUNT).
 **********************************************************************/
 
void private_data_free_fn(void **result) 
{
	ldap_msgfree(*result);
	*result = NULL;
}

/**********************************************************************
 Get the attribute name given a user schame version.
 **********************************************************************/
 
static const char* get_userattr_key2string( int schema_ver, int key )
{
	switch ( schema_ver ) {
		case SCHEMAVER_SAMBAACCOUNT:
			return get_attr_key2string( attrib_map_v22, key );
			
		case SCHEMAVER_SAMBASAMACCOUNT:
			return get_attr_key2string( attrib_map_v30, key );
			
		default:
			DEBUG(0,("get_userattr_key2string: unknown schema version specified\n"));
			break;
	}
	return NULL;
}

/**********************************************************************
 Return the list of attribute names given a user schema version.
**********************************************************************/

const char** get_userattr_list( int schema_ver )
{
	switch ( schema_ver ) {
		case SCHEMAVER_SAMBAACCOUNT:
			return get_attr_list( attrib_map_v22 );
			
		case SCHEMAVER_SAMBASAMACCOUNT:
			return get_attr_list( attrib_map_v30 );
		default:
			DEBUG(0,("get_userattr_list: unknown schema version specified!\n"));
			break;
	}
	
	return NULL;
}

/**************************************************************************
 Return the list of attribute names to delete given a user schema version.
**************************************************************************/

static const char** get_userattr_delete_list( int schema_ver )
{
	switch ( schema_ver ) {
		case SCHEMAVER_SAMBAACCOUNT:
			return get_attr_list( attrib_map_to_delete_v22 );
			
		case SCHEMAVER_SAMBASAMACCOUNT:
			return get_attr_list( attrib_map_to_delete_v30 );
		default:
			DEBUG(0,("get_userattr_delete_list: unknown schema version specified!\n"));
			break;
	}
	
	return NULL;
}


/*******************************************************************
 Generate the LDAP search filter for the objectclass based on the 
 version of the schema we are using.
******************************************************************/

static const char* get_objclass_filter( int schema_ver )
{
	static fstring objclass_filter;
	
	switch( schema_ver ) {
		case SCHEMAVER_SAMBAACCOUNT:
			fstr_sprintf( objclass_filter, "(objectclass=%s)", LDAP_OBJ_SAMBAACCOUNT );
			break;
		case SCHEMAVER_SAMBASAMACCOUNT:
			fstr_sprintf( objclass_filter, "(objectclass=%s)", LDAP_OBJ_SAMBASAMACCOUNT );
			break;
		default:
			DEBUG(0,("get_objclass_filter: Invalid schema version specified!\n"));
			break;
	}
	
	return objclass_filter;	
}

/*****************************************************************
 Scan a sequence number off OpenLDAP's syncrepl contextCSN
******************************************************************/

static NTSTATUS ldapsam_get_seq_num(struct pdb_methods *my_methods, time_t *seq_num)
{
	struct ldapsam_privates *ldap_state = (struct ldapsam_privates *)my_methods->private_data;
	NTSTATUS ntstatus = NT_STATUS_UNSUCCESSFUL;
	LDAPMessage *msg = NULL;
	LDAPMessage *entry = NULL;
	TALLOC_CTX *mem_ctx;
	char **values = NULL;
	int rc, num_result, num_values, rid;
	pstring suffix;
	fstring tok;
	const char *p;
	const char **attrs;

	/* Unfortunatly there is no proper way to detect syncrepl-support in
	 * smbldap_connect_system(). The syncrepl OIDs are submitted for publication
	 * but do not show up in the root-DSE yet. Neither we can query the
	 * subschema-context for the syncProviderSubentry or syncConsumerSubentry
	 * objectclass. Currently we require lp_ldap_suffix() to show up as
	 * namingContext.  -  Guenther
	 */

	if (!lp_parm_bool(-1, "ldapsam", "syncrepl_seqnum", False)) {
		return ntstatus;
	}

	if (!seq_num) {
		DEBUG(3,("ldapsam_get_seq_num: no sequence_number\n"));
		return ntstatus;
	}

	if (!smbldap_has_naming_context(ldap_state->smbldap_state, lp_ldap_suffix())) {
		DEBUG(3,("ldapsam_get_seq_num: DIT not configured to hold %s "
			 "as top-level namingContext\n", lp_ldap_suffix()));
		return ntstatus;
	}

	mem_ctx = talloc_init("ldapsam_get_seq_num");

	if (mem_ctx == NULL)
		return NT_STATUS_NO_MEMORY;

	attrs = TALLOC_ARRAY(mem_ctx, const char *, 2);

	/* if we got a syncrepl-rid (up to three digits long) we speak with a consumer */
	rid = lp_parm_int(-1, "ldapsam", "syncrepl_rid", -1);
	if (rid > 0) {

		/* consumer syncreplCookie: */
		/* csn=20050126161620Z#0000001#00#00000 */
		attrs[0] = talloc_strdup(mem_ctx, "syncreplCookie");
		attrs[1] = NULL;
		pstr_sprintf( suffix, "cn=syncrepl%d,%s", rid, lp_ldap_suffix());

	} else {

		/* provider contextCSN */
		/* 20050126161620Z#000009#00#000000 */
		attrs[0] = talloc_strdup(mem_ctx, "contextCSN");
		attrs[1] = NULL;
		pstr_sprintf( suffix, "cn=ldapsync,%s", lp_ldap_suffix());

	}

	rc = smbldap_search(ldap_state->smbldap_state, suffix,
			    LDAP_SCOPE_BASE, "(objectclass=*)", attrs, 0, &msg);

	if (rc != LDAP_SUCCESS) {

		char *ld_error = NULL;
		ldap_get_option(ldap_state->smbldap_state->ldap_struct,
				LDAP_OPT_ERROR_STRING, &ld_error);
		DEBUG(0,("ldapsam_get_seq_num: Failed search for suffix: %s, error: %s (%s)\n", 
			suffix,ldap_err2string(rc), ld_error?ld_error:"unknown"));
		SAFE_FREE(ld_error);
		goto done;
	}

	num_result = ldap_count_entries(ldap_state->smbldap_state->ldap_struct, msg);
	if (num_result != 1) {
		DEBUG(3,("ldapsam_get_seq_num: Expected one entry, got %d\n", num_result));
		goto done;
	}

	entry = ldap_first_entry(ldap_state->smbldap_state->ldap_struct, msg);
	if (entry == NULL) {
		DEBUG(3,("ldapsam_get_seq_num: Could not retrieve entry\n"));
		goto done;
	}

	values = ldap_get_values(ldap_state->smbldap_state->ldap_struct, entry, attrs[0]);
	if (values == NULL) {
		DEBUG(3,("ldapsam_get_seq_num: no values\n"));
		goto done;
	}

	num_values = ldap_count_values(values);
	if (num_values == 0) {
		DEBUG(3,("ldapsam_get_seq_num: not a single value\n"));
		goto done;
	}

	p = values[0];
	if (!next_token(&p, tok, "#", sizeof(tok))) {
		DEBUG(0,("ldapsam_get_seq_num: failed to parse sequence number\n"));
		goto done;
	}

	p = tok;
	if (!strncmp(p, "csn=", strlen("csn=")))
		p += strlen("csn=");

	DEBUG(10,("ldapsam_get_seq_num: got %s: %s\n", attrs[0], p));

	*seq_num = generalized_to_unix_time(p);

	/* very basic sanity check */
	if (*seq_num <= 0) {
		DEBUG(3,("ldapsam_get_seq_num: invalid sequence number: %d\n", 
			(int)*seq_num));
		goto done;
	}

	ntstatus = NT_STATUS_OK;

 done:
	if (values != NULL)
		ldap_value_free(values);
	if (msg != NULL)
		ldap_msgfree(msg);
	if (mem_ctx)
		talloc_destroy(mem_ctx);

	return ntstatus;
}

/*******************************************************************
 Run the search by name.
******************************************************************/

int ldapsam_search_suffix_by_name(struct ldapsam_privates *ldap_state, 
					  const char *user,
					  LDAPMessage ** result,
					  const char **attr)
{
	pstring filter;
	char *escape_user = escape_ldap_string_alloc(user);

	if (!escape_user) {
		return LDAP_NO_MEMORY;
	}

	/*
	 * in the filter expression, replace %u with the real name
	 * so in ldap filter, %u MUST exist :-)
	 */
	pstr_sprintf(filter, "(&%s%s)", "(uid=%u)", 
		get_objclass_filter(ldap_state->schema_ver));

	/* 
	 * have to use this here because $ is filtered out
	   * in pstring_sub
	 */
	

	all_string_sub(filter, "%u", escape_user, sizeof(pstring));
	SAFE_FREE(escape_user);

	return smbldap_search_suffix(ldap_state->smbldap_state, filter, attr, result);
}

/*******************************************************************
 Run the search by rid.
******************************************************************/

static int ldapsam_search_suffix_by_rid (struct ldapsam_privates *ldap_state, 
					 uint32 rid, LDAPMessage ** result, 
					 const char **attr)
{
	pstring filter;
	int rc;

	pstr_sprintf(filter, "(&(rid=%i)%s)", rid, 
		get_objclass_filter(ldap_state->schema_ver));
	
	rc = smbldap_search_suffix(ldap_state->smbldap_state, filter, attr, result);
	
	return rc;
}

/*******************************************************************
 Run the search by SID.
******************************************************************/

static int ldapsam_search_suffix_by_sid (struct ldapsam_privates *ldap_state, 
					 const DOM_SID *sid, LDAPMessage ** result, 
					 const char **attr)
{
	pstring filter;
	int rc;
	fstring sid_string;

	pstr_sprintf(filter, "(&(%s=%s)%s)", 
		get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_USER_SID),
		sid_to_string(sid_string, sid), 
		get_objclass_filter(ldap_state->schema_ver));
		
	rc = smbldap_search_suffix(ldap_state->smbldap_state, filter, attr, result);
	
	return rc;
}

/*******************************************************************
 Delete complete object or objectclass and attrs from
 object found in search_result depending on lp_ldap_delete_dn
******************************************************************/

static NTSTATUS ldapsam_delete_entry(struct ldapsam_privates *ldap_state,
				     LDAPMessage *result,
				     const char *objectclass,
				     const char **attrs)
{
	int rc;
	LDAPMessage *entry = NULL;
	LDAPMod **mods = NULL;
	char *name, *dn;
	BerElement *ptr = NULL;

	rc = ldap_count_entries(ldap_state->smbldap_state->ldap_struct, result);

	if (rc != 1) {
		DEBUG(0, ("ldapsam_delete_entry: Entry must exist exactly once!\n"));
		return NT_STATUS_UNSUCCESSFUL;
	}

	entry = ldap_first_entry(ldap_state->smbldap_state->ldap_struct, result);
	dn = smbldap_get_dn(ldap_state->smbldap_state->ldap_struct, entry);
	if (!dn) {
		return NT_STATUS_UNSUCCESSFUL;
	}

	if (lp_ldap_delete_dn()) {
		NTSTATUS ret = NT_STATUS_OK;
		rc = smbldap_delete(ldap_state->smbldap_state, dn);

		if (rc != LDAP_SUCCESS) {
			DEBUG(0, ("ldapsam_delete_entry: Could not delete object %s\n", dn));
			ret = NT_STATUS_UNSUCCESSFUL;
		}
		SAFE_FREE(dn);
		return ret;
	}

	/* Ok, delete only the SAM attributes */
	
	for (name = ldap_first_attribute(ldap_state->smbldap_state->ldap_struct, entry, &ptr);
	     name != NULL;
	     name = ldap_next_attribute(ldap_state->smbldap_state->ldap_struct, entry, ptr)) {
		const char **attrib;

		/* We are only allowed to delete the attributes that
		   really exist. */

		for (attrib = attrs; *attrib != NULL; attrib++) {
			/* Don't delete LDAP_ATTR_MOD_TIMESTAMP attribute. */
			if (strequal(*attrib, get_userattr_key2string(ldap_state->schema_ver,
						LDAP_ATTR_MOD_TIMESTAMP))) {
				continue;
			}
			if (strequal(*attrib, name)) {
				DEBUG(10, ("ldapsam_delete_entry: deleting "
					   "attribute %s\n", name));
				smbldap_set_mod(&mods, LDAP_MOD_DELETE, name,
						NULL);
			}
		}

		ldap_memfree(name);
	}
	
	if (ptr != NULL) {
		ber_free(ptr, 0);
	}
	
	smbldap_set_mod(&mods, LDAP_MOD_DELETE, "objectClass", objectclass);

	rc = smbldap_modify(ldap_state->smbldap_state, dn, mods);
	ldap_mods_free(mods, True);

	if (rc != LDAP_SUCCESS) {
		char *ld_error = NULL;
		ldap_get_option(ldap_state->smbldap_state->ldap_struct, LDAP_OPT_ERROR_STRING,
				&ld_error);
		
		DEBUG(0, ("ldapsam_delete_entry: Could not delete attributes for %s, error: %s (%s)\n",
			  dn, ldap_err2string(rc), ld_error?ld_error:"unknown"));
		SAFE_FREE(ld_error);
		SAFE_FREE(dn);
		return NT_STATUS_UNSUCCESSFUL;
	}

	SAFE_FREE(dn);
	return NT_STATUS_OK;
}
		  
/* New Interface is being implemented here */

#if 0	/* JERRY - not uesed anymore */

/**********************************************************************
Initialize SAM_ACCOUNT from an LDAP query (unix attributes only)
*********************************************************************/
static BOOL get_unix_attributes (struct ldapsam_privates *ldap_state, 
				SAM_ACCOUNT * sampass,
				LDAPMessage * entry,
				gid_t *gid)
{
	pstring  homedir;
	pstring  temp;
	char **ldap_values;
	char **values;

	if ((ldap_values = ldap_get_values (ldap_state->smbldap_state->ldap_struct, entry, "objectClass")) == NULL) {
		DEBUG (1, ("get_unix_attributes: no objectClass! \n"));
		return False;
	}

	for (values=ldap_values;*values;values++) {
		if (strequal(*values, LDAP_OBJ_POSIXACCOUNT )) {
			break;
		}
	}
	
	if (!*values) { /*end of array, no posixAccount */
		DEBUG(10, ("user does not have %s attributes\n", LDAP_OBJ_POSIXACCOUNT));
		ldap_value_free(ldap_values);
		return False;
	}
	ldap_value_free(ldap_values);

	if ( !smbldap_get_single_pstring(ldap_state->smbldap_state->ldap_struct, entry, 
		get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_UNIX_HOME), homedir) ) 
	{
		return False;
	}
	
	if ( !smbldap_get_single_pstring(ldap_state->smbldap_state->ldap_struct, entry, 
		get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_GIDNUMBER), temp) )
	{
		return False;
	}
	
	*gid = (gid_t)atol(temp);

	pdb_set_unix_homedir(sampass, homedir, PDB_SET);
	
	DEBUG(10, ("user has %s attributes\n", LDAP_OBJ_POSIXACCOUNT));
	
	return True;
}

#endif

static time_t ldapsam_get_entry_timestamp(
	struct ldapsam_privates *ldap_state,
	LDAPMessage * entry)
{
	pstring temp;	
	struct tm tm;

	if (!smbldap_get_single_pstring(ldap_state->smbldap_state->ldap_struct, entry,
			get_userattr_key2string(ldap_state->schema_ver,LDAP_ATTR_MOD_TIMESTAMP),
			temp))
		return (time_t) 0;

	strptime(temp, "%Y%m%d%H%M%SZ", &tm);
	tzset();
	return timegm(&tm);
}

/**********************************************************************
 Initialize SAM_ACCOUNT from an LDAP query.
 (Based on init_sam_from_buffer in pdb_tdb.c)
*********************************************************************/

static BOOL init_sam_from_ldap(struct ldapsam_privates *ldap_state, 
				SAM_ACCOUNT * sampass,
				LDAPMessage * entry)
{
	time_t  logon_time,
			logoff_time,
			kickoff_time,
			pass_last_set_time, 
			pass_can_change_time, 
			pass_must_change_time,
			ldap_entry_time,
			bad_password_time;
	pstring 	username, 
			domain,
			nt_username,
			fullname,
			homedir,
			dir_drive,
			logon_script,
			profile_path,
			acct_desc,
			workstations;
	char		munged_dial[2048];
	uint32 		user_rid; 
	uint8 		smblmpwd[LM_HASH_LEN],
			smbntpwd[NT_HASH_LEN];
	BOOL 		use_samba_attrs = True;
	uint16 		acct_ctrl = 0, 
			logon_divs;
	uint16 		bad_password_count = 0, 
			logon_count = 0;
	uint32 hours_len;
	uint8 		hours[MAX_HOURS_LEN];
	pstring temp;
	LOGIN_CACHE	*cache_entry = NULL;
	uint32 		pwHistLen;
	pstring		tmpstring;
	BOOL expand_explicit = lp_passdb_expand_explicit();

	/*
	 * do a little initialization
	 */
	username[0] 	= '\0';
	domain[0] 	= '\0';
	nt_username[0] 	= '\0';
	fullname[0] 	= '\0';
	homedir[0] 	= '\0';
	dir_drive[0] 	= '\0';
	logon_script[0] = '\0';
	profile_path[0] = '\0';
	acct_desc[0] 	= '\0';
	munged_dial[0] 	= '\0';
	workstations[0] = '\0';
	 

	if (sampass == NULL || ldap_state == NULL || entry == NULL) {
		DEBUG(0, ("init_sam_from_ldap: NULL parameters found!\n"));
		return False;
	}

	if (ldap_state->smbldap_state->ldap_struct == NULL) {
		DEBUG(0, ("init_sam_from_ldap: ldap_state->smbldap_state->ldap_struct is NULL!\n"));
		return False;
	}
	
	if (!smbldap_get_single_pstring(ldap_state->smbldap_state->ldap_struct, entry, "uid", username)) {
		DEBUG(1, ("init_sam_from_ldap: No uid attribute found for this user!\n"));
		return False;
	}

	DEBUG(2, ("init_sam_from_ldap: Entry found for user: %s\n", username));

	pstrcpy(nt_username, username);

	pstrcpy(domain, ldap_state->domain_name);
	
	pdb_set_username(sampass, username, PDB_SET);

	pdb_set_domain(sampass, domain, PDB_DEFAULT);
	pdb_set_nt_username(sampass, nt_username, PDB_SET);

	/* deal with different attributes between the schema first */
	
	if ( ldap_state->schema_ver == SCHEMAVER_SAMBASAMACCOUNT ) {
		if (smbldap_get_single_pstring(ldap_state->smbldap_state->ldap_struct, entry, 
				get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_USER_SID), temp)) {
			pdb_set_user_sid_from_string(sampass, temp, PDB_SET);
		}
		
		if (smbldap_get_single_pstring(ldap_state->smbldap_state->ldap_struct, entry, 
				get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_PRIMARY_GROUP_SID), temp)) {
			pdb_set_group_sid_from_string(sampass, temp, PDB_SET);			
		} else {
			pdb_set_group_sid_from_rid(sampass, DOMAIN_GROUP_RID_USERS, PDB_DEFAULT);
		}
	} else {
		if (smbldap_get_single_pstring(ldap_state->smbldap_state->ldap_struct, entry,
				get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_USER_RID), temp)) {
			user_rid = (uint32)atol(temp);
			pdb_set_user_sid_from_rid(sampass, user_rid, PDB_SET);
		}
		
		if (!smbldap_get_single_pstring(ldap_state->smbldap_state->ldap_struct, entry, 
				get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_PRIMARY_GROUP_RID), temp)) {
			pdb_set_group_sid_from_rid(sampass, DOMAIN_GROUP_RID_USERS, PDB_DEFAULT);
		} else {
			uint32 group_rid;
			
			group_rid = (uint32)atol(temp);
			
			/* for some reason, we often have 0 as a primary group RID.
			   Make sure that we treat this just as a 'default' value */
			   
			if ( group_rid > 0 )
				pdb_set_group_sid_from_rid(sampass, group_rid, PDB_SET);
			else
				pdb_set_group_sid_from_rid(sampass, DOMAIN_GROUP_RID_USERS, PDB_DEFAULT);
		}
	}

	if (pdb_get_init_flags(sampass,PDB_USERSID) == PDB_DEFAULT) {
		DEBUG(1, ("init_sam_from_ldap: no %s or %s attribute found for this user %s\n", 
			get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_USER_SID),
			get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_USER_RID),
			username));
		return False;
	}

	if (!smbldap_get_single_pstring(ldap_state->smbldap_state->ldap_struct, entry, 
			get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_PWD_LAST_SET), temp)) {
		/* leave as default */
	} else {
		pass_last_set_time = (time_t) atol(temp);
		pdb_set_pass_last_set_time(sampass, pass_last_set_time, PDB_SET);
	}

	if (!smbldap_get_single_pstring(ldap_state->smbldap_state->ldap_struct, entry, 
			get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_LOGON_TIME), temp)) {
		/* leave as default */
	} else {
		logon_time = (time_t) atol(temp);
		pdb_set_logon_time(sampass, logon_time, PDB_SET);
	}

	if (!smbldap_get_single_pstring(ldap_state->smbldap_state->ldap_struct, entry, 
			get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_LOGOFF_TIME), temp)) {
		/* leave as default */
	} else {
		logoff_time = (time_t) atol(temp);
		pdb_set_logoff_time(sampass, logoff_time, PDB_SET);
	}

	if (!smbldap_get_single_pstring(ldap_state->smbldap_state->ldap_struct, entry, 
			get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_KICKOFF_TIME), temp)) {
		/* leave as default */
	} else {
		kickoff_time = (time_t) atol(temp);
		pdb_set_kickoff_time(sampass, kickoff_time, PDB_SET);
	}

	if (!smbldap_get_single_pstring(ldap_state->smbldap_state->ldap_struct, entry, 
			get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_PWD_CAN_CHANGE), temp)) {
		/* leave as default */
	} else {
		pass_can_change_time = (time_t) atol(temp);
		pdb_set_pass_can_change_time(sampass, pass_can_change_time, PDB_SET);
	}

	if (!smbldap_get_single_pstring(ldap_state->smbldap_state->ldap_struct, entry, 
			get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_PWD_MUST_CHANGE), temp)) {	
		/* leave as default */
	} else {
		pass_must_change_time = (time_t) atol(temp);
		pdb_set_pass_must_change_time(sampass, pass_must_change_time, PDB_SET);
	}

	/* recommend that 'gecos' and 'displayName' should refer to the same
	 * attribute OID.  userFullName depreciated, only used by Samba
	 * primary rules of LDAP: don't make a new attribute when one is already defined
	 * that fits your needs; using cn then displayName rather than 'userFullName'
	 */

	if (!smbldap_get_single_pstring(ldap_state->smbldap_state->ldap_struct, entry,
			get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_DISPLAY_NAME), fullname)) {
		if (!smbldap_get_single_pstring(ldap_state->smbldap_state->ldap_struct, entry,
				get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_CN), fullname)) {
			/* leave as default */
		} else {
			pdb_set_fullname(sampass, fullname, PDB_SET);
		}
	} else {
		pdb_set_fullname(sampass, fullname, PDB_SET);
	}

	if (!smbldap_get_single_pstring(ldap_state->smbldap_state->ldap_struct, entry, 
			get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_HOME_DRIVE), dir_drive)) 
	{
		pdb_set_dir_drive( sampass, lp_logon_drive(), PDB_DEFAULT );
	} else {
		pdb_set_dir_drive(sampass, dir_drive, PDB_SET);
	}

	if (!smbldap_get_single_pstring(ldap_state->smbldap_state->ldap_struct, entry,
			get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_HOME_PATH), homedir)) 
	{
		pdb_set_homedir( sampass, 
			talloc_sub_basic(sampass->mem_ctx, username, lp_logon_home()),
			PDB_DEFAULT );
	} else {
		pstrcpy( tmpstring, homedir );
		if (expand_explicit) {
			standard_sub_basic( username, tmpstring,
					    sizeof(tmpstring) );
		}
		pdb_set_homedir(sampass, tmpstring, PDB_SET);
	}

	if (!smbldap_get_single_pstring(ldap_state->smbldap_state->ldap_struct, entry,
			get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_LOGON_SCRIPT), logon_script)) 
	{
		pdb_set_logon_script( sampass, 
			talloc_sub_basic(sampass->mem_ctx, username, lp_logon_script()), 
			PDB_DEFAULT );
	} else {
		pstrcpy( tmpstring, logon_script );
		if (expand_explicit) {
			standard_sub_basic( username, tmpstring,
					    sizeof(tmpstring) );
		}
		pdb_set_logon_script(sampass, tmpstring, PDB_SET);
	}

	if (!smbldap_get_single_pstring(ldap_state->smbldap_state->ldap_struct, entry,
			get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_PROFILE_PATH), profile_path)) 
	{
		pdb_set_profile_path( sampass, 
			talloc_sub_basic( sampass->mem_ctx, username, lp_logon_path()),
			PDB_DEFAULT );
	} else {
		pstrcpy( tmpstring, profile_path );
		if (expand_explicit) {
			standard_sub_basic( username, tmpstring,
					    sizeof(tmpstring) );
		}
		pdb_set_profile_path(sampass, tmpstring, PDB_SET);
	}

	if (!smbldap_get_single_pstring(ldap_state->smbldap_state->ldap_struct, entry, 
		get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_DESC), acct_desc)) 
	{
		/* leave as default */
	} else {
		pdb_set_acct_desc(sampass, acct_desc, PDB_SET);
	}

	if (!smbldap_get_single_pstring(ldap_state->smbldap_state->ldap_struct, entry, 
		get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_USER_WKS), workstations)) {
		/* leave as default */;
	} else {
		pdb_set_workstations(sampass, workstations, PDB_SET);
	}

	if (!smbldap_get_single_attribute(ldap_state->smbldap_state->ldap_struct, entry, 
		get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_MUNGED_DIAL), munged_dial, sizeof(munged_dial))) {
		/* leave as default */;
	} else {
		pdb_set_munged_dial(sampass, munged_dial, PDB_SET);
	}
	
	/* FIXME: hours stuff should be cleaner */
	
	logon_divs = 168;
	hours_len = 21;
	memset(hours, 0xff, hours_len);

	if (ldap_state->is_nds_ldap) {
		char *user_dn;
		size_t pwd_len;
		char clear_text_pw[512];
   
		/* Make call to Novell eDirectory ldap extension to get clear text password.
			NOTE: This will only work if we have an SSL connection to eDirectory. */
		user_dn = smbldap_get_dn(ldap_state->smbldap_state->ldap_struct, entry);
		if (user_dn != NULL) {
			DEBUG(3, ("init_sam_from_ldap: smbldap_get_dn(%s) returned '%s'\n", username, user_dn));

			pwd_len = sizeof(clear_text_pw);
			if (pdb_nds_get_password(ldap_state->smbldap_state, user_dn, &pwd_len, clear_text_pw) == LDAP_SUCCESS) {
				nt_lm_owf_gen(clear_text_pw, smbntpwd, smblmpwd);
				if (!pdb_set_lanman_passwd(sampass, smblmpwd, PDB_SET))
					return False;
				ZERO_STRUCT(smblmpwd);
				if (!pdb_set_nt_passwd(sampass, smbntpwd, PDB_SET))
					return False;
				ZERO_STRUCT(smbntpwd);
				use_samba_attrs = False;
			}
		} else {
			DEBUG(0, ("init_sam_from_ldap: failed to get user_dn for '%s'\n", username));
		}
	}

	if (use_samba_attrs) {
		if (!smbldap_get_single_pstring (ldap_state->smbldap_state->ldap_struct, entry, 
			get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_LMPW), temp)) {
			/* leave as default */
		} else {
			pdb_gethexpwd(temp, smblmpwd);
			memset((char *)temp, '\0', strlen(temp)+1);
			if (!pdb_set_lanman_passwd(sampass, smblmpwd, PDB_SET))
				return False;
			ZERO_STRUCT(smblmpwd);
		}

		if (!smbldap_get_single_pstring (ldap_state->smbldap_state->ldap_struct, entry,
			get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_NTPW), temp)) {
			/* leave as default */
		} else {
			pdb_gethexpwd(temp, smbntpwd);
			memset((char *)temp, '\0', strlen(temp)+1);
			if (!pdb_set_nt_passwd(sampass, smbntpwd, PDB_SET))
				return False;
			ZERO_STRUCT(smbntpwd);
		}
	}

	pwHistLen = 0;

	pdb_get_account_policy(AP_PASSWORD_HISTORY, &pwHistLen);
	if (pwHistLen > 0){
		uint8 *pwhist = NULL;
		int i;

		/* We can only store (sizeof(pstring)-1)/64 password history entries. */
		pwHistLen = MIN(pwHistLen, ((sizeof(temp)-1)/64));

		if ((pwhist = SMB_MALLOC(pwHistLen * PW_HISTORY_ENTRY_LEN)) == NULL){
			DEBUG(0, ("init_sam_from_ldap: malloc failed!\n"));
			return False;
		}
		memset(pwhist, '\0', pwHistLen * PW_HISTORY_ENTRY_LEN);

		if (!smbldap_get_single_pstring (ldap_state->smbldap_state->ldap_struct, entry, 
			get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_PWD_HISTORY), temp)) {
			/* leave as default - zeros */
		} else {
			BOOL hex_failed = False;
			for (i = 0; i < pwHistLen; i++){
				/* Get the 16 byte salt. */
				if (!pdb_gethexpwd(&temp[i*64], &pwhist[i*PW_HISTORY_ENTRY_LEN])) {
					hex_failed = True;
					break;
				}
				/* Get the 16 byte MD5 hash of salt+passwd. */
				if (!pdb_gethexpwd(&temp[(i*64)+32],
						&pwhist[(i*PW_HISTORY_ENTRY_LEN)+PW_HISTORY_SALT_LEN])) {
					hex_failed = True;
					break;
				}
			}
			if (hex_failed) {
				DEBUG(0,("init_sam_from_ldap: Failed to get password history for user %s\n",
					username));
				memset(pwhist, '\0', pwHistLen * PW_HISTORY_ENTRY_LEN);
			}
		}
		if (!pdb_set_pw_history(sampass, pwhist, pwHistLen, PDB_SET)){
			SAFE_FREE(pwhist);
			return False;
		}
		SAFE_FREE(pwhist);
	}

	if (!smbldap_get_single_pstring (ldap_state->smbldap_state->ldap_struct, entry,
			get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_ACB_INFO), temp)) {
		acct_ctrl |= ACB_NORMAL;
	} else {
		acct_ctrl = pdb_decode_acct_ctrl(temp);

		if (acct_ctrl == 0)
			acct_ctrl |= ACB_NORMAL;

		pdb_set_acct_ctrl(sampass, acct_ctrl, PDB_SET);
	}

	pdb_set_hours_len(sampass, hours_len, PDB_SET);
	pdb_set_logon_divs(sampass, logon_divs, PDB_SET);

	if (!smbldap_get_single_pstring(ldap_state->smbldap_state->ldap_struct, entry,
			get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_BAD_PASSWORD_COUNT), temp)) {
			/* leave as default */
	} else {
		bad_password_count = (uint32) atol(temp);
		pdb_set_bad_password_count(sampass, bad_password_count, PDB_SET);
	}

	if (!smbldap_get_single_pstring(ldap_state->smbldap_state->ldap_struct, entry, 
			get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_BAD_PASSWORD_TIME), temp)) {
		/* leave as default */
	} else {
		bad_password_time = (time_t) atol(temp);
		pdb_set_bad_password_time(sampass, bad_password_time, PDB_SET);
	}


	if (!smbldap_get_single_pstring(ldap_state->smbldap_state->ldap_struct, entry,
			get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_LOGON_COUNT), temp)) {
			/* leave as default */
	} else {
		logon_count = (uint32) atol(temp);
		pdb_set_logon_count(sampass, logon_count, PDB_SET);
	}

	/* pdb_set_unknown_6(sampass, unknown6, PDB_SET); */

	if(!smbldap_get_single_pstring(ldap_state->smbldap_state->ldap_struct, entry,
		get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_LOGON_HOURS), temp)) {
			/* leave as default */
	} else {
		pdb_gethexhours(temp, hours);
		memset((char *)temp, '\0', strlen(temp) +1);
		pdb_set_hours(sampass, hours, PDB_SET);
		ZERO_STRUCT(hours);
	}

	/* check the timestamp of the cache vs ldap entry */
	if (!(ldap_entry_time = ldapsam_get_entry_timestamp(ldap_state, 
							    entry)))
		return True;

	/* see if we have newer updates */
	if (!(cache_entry = login_cache_read(sampass))) {
		DEBUG (9, ("No cache entry, bad count = %u, bad time = %u\n",
			   (unsigned int)pdb_get_bad_password_count(sampass),
			   (unsigned int)pdb_get_bad_password_time(sampass)));
		return True;
	}

	DEBUG(7, ("ldap time is %u, cache time is %u, bad time = %u\n", 
		  (unsigned int)ldap_entry_time, (unsigned int)cache_entry->entry_timestamp, 
		  (unsigned int)cache_entry->bad_password_time));

	if (ldap_entry_time > cache_entry->entry_timestamp) {
		/* cache is older than directory , so
		   we need to delete the entry but allow the 
		   fields to be written out */
		login_cache_delentry(sampass);
	} else {
		/* read cache in */
		pdb_set_acct_ctrl(sampass, 
				  pdb_get_acct_ctrl(sampass) | 
				  (cache_entry->acct_ctrl & ACB_AUTOLOCK),
				  PDB_SET);
		pdb_set_bad_password_count(sampass, 
					   cache_entry->bad_password_count, 
					   PDB_SET);
		pdb_set_bad_password_time(sampass, 
					  cache_entry->bad_password_time, 
					  PDB_SET);
	}

	SAFE_FREE(cache_entry);
	return True;
}

/**********************************************************************
 Initialize the ldap db from a SAM_ACCOUNT. Called on update.
 (Based on init_buffer_from_sam in pdb_tdb.c)
*********************************************************************/

static BOOL init_ldap_from_sam (struct ldapsam_privates *ldap_state, 
				LDAPMessage *existing,
				LDAPMod *** mods, SAM_ACCOUNT * sampass,
				BOOL (*need_update)(const SAM_ACCOUNT *,
						    enum pdb_elements))
{
	pstring temp;
	uint32 rid;

	if (mods == NULL || sampass == NULL) {
		DEBUG(0, ("init_ldap_from_sam: NULL parameters found!\n"));
		return False;
	}

	*mods = NULL;

	/* 
	 * took out adding "objectclass: sambaAccount"
	 * do this on a per-mod basis
	 */
	if (need_update(sampass, PDB_USERNAME))
		smbldap_make_mod(ldap_state->smbldap_state->ldap_struct, existing, mods, 
			      "uid", pdb_get_username(sampass));

	DEBUG(2, ("init_ldap_from_sam: Setting entry for user: %s\n", pdb_get_username(sampass)));

	/* only update the RID if we actually need to */
	if (need_update(sampass, PDB_USERSID)) {
		fstring sid_string;
		fstring dom_sid_string;
		const DOM_SID *user_sid = pdb_get_user_sid(sampass);
		
		switch ( ldap_state->schema_ver ) {
			case SCHEMAVER_SAMBAACCOUNT:
				if (!sid_peek_check_rid(&ldap_state->domain_sid, user_sid, &rid)) {
					DEBUG(1, ("init_ldap_from_sam: User's SID (%s) is not for this domain (%s), cannot add to LDAP!\n", 
						sid_to_string(sid_string, user_sid), 
						sid_to_string(dom_sid_string, &ldap_state->domain_sid)));
					return False;
				}
				slprintf(temp, sizeof(temp) - 1, "%i", rid);
				smbldap_make_mod(ldap_state->smbldap_state->ldap_struct, existing, mods,
					get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_USER_RID), 
					temp);
				break;
				
			case SCHEMAVER_SAMBASAMACCOUNT:
				smbldap_make_mod(ldap_state->smbldap_state->ldap_struct, existing, mods,
					get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_USER_SID), 
					sid_to_string(sid_string, user_sid));				      
				break;
				
			default:
				DEBUG(0,("init_ldap_from_sam: unknown schema version specified\n"));
				break;
		}		
	}

	/* we don't need to store the primary group RID - so leaving it
	   'free' to hang off the unix primary group makes life easier */

	if (need_update(sampass, PDB_GROUPSID)) {
		fstring sid_string;
		fstring dom_sid_string;
		const DOM_SID *group_sid = pdb_get_group_sid(sampass);
		
		switch ( ldap_state->schema_ver ) {
			case SCHEMAVER_SAMBAACCOUNT:
				if (!sid_peek_check_rid(&ldap_state->domain_sid, group_sid, &rid)) {
					DEBUG(1, ("init_ldap_from_sam: User's Primary Group SID (%s) is not for this domain (%s), cannot add to LDAP!\n",
						sid_to_string(sid_string, group_sid),
						sid_to_string(dom_sid_string, &ldap_state->domain_sid)));
					return False;
				}

				slprintf(temp, sizeof(temp) - 1, "%i", rid);
				smbldap_make_mod(ldap_state->smbldap_state->ldap_struct, existing, mods,
					get_userattr_key2string(ldap_state->schema_ver, 
					LDAP_ATTR_PRIMARY_GROUP_RID), temp);
				break;
				
			case SCHEMAVER_SAMBASAMACCOUNT:
				smbldap_make_mod(ldap_state->smbldap_state->ldap_struct, existing, mods,
					get_userattr_key2string(ldap_state->schema_ver, 
					LDAP_ATTR_PRIMARY_GROUP_SID), sid_to_string(sid_string, group_sid));
				break;
				
			default:
				DEBUG(0,("init_ldap_from_sam: unknown schema version specified\n"));
				break;
		}
		
	}
	
	/* displayName, cn, and gecos should all be the same
	 *  most easily accomplished by giving them the same OID
	 *  gecos isn't set here b/c it should be handled by the 
	 *  add-user script
	 *  We change displayName only and fall back to cn if
	 *  it does not exist.
	 */

	if (need_update(sampass, PDB_FULLNAME))
		smbldap_make_mod(ldap_state->smbldap_state->ldap_struct, existing, mods,
			get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_DISPLAY_NAME), 
			pdb_get_fullname(sampass));

	if (need_update(sampass, PDB_ACCTDESC))
		smbldap_make_mod(ldap_state->smbldap_state->ldap_struct, existing, mods,
			get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_DESC), 
			pdb_get_acct_desc(sampass));

	if (need_update(sampass, PDB_WORKSTATIONS))
		smbldap_make_mod(ldap_state->smbldap_state->ldap_struct, existing, mods,
			get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_USER_WKS), 
			pdb_get_workstations(sampass));
	
	if (need_update(sampass, PDB_MUNGEDDIAL))
		smbldap_make_mod(ldap_state->smbldap_state->ldap_struct, existing, mods,
			get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_MUNGED_DIAL), 
			pdb_get_munged_dial(sampass));
	
	if (need_update(sampass, PDB_SMBHOME))
		smbldap_make_mod(ldap_state->smbldap_state->ldap_struct, existing, mods,
			get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_HOME_PATH), 
			pdb_get_homedir(sampass));
			
	if (need_update(sampass, PDB_DRIVE))
		smbldap_make_mod(ldap_state->smbldap_state->ldap_struct, existing, mods,
			get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_HOME_DRIVE), 
			pdb_get_dir_drive(sampass));

	if (need_update(sampass, PDB_LOGONSCRIPT))
		smbldap_make_mod(ldap_state->smbldap_state->ldap_struct, existing, mods,
			get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_LOGON_SCRIPT), 
			pdb_get_logon_script(sampass));

	if (need_update(sampass, PDB_PROFILE))
		smbldap_make_mod(ldap_state->smbldap_state->ldap_struct, existing, mods,
			get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_PROFILE_PATH), 
			pdb_get_profile_path(sampass));

	slprintf(temp, sizeof(temp) - 1, "%li", pdb_get_logon_time(sampass));
	if (need_update(sampass, PDB_LOGONTIME))
		smbldap_make_mod(ldap_state->smbldap_state->ldap_struct, existing, mods,
			get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_LOGON_TIME), temp);

	slprintf(temp, sizeof(temp) - 1, "%li", pdb_get_logoff_time(sampass));
	if (need_update(sampass, PDB_LOGOFFTIME))
		smbldap_make_mod(ldap_state->smbldap_state->ldap_struct, existing, mods,
			get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_LOGOFF_TIME), temp);

	slprintf (temp, sizeof (temp) - 1, "%li", pdb_get_kickoff_time(sampass));
	if (need_update(sampass, PDB_KICKOFFTIME))
		smbldap_make_mod(ldap_state->smbldap_state->ldap_struct, existing, mods,
			get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_KICKOFF_TIME), temp);

	slprintf (temp, sizeof (temp) - 1, "%li", pdb_get_pass_can_change_time(sampass));
	if (need_update(sampass, PDB_CANCHANGETIME))
		smbldap_make_mod(ldap_state->smbldap_state->ldap_struct, existing, mods,
			get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_PWD_CAN_CHANGE), temp);

	slprintf (temp, sizeof (temp) - 1, "%li", pdb_get_pass_must_change_time(sampass));
	if (need_update(sampass, PDB_MUSTCHANGETIME))
		smbldap_make_mod(ldap_state->smbldap_state->ldap_struct, existing, mods,
			get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_PWD_MUST_CHANGE), temp);


	if ((pdb_get_acct_ctrl(sampass)&(ACB_WSTRUST|ACB_SVRTRUST|ACB_DOMTRUST))
			|| (lp_ldap_passwd_sync()!=LDAP_PASSWD_SYNC_ONLY)) {

		if (need_update(sampass, PDB_LMPASSWD)) {
			const uchar *lm_pw =  pdb_get_lanman_passwd(sampass);
			if (lm_pw) {
				pdb_sethexpwd(temp, lm_pw,
					      pdb_get_acct_ctrl(sampass));
				smbldap_make_mod(ldap_state->smbldap_state->ldap_struct, existing, mods,
						 get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_LMPW), 
						 temp);
			} else {
				smbldap_make_mod(ldap_state->smbldap_state->ldap_struct, existing, mods,
						 get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_LMPW), 
						 NULL);
			}
		}
		if (need_update(sampass, PDB_NTPASSWD)) {
			const uchar *nt_pw =  pdb_get_nt_passwd(sampass);
			if (nt_pw) {
				pdb_sethexpwd(temp, nt_pw,
					      pdb_get_acct_ctrl(sampass));
				smbldap_make_mod(ldap_state->smbldap_state->ldap_struct, existing, mods,
						 get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_NTPW), 
						 temp);
			} else {
				smbldap_make_mod(ldap_state->smbldap_state->ldap_struct, existing, mods,
						 get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_NTPW), 
						 NULL);
			}
		}

		if (need_update(sampass, PDB_PWHISTORY)) {
			uint32 pwHistLen = 0;
			pdb_get_account_policy(AP_PASSWORD_HISTORY, &pwHistLen);
			if (pwHistLen == 0) {
				/* Remove any password history from the LDAP store. */
				memset(temp, '0', 64); /* NOTE !!!! '0' *NOT '\0' */
				temp[64] = '\0';
			} else {
				int i; 
				uint32 currHistLen = 0;
				const uint8 *pwhist = pdb_get_pw_history(sampass, &currHistLen);
				if (pwhist != NULL) {
					/* We can only store (sizeof(pstring)-1)/64 password history entries. */
					pwHistLen = MIN(pwHistLen, ((sizeof(temp)-1)/64));
					for (i=0; i< pwHistLen && i < currHistLen; i++) {
						/* Store the salt. */
						pdb_sethexpwd(&temp[i*64], &pwhist[i*PW_HISTORY_ENTRY_LEN], 0);
						/* Followed by the md5 hash of salt + md4 hash */
						pdb_sethexpwd(&temp[(i*64)+32],
							&pwhist[(i*PW_HISTORY_ENTRY_LEN)+PW_HISTORY_SALT_LEN], 0);
						DEBUG(100, ("temp=%s\n", temp));
					}
				} 
			}
			smbldap_make_mod(ldap_state->smbldap_state->ldap_struct, existing, mods,
					 get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_PWD_HISTORY), 
					 temp);
		}

		if (need_update(sampass, PDB_PASSLASTSET)) {
			slprintf (temp, sizeof (temp) - 1, "%li", pdb_get_pass_last_set_time(sampass));
			smbldap_make_mod(ldap_state->smbldap_state->ldap_struct, existing, mods,
				get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_PWD_LAST_SET), 
				temp);
		}
	}

	if (need_update(sampass, PDB_HOURS)) {
		const uint8 *hours = pdb_get_hours(sampass);
		if (hours) {
			pdb_sethexhours(temp, hours);
			smbldap_make_mod(ldap_state->smbldap_state->ldap_struct,
				existing,
				mods,
				get_userattr_key2string(ldap_state->schema_ver,
						LDAP_ATTR_LOGON_HOURS),
				temp);
		}
	}

	if (need_update(sampass, PDB_ACCTCTRL))
		smbldap_make_mod(ldap_state->smbldap_state->ldap_struct, existing, mods,
			get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_ACB_INFO), 
			pdb_encode_acct_ctrl (pdb_get_acct_ctrl(sampass), NEW_PW_FORMAT_SPACE_PADDED_LEN));

	/* password lockout cache: 
	   - If we are now autolocking or clearing, we write to ldap
	   - If we are clearing, we delete the cache entry
	   - If the count is > 0, we update the cache

	   This even means when autolocking, we cache, just in case the
	   update doesn't work, and we have to cache the autolock flag */

	if (need_update(sampass, PDB_BAD_PASSWORD_COUNT))  /* &&
	    need_update(sampass, PDB_BAD_PASSWORD_TIME)) */ {
		uint16 badcount = pdb_get_bad_password_count(sampass);
		time_t badtime = pdb_get_bad_password_time(sampass);
		uint32 pol;
		pdb_get_account_policy(AP_BAD_ATTEMPT_LOCKOUT, &pol);

		DEBUG(3, ("updating bad password fields, policy=%u, count=%u, time=%u\n",
			(unsigned int)pol, (unsigned int)badcount, (unsigned int)badtime));

		if ((badcount >= pol) || (badcount == 0)) {
			DEBUG(7, ("making mods to update ldap, count=%u, time=%u\n",
				(unsigned int)badcount, (unsigned int)badtime));
			slprintf (temp, sizeof (temp) - 1, "%li", (long)badcount);
			smbldap_make_mod(
				ldap_state->smbldap_state->ldap_struct,
				existing, mods, 
				get_userattr_key2string(
					ldap_state->schema_ver, 
					LDAP_ATTR_BAD_PASSWORD_COUNT),
				temp);

			slprintf (temp, sizeof (temp) - 1, "%li", badtime);
			smbldap_make_mod(
				ldap_state->smbldap_state->ldap_struct, 
				existing, mods,
				get_userattr_key2string(
					ldap_state->schema_ver, 
					LDAP_ATTR_BAD_PASSWORD_TIME), 
				temp);
		}
		if (badcount == 0) {
			DEBUG(7, ("bad password count is reset, deleting login cache entry for %s\n", pdb_get_nt_username(sampass)));
			login_cache_delentry(sampass);
		} else {
			LOGIN_CACHE cache_entry;

			cache_entry.entry_timestamp = time(NULL);
			cache_entry.acct_ctrl = pdb_get_acct_ctrl(sampass);
			cache_entry.bad_password_count = badcount;
			cache_entry.bad_password_time = badtime;

			DEBUG(7, ("Updating bad password count and time in login cache\n"));
			login_cache_write(sampass, cache_entry);
		}
	}

	return True;
}

/**********************************************************************
 Connect to LDAP server for password enumeration.
*********************************************************************/

static NTSTATUS ldapsam_setsampwent(struct pdb_methods *my_methods, BOOL update, uint16 acb_mask)
{
	struct ldapsam_privates *ldap_state = (struct ldapsam_privates *)my_methods->private_data;
	int rc;
	pstring filter, suffix;
	const char **attr_list;
	BOOL machine_mask = False, user_mask = False;

	pstr_sprintf( filter, "(&%s%s)", "(uid=%u)", 
		get_objclass_filter(ldap_state->schema_ver));
	all_string_sub(filter, "%u", "*", sizeof(pstring));

	machine_mask 	= ((acb_mask != 0) && (acb_mask & (ACB_WSTRUST|ACB_SVRTRUST|ACB_DOMTRUST)));
	user_mask 	= ((acb_mask != 0) && (acb_mask & ACB_NORMAL));

	if (machine_mask) {
		pstrcpy(suffix, lp_ldap_machine_suffix());
	} else if (user_mask) {
		pstrcpy(suffix, lp_ldap_user_suffix());
	} else {
		pstrcpy(suffix, lp_ldap_suffix());
	}

	DEBUG(10,("ldapsam_setsampwent: LDAP Query for acb_mask 0x%x will use suffix %s\n", 
		acb_mask, suffix));

	attr_list = get_userattr_list(ldap_state->schema_ver);
	rc = smbldap_search(ldap_state->smbldap_state, suffix, LDAP_SCOPE_SUBTREE, filter, 
			    attr_list, 0, &ldap_state->result);
	free_attr_list( attr_list );

	if (rc != LDAP_SUCCESS) {
		DEBUG(0, ("ldapsam_setsampwent: LDAP search failed: %s\n", ldap_err2string(rc)));
		DEBUG(3, ("ldapsam_setsampwent: Query was: %s, %s\n", suffix, filter));
		ldap_msgfree(ldap_state->result);
		ldap_state->result = NULL;
		return NT_STATUS_UNSUCCESSFUL;
	}

	DEBUG(2, ("ldapsam_setsampwent: %d entries in the base %s\n",
		ldap_count_entries(ldap_state->smbldap_state->ldap_struct, 
		ldap_state->result), suffix));

	ldap_state->entry = ldap_first_entry(ldap_state->smbldap_state->ldap_struct,
				 ldap_state->result);
	ldap_state->index = 0;

	return NT_STATUS_OK;
}

/**********************************************************************
 End enumeration of the LDAP password list.
*********************************************************************/

static void ldapsam_endsampwent(struct pdb_methods *my_methods)
{
	struct ldapsam_privates *ldap_state = (struct ldapsam_privates *)my_methods->private_data;
	if (ldap_state->result) {
		ldap_msgfree(ldap_state->result);
		ldap_state->result = NULL;
	}
}

/**********************************************************************
Get the next entry in the LDAP password database.
*********************************************************************/

static NTSTATUS ldapsam_getsampwent(struct pdb_methods *my_methods, SAM_ACCOUNT *user)
{
	NTSTATUS ret = NT_STATUS_UNSUCCESSFUL;
	struct ldapsam_privates *ldap_state = (struct ldapsam_privates *)my_methods->private_data;
	BOOL bret = False;

	while (!bret) {
		if (!ldap_state->entry)
			return ret;
		
		ldap_state->index++;
		bret = init_sam_from_ldap(ldap_state, user, ldap_state->entry);
		
		ldap_state->entry = ldap_next_entry(ldap_state->smbldap_state->ldap_struct,
					    ldap_state->entry);	
	}

	return NT_STATUS_OK;
}

static void append_attr(const char ***attr_list, const char *new_attr)
{
	int i;

	if (new_attr == NULL) {
		return;
	}

	for (i=0; (*attr_list)[i] != NULL; i++) {
		;
	}

	(*attr_list) = SMB_REALLOC_ARRAY((*attr_list), const char *,  i+2);
	SMB_ASSERT((*attr_list) != NULL);
	(*attr_list)[i] = SMB_STRDUP(new_attr);
	(*attr_list)[i+1] = NULL;
}

/**********************************************************************
Get SAM_ACCOUNT entry from LDAP by username.
*********************************************************************/

static NTSTATUS ldapsam_getsampwnam(struct pdb_methods *my_methods, SAM_ACCOUNT *user, const char *sname)
{
	NTSTATUS ret = NT_STATUS_UNSUCCESSFUL;
	struct ldapsam_privates *ldap_state = (struct ldapsam_privates *)my_methods->private_data;
	LDAPMessage *result = NULL;
	LDAPMessage *entry = NULL;
	int count;
	const char ** attr_list;
	int rc;
	
	attr_list = get_userattr_list( ldap_state->schema_ver );
	append_attr(&attr_list, get_userattr_key2string(ldap_state->schema_ver,LDAP_ATTR_MOD_TIMESTAMP));
	rc = ldapsam_search_suffix_by_name(ldap_state, sname, &result, attr_list);
	free_attr_list( attr_list );

	if ( rc != LDAP_SUCCESS ) 
		return NT_STATUS_NO_SUCH_USER;
	
	count = ldap_count_entries(ldap_state->smbldap_state->ldap_struct, result);
	
	if (count < 1) {
		DEBUG(4, ("ldapsam_getsampwnam: Unable to locate user [%s] count=%d\n", sname, count));
		ldap_msgfree(result);
		return NT_STATUS_NO_SUCH_USER;
	} else if (count > 1) {
		DEBUG(1, ("ldapsam_getsampwnam: Duplicate entries for this user [%s] Failing. count=%d\n", sname, count));
		ldap_msgfree(result);
		return NT_STATUS_NO_SUCH_USER;
	}

	entry = ldap_first_entry(ldap_state->smbldap_state->ldap_struct, result);
	if (entry) {
		if (!init_sam_from_ldap(ldap_state, user, entry)) {
			DEBUG(1,("ldapsam_getsampwnam: init_sam_from_ldap failed for user '%s'!\n", sname));
			ldap_msgfree(result);
			return NT_STATUS_NO_SUCH_USER;
		}
		pdb_set_backend_private_data(user, result, 
					     private_data_free_fn, 
					     my_methods, PDB_CHANGED);
		ret = NT_STATUS_OK;
	} else {
		ldap_msgfree(result);
	}
	return ret;
}

static int ldapsam_get_ldap_user_by_sid(struct ldapsam_privates *ldap_state, 
				   const DOM_SID *sid, LDAPMessage **result) 
{
	int rc = -1;
	const char ** attr_list;
	uint32 rid;

	switch ( ldap_state->schema_ver ) {
		case SCHEMAVER_SAMBASAMACCOUNT:
			attr_list = get_userattr_list(ldap_state->schema_ver);
			append_attr(&attr_list, get_userattr_key2string(ldap_state->schema_ver,LDAP_ATTR_MOD_TIMESTAMP));
			rc = ldapsam_search_suffix_by_sid(ldap_state, sid, result, attr_list);
			free_attr_list( attr_list );

			if ( rc != LDAP_SUCCESS ) 
				return rc;
			break;
			
		case SCHEMAVER_SAMBAACCOUNT:
			if (!sid_peek_check_rid(&ldap_state->domain_sid, sid, &rid)) {
				return rc;
			}
		
			attr_list = get_userattr_list(ldap_state->schema_ver);
			rc = ldapsam_search_suffix_by_rid(ldap_state, rid, result, attr_list );
			free_attr_list( attr_list );

			if ( rc != LDAP_SUCCESS ) 
				return rc;
			break;
	}
	return rc;
}

/**********************************************************************
 Get SAM_ACCOUNT entry from LDAP by SID.
*********************************************************************/

static NTSTATUS ldapsam_getsampwsid(struct pdb_methods *my_methods, SAM_ACCOUNT * user, const DOM_SID *sid)
{
	struct ldapsam_privates *ldap_state = (struct ldapsam_privates *)my_methods->private_data;
	LDAPMessage *result = NULL;
	LDAPMessage *entry = NULL;
	int count;
	int rc;
	fstring sid_string;

	rc = ldapsam_get_ldap_user_by_sid(ldap_state, 
					  sid, &result); 
	if (rc != LDAP_SUCCESS)
		return NT_STATUS_NO_SUCH_USER;

	count = ldap_count_entries(ldap_state->smbldap_state->ldap_struct, result);
	
	if (count < 1) {
		DEBUG(4, ("ldapsam_getsampwsid: Unable to locate SID [%s] count=%d\n", sid_to_string(sid_string, sid),
		       count));
		ldap_msgfree(result);
		return NT_STATUS_NO_SUCH_USER;
	}  else if (count > 1) {
		DEBUG(1, ("ldapsam_getsampwsid: More than one user with SID [%s]. Failing. count=%d\n", sid_to_string(sid_string, sid),
		       count));
		ldap_msgfree(result);
		return NT_STATUS_NO_SUCH_USER;
	}

	entry = ldap_first_entry(ldap_state->smbldap_state->ldap_struct, result);
	if (!entry) {
		ldap_msgfree(result);
		return NT_STATUS_NO_SUCH_USER;
	}

	if (!init_sam_from_ldap(ldap_state, user, entry)) {
		DEBUG(1,("ldapsam_getsampwsid: init_sam_from_ldap failed!\n"));
		ldap_msgfree(result);
		return NT_STATUS_NO_SUCH_USER;
	}

	pdb_set_backend_private_data(user, result, 
				     private_data_free_fn, 
				     my_methods, PDB_CHANGED);
	return NT_STATUS_OK;
}	

static BOOL ldapsam_can_pwchange_exop(struct smbldap_state *ldap_state)
{
	return smbldap_has_extension(ldap_state, LDAP_EXOP_MODIFY_PASSWD);
}

/********************************************************************
 Do the actual modification - also change a plaintext passord if 
 it it set.
**********************************************************************/

static NTSTATUS ldapsam_modify_entry(struct pdb_methods *my_methods, 
				     SAM_ACCOUNT *newpwd, char *dn,
				     LDAPMod **mods, int ldap_op, 
				     BOOL (*need_update)(const SAM_ACCOUNT *, enum pdb_elements))
{
	struct ldapsam_privates *ldap_state = (struct ldapsam_privates *)my_methods->private_data;
	int rc;
	
	if (!my_methods || !newpwd || !dn) {
		return NT_STATUS_INVALID_PARAMETER;
	}
	
	if (!mods) {
		DEBUG(5,("ldapsam_modify_entry: mods is empty: nothing to modify\n"));
		/* may be password change below however */
	} else {
		switch(ldap_op) {
			case LDAP_MOD_ADD: 
				smbldap_set_mod(&mods, LDAP_MOD_ADD, 
						"objectclass", 
						LDAP_OBJ_ACCOUNT);
				rc = smbldap_add(ldap_state->smbldap_state, 
						 dn, mods);
				break;
			case LDAP_MOD_REPLACE: 
				rc = smbldap_modify(ldap_state->smbldap_state, 
						    dn ,mods);
				break;
			default: 	
				DEBUG(0,("ldapsam_modify_entry: Wrong LDAP operation type: %d!\n", 
					 ldap_op));
				return NT_STATUS_INVALID_PARAMETER;
		}
		
		if (rc!=LDAP_SUCCESS) {
			char *ld_error = NULL;
			ldap_get_option(ldap_state->smbldap_state->ldap_struct, LDAP_OPT_ERROR_STRING,
					&ld_error);
			DEBUG(1, ("ldapsam_modify_entry: Failed to %s user dn= %s with: %s\n\t%s\n",
			       ldap_op == LDAP_MOD_ADD ? "add" : "modify",
			       dn, ldap_err2string(rc),
			       ld_error?ld_error:"unknown"));
			SAFE_FREE(ld_error);
			return NT_STATUS_UNSUCCESSFUL;
		}  
	}
	
	if (!(pdb_get_acct_ctrl(newpwd)&(ACB_WSTRUST|ACB_SVRTRUST|ACB_DOMTRUST)) &&
			(lp_ldap_passwd_sync() != LDAP_PASSWD_SYNC_OFF) &&
			need_update(newpwd, PDB_PLAINTEXT_PW) &&
			(pdb_get_plaintext_passwd(newpwd)!=NULL)) {
		BerElement *ber;
		struct berval *bv;
		char *retoid = NULL;
		struct berval *retdata = NULL;
		char *utf8_password;
		char *utf8_dn;

		if (!ldap_state->is_nds_ldap) {
			if (!ldapsam_can_pwchange_exop(ldap_state->smbldap_state)) {
				DEBUG(2, ("ldap password change requested, but LDAP "
					  "server does not support it -- ignoring\n"));
				return NT_STATUS_OK;
			}
		}

		if (push_utf8_allocate(&utf8_password, pdb_get_plaintext_passwd(newpwd)) == (size_t)-1) {
			return NT_STATUS_NO_MEMORY;
		}

		if (push_utf8_allocate(&utf8_dn, dn) == (size_t)-1) {
			return NT_STATUS_NO_MEMORY;
		}

		if ((ber = ber_alloc_t(LBER_USE_DER))==NULL) {
			DEBUG(0,("ber_alloc_t returns NULL\n"));
			SAFE_FREE(utf8_password);
			return NT_STATUS_UNSUCCESSFUL;
		}

		ber_printf (ber, "{");
		ber_printf (ber, "ts", LDAP_TAG_EXOP_MODIFY_PASSWD_ID, utf8_dn);
	        ber_printf (ber, "ts", LDAP_TAG_EXOP_MODIFY_PASSWD_NEW, utf8_password);
	        ber_printf (ber, "N}");

	        if ((rc = ber_flatten (ber, &bv))<0) {
			DEBUG(0,("ldapsam_modify_entry: ber_flatten returns a value <0\n"));
			ber_free(ber,1);
			SAFE_FREE(utf8_dn);
			SAFE_FREE(utf8_password);
			return NT_STATUS_UNSUCCESSFUL;
		}
		
		SAFE_FREE(utf8_dn);
		SAFE_FREE(utf8_password);
		ber_free(ber, 1);

		if (!ldap_state->is_nds_ldap) {
			rc = smbldap_extended_operation(ldap_state->smbldap_state, 
							LDAP_EXOP_MODIFY_PASSWD,
							bv, NULL, NULL, &retoid, 
							&retdata);
		} else {
			rc = pdb_nds_set_password(ldap_state->smbldap_state, dn,
							pdb_get_plaintext_passwd(newpwd));
		}
		if (rc != LDAP_SUCCESS) {
			char *ld_error = NULL;

			if (rc == LDAP_OBJECT_CLASS_VIOLATION) {
				DEBUG(3, ("Could not set userPassword "
					  "attribute due to an objectClass "
					  "violation -- ignoring\n"));
				ber_bvfree(bv);
				return NT_STATUS_OK;
			}

			ldap_get_option(ldap_state->smbldap_state->ldap_struct, LDAP_OPT_ERROR_STRING,
					&ld_error);
			DEBUG(0,("ldapsam_modify_entry: LDAP Password could not be changed for user %s: %s\n\t%s\n",
				pdb_get_username(newpwd), ldap_err2string(rc), ld_error?ld_error:"unknown"));
			SAFE_FREE(ld_error);
			ber_bvfree(bv);
			return NT_STATUS_UNSUCCESSFUL;
		} else {
			DEBUG(3,("ldapsam_modify_entry: LDAP Password changed for user %s\n",pdb_get_username(newpwd)));
#ifdef DEBUG_PASSWORD
			DEBUG(100,("ldapsam_modify_entry: LDAP Password changed to %s\n",pdb_get_plaintext_passwd(newpwd)));
#endif    
			if (retdata)
				ber_bvfree(retdata);
			if (retoid)
				ldap_memfree(retoid);
		}
		ber_bvfree(bv);
	}
	return NT_STATUS_OK;
}

/**********************************************************************
 Delete entry from LDAP for username.
*********************************************************************/

static NTSTATUS ldapsam_delete_sam_account(struct pdb_methods *my_methods, SAM_ACCOUNT * sam_acct)
{
	struct ldapsam_privates *ldap_state = (struct ldapsam_privates *)my_methods->private_data;
	const char *sname;
	int rc;
	LDAPMessage *result = NULL;
	NTSTATUS ret;
	const char **attr_list;
	fstring objclass;

	if (!sam_acct) {
		DEBUG(0, ("ldapsam_delete_sam_account: sam_acct was NULL!\n"));
		return NT_STATUS_INVALID_PARAMETER;
	}

	sname = pdb_get_username(sam_acct);

	DEBUG (3, ("ldapsam_delete_sam_account: Deleting user %s from LDAP.\n", sname));

	attr_list= get_userattr_delete_list( ldap_state->schema_ver );
	rc = ldapsam_search_suffix_by_name(ldap_state, sname, &result, attr_list);

	if (rc != LDAP_SUCCESS)  {
		free_attr_list( attr_list );
		return NT_STATUS_NO_SUCH_USER;
	}
	
	switch ( ldap_state->schema_ver ) {
		case SCHEMAVER_SAMBASAMACCOUNT:
			fstrcpy( objclass, LDAP_OBJ_SAMBASAMACCOUNT );
			break;
			
		case SCHEMAVER_SAMBAACCOUNT:
			fstrcpy( objclass, LDAP_OBJ_SAMBAACCOUNT );
			break;
		default:
			fstrcpy( objclass, "UNKNOWN" );
			DEBUG(0,("ldapsam_delete_sam_account: Unknown schema version specified!\n"));
				break;
	}

	ret = ldapsam_delete_entry(ldap_state, result, objclass, attr_list );
	ldap_msgfree(result);
	free_attr_list( attr_list );

	return ret;
}

/**********************************************************************
 Helper function to determine for update_sam_account whether
 we need LDAP modification.
*********************************************************************/

static BOOL element_is_changed(const SAM_ACCOUNT *sampass,
			       enum pdb_elements element)
{
	return IS_SAM_CHANGED(sampass, element);
}

/**********************************************************************
 Update SAM_ACCOUNT.
*********************************************************************/

static NTSTATUS ldapsam_update_sam_account(struct pdb_methods *my_methods, SAM_ACCOUNT * newpwd)
{
	NTSTATUS ret = NT_STATUS_UNSUCCESSFUL;
	struct ldapsam_privates *ldap_state = (struct ldapsam_privates *)my_methods->private_data;
	int rc = 0;
	char *dn;
	LDAPMessage *result = NULL;
	LDAPMessage *entry = NULL;
	LDAPMod **mods = NULL;
	const char **attr_list;

	result = pdb_get_backend_private_data(newpwd, my_methods);
	if (!result) {
		attr_list = get_userattr_list(ldap_state->schema_ver);
		rc = ldapsam_search_suffix_by_name(ldap_state, pdb_get_username(newpwd), &result, attr_list );
		free_attr_list( attr_list );
		if (rc != LDAP_SUCCESS) {
			return NT_STATUS_UNSUCCESSFUL;
		}
		pdb_set_backend_private_data(newpwd, result, private_data_free_fn, my_methods, PDB_CHANGED);
	}

	if (ldap_count_entries(ldap_state->smbldap_state->ldap_struct, result) == 0) {
		DEBUG(0, ("ldapsam_update_sam_account: No user to modify!\n"));
		return NT_STATUS_UNSUCCESSFUL;
	}

	entry = ldap_first_entry(ldap_state->smbldap_state->ldap_struct, result);
	dn = smbldap_get_dn(ldap_state->smbldap_state->ldap_struct, entry);
	if (!dn) {
		return NT_STATUS_UNSUCCESSFUL;
	}

	DEBUG(4, ("ldapsam_update_sam_account: user %s to be modified has dn: %s\n", pdb_get_username(newpwd), dn));

	if (!init_ldap_from_sam(ldap_state, entry, &mods, newpwd,
				element_is_changed)) {
		DEBUG(0, ("ldapsam_update_sam_account: init_ldap_from_sam failed!\n"));
		SAFE_FREE(dn);
		if (mods != NULL)
			ldap_mods_free(mods,True);
		return NT_STATUS_UNSUCCESSFUL;
	}
	
	if (mods == NULL) {
		DEBUG(4,("ldapsam_update_sam_account: mods is empty: nothing to update for user: %s\n",
			 pdb_get_username(newpwd)));
		SAFE_FREE(dn);
		return NT_STATUS_OK;
	}
	
	ret = ldapsam_modify_entry(my_methods,newpwd,dn,mods,LDAP_MOD_REPLACE, element_is_changed);
	ldap_mods_free(mods,True);
	SAFE_FREE(dn);

	if (!NT_STATUS_IS_OK(ret)) {
		char *ld_error = NULL;
		ldap_get_option(ldap_state->smbldap_state->ldap_struct, LDAP_OPT_ERROR_STRING,
				&ld_error);
		DEBUG(0,("ldapsam_update_sam_account: failed to modify user with uid = %s, error: %s (%s)\n",
			 pdb_get_username(newpwd), ld_error?ld_error:"(unknwon)", ldap_err2string(rc)));
		SAFE_FREE(ld_error);
		return ret;
	}

	DEBUG(2, ("ldapsam_update_sam_account: successfully modified uid = %s in the LDAP database\n",
		  pdb_get_username(newpwd)));
	return NT_STATUS_OK;
}

/***************************************************************************
 Renames a SAM_ACCOUNT
 - The "rename user script" has full responsibility for changing everything
***************************************************************************/

static NTSTATUS ldapsam_rename_sam_account(struct pdb_methods *my_methods,
					   SAM_ACCOUNT *old_acct, 
					   const char *newname)
{
	const char *oldname;
	int rc;
	pstring rename_script;

	if (!old_acct) {
		DEBUG(0, ("ldapsam_rename_sam_account: old_acct was NULL!\n"));
		return NT_STATUS_INVALID_PARAMETER;
	}
	if (!newname) {
		DEBUG(0, ("ldapsam_rename_sam_account: newname was NULL!\n"));
		return NT_STATUS_INVALID_PARAMETER;
	}
		
	oldname = pdb_get_username(old_acct);

        /* rename the posix user */
        pstrcpy(rename_script, lp_renameuser_script());

	if (!(*rename_script))
		return NT_STATUS_ACCESS_DENIED;

	DEBUG (3, ("ldapsam_rename_sam_account: Renaming user %s to %s.\n", 
		   oldname, newname));

	pstring_sub(rename_script, "%unew", newname);
	pstring_sub(rename_script, "%uold", oldname);
	rc = smbrun(rename_script, NULL);

	DEBUG(rc ? 0 : 3,("Running the command `%s' gave %d\n", 
			  rename_script, rc));

	if (rc)
		return NT_STATUS_UNSUCCESSFUL;

	return NT_STATUS_OK;
}

/**********************************************************************
 Helper function to determine for update_sam_account whether
 we need LDAP modification.
 *********************************************************************/

static BOOL element_is_set_or_changed(const SAM_ACCOUNT *sampass,
				      enum pdb_elements element)
{
	return (IS_SAM_SET(sampass, element) ||
		IS_SAM_CHANGED(sampass, element));
}

/**********************************************************************
 Add SAM_ACCOUNT to LDAP.
*********************************************************************/

static NTSTATUS ldapsam_add_sam_account(struct pdb_methods *my_methods, SAM_ACCOUNT * newpwd)
{
	NTSTATUS ret = NT_STATUS_UNSUCCESSFUL;
	struct ldapsam_privates *ldap_state = (struct ldapsam_privates *)my_methods->private_data;
	int rc;
	LDAPMessage 	*result = NULL;
	LDAPMessage 	*entry  = NULL;
	pstring 	dn;
	LDAPMod 	**mods = NULL;
	int		ldap_op = LDAP_MOD_REPLACE;
	uint32		num_result;
	const char	**attr_list;
	char 		*escape_user;
	const char 	*username = pdb_get_username(newpwd);
	const DOM_SID 	*sid = pdb_get_user_sid(newpwd);
	pstring		filter;
	fstring         sid_string;

	if (!username || !*username) {
		DEBUG(0, ("ldapsam_add_sam_account: Cannot add user without a username!\n"));
		return NT_STATUS_INVALID_PARAMETER;
	}

	/* free this list after the second search or in case we exit on failure */
	attr_list = get_userattr_list(ldap_state->schema_ver);

	rc = ldapsam_search_suffix_by_name (ldap_state, username, &result, attr_list);

	if (rc != LDAP_SUCCESS) {
		free_attr_list( attr_list );
		return NT_STATUS_UNSUCCESSFUL;
	}

	if (ldap_count_entries(ldap_state->smbldap_state->ldap_struct, result) != 0) {
		DEBUG(0,("ldapsam_add_sam_account: User '%s' already in the base, with samba attributes\n", 
			 username));
		ldap_msgfree(result);
		free_attr_list( attr_list );
		return NT_STATUS_UNSUCCESSFUL;
	}
	ldap_msgfree(result);
	result = NULL;

	if (element_is_set_or_changed(newpwd, PDB_USERSID)) {
		rc = ldapsam_get_ldap_user_by_sid(ldap_state, 
						  sid, &result); 
		if (rc == LDAP_SUCCESS) {
			if (ldap_count_entries(ldap_state->smbldap_state->ldap_struct, result) != 0) {
				DEBUG(0,("ldapsam_add_sam_account: SID '%s' already in the base, with samba attributes\n", 
					 sid_to_string(sid_string, sid)));
				free_attr_list( attr_list );
				ldap_msgfree(result);
				return NT_STATUS_UNSUCCESSFUL;
			}
			ldap_msgfree(result);
		}
	}

	/* does the entry already exist but without a samba attributes?
	   we need to return the samba attributes here */
	   
	escape_user = escape_ldap_string_alloc( username );
	pstrcpy( filter, "(uid=%u)" );
	all_string_sub( filter, "%u", escape_user, sizeof(filter) );
	SAFE_FREE( escape_user );

	rc = smbldap_search_suffix(ldap_state->smbldap_state, 
				   filter, attr_list, &result);
	if ( rc != LDAP_SUCCESS ) {
		free_attr_list( attr_list );
		return NT_STATUS_UNSUCCESSFUL;
	}

	num_result = ldap_count_entries(ldap_state->smbldap_state->ldap_struct, result);
	
	if (num_result > 1) {
		DEBUG (0, ("ldapsam_add_sam_account: More than one user with that uid exists: bailing out!\n"));
		free_attr_list( attr_list );
		ldap_msgfree(result);
		return NT_STATUS_UNSUCCESSFUL;
	}
	
	/* Check if we need to update an existing entry */
	if (num_result == 1) {
		char *tmp;
		
		DEBUG(3,("ldapsam_add_sam_account: User exists without samba attributes: adding them\n"));
		ldap_op = LDAP_MOD_REPLACE;
		entry = ldap_first_entry (ldap_state->smbldap_state->ldap_struct, result);
		tmp = smbldap_get_dn (ldap_state->smbldap_state->ldap_struct, entry);
		if (!tmp) {
			free_attr_list( attr_list );
			ldap_msgfree(result);
			return NT_STATUS_UNSUCCESSFUL;
		}
		slprintf (dn, sizeof (dn) - 1, "%s", tmp);
		SAFE_FREE(tmp);

	} else if (ldap_state->schema_ver == SCHEMAVER_SAMBASAMACCOUNT) {

		/* There might be a SID for this account already - say an idmap entry */

		pstr_sprintf(filter, "(&(%s=%s)(|(objectClass=%s)(objectClass=%s)))", 
			 get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_USER_SID),
			 sid_to_string(sid_string, sid),
			 LDAP_OBJ_IDMAP_ENTRY,
			 LDAP_OBJ_SID_ENTRY);
		
		/* free old result before doing a new search */
		if (result != NULL) {
			ldap_msgfree(result);
			result = NULL;
		}
		rc = smbldap_search_suffix(ldap_state->smbldap_state, 
					   filter, attr_list, &result);
			
		if ( rc != LDAP_SUCCESS ) {
			free_attr_list( attr_list );
			return NT_STATUS_UNSUCCESSFUL;
		}
		
		num_result = ldap_count_entries(ldap_state->smbldap_state->ldap_struct, result);
		
		if (num_result > 1) {
			DEBUG (0, ("ldapsam_add_sam_account: More than one user with that uid exists: bailing out!\n"));
			free_attr_list( attr_list );
			ldap_msgfree(result);
			return NT_STATUS_UNSUCCESSFUL;
		}
		
		/* Check if we need to update an existing entry */
		if (num_result == 1) {
			char *tmp;
			
			DEBUG(3,("ldapsam_add_sam_account: User exists without samba attributes: adding them\n"));
			ldap_op = LDAP_MOD_REPLACE;
			entry = ldap_first_entry (ldap_state->smbldap_state->ldap_struct, result);
			tmp = smbldap_get_dn (ldap_state->smbldap_state->ldap_struct, entry);
			if (!tmp) {
				free_attr_list( attr_list );
				ldap_msgfree(result);
				return NT_STATUS_UNSUCCESSFUL;
			}
			slprintf (dn, sizeof (dn) - 1, "%s", tmp);
			SAFE_FREE(tmp);
		}
	}
	
	free_attr_list( attr_list );

	if (num_result == 0) {
		/* Check if we need to add an entry */
		DEBUG(3,("ldapsam_add_sam_account: Adding new user\n"));
		ldap_op = LDAP_MOD_ADD;
		if (username[strlen(username)-1] == '$') {
			slprintf (dn, sizeof (dn) - 1, "uid=%s,%s", username, lp_ldap_machine_suffix ());
		} else {
			slprintf (dn, sizeof (dn) - 1, "uid=%s,%s", username, lp_ldap_user_suffix ());
		}
	}

	if (!init_ldap_from_sam(ldap_state, entry, &mods, newpwd,
				element_is_set_or_changed)) {
		DEBUG(0, ("ldapsam_add_sam_account: init_ldap_from_sam failed!\n"));
		ldap_msgfree(result);
		if (mods != NULL)
			ldap_mods_free(mods,True);
		return NT_STATUS_UNSUCCESSFUL;		
	}
	
	ldap_msgfree(result);

	if (mods == NULL) {
		DEBUG(0,("ldapsam_add_sam_account: mods is empty: nothing to add for user: %s\n",pdb_get_username(newpwd)));
		return NT_STATUS_UNSUCCESSFUL;
	}
	switch ( ldap_state->schema_ver ) {
		case SCHEMAVER_SAMBAACCOUNT:
			smbldap_set_mod(&mods, LDAP_MOD_ADD, "objectclass", LDAP_OBJ_SAMBAACCOUNT);
			break;
		case SCHEMAVER_SAMBASAMACCOUNT:
			smbldap_set_mod(&mods, LDAP_MOD_ADD, "objectclass", LDAP_OBJ_SAMBASAMACCOUNT);
			break;
		default:
			DEBUG(0,("ldapsam_add_sam_account: invalid schema version specified\n"));
			break;
	}

	ret = ldapsam_modify_entry(my_methods,newpwd,dn,mods,ldap_op, element_is_set_or_changed);
	if (!NT_STATUS_IS_OK(ret)) {
		DEBUG(0,("ldapsam_add_sam_account: failed to modify/add user with uid = %s (dn = %s)\n",
			 pdb_get_username(newpwd),dn));
		ldap_mods_free(mods, True);
		return ret;
	}

	DEBUG(2,("ldapsam_add_sam_account: added: uid == %s in the LDAP database\n", pdb_get_username(newpwd)));
	ldap_mods_free(mods, True);
	
	return NT_STATUS_OK;
}

/**********************************************************************
 *********************************************************************/

static int ldapsam_search_one_group (struct ldapsam_privates *ldap_state,
				     const char *filter,
				     LDAPMessage ** result)
{
	int scope = LDAP_SCOPE_SUBTREE;
	int rc;
	const char **attr_list;

	attr_list = get_attr_list(groupmap_attr_list);
	rc = smbldap_search(ldap_state->smbldap_state, 
			    lp_ldap_group_suffix (), scope,
			    filter, attr_list, 0, result);
	free_attr_list( attr_list );

	if (rc != LDAP_SUCCESS) {
		char *ld_error = NULL;
		ldap_get_option(ldap_state->smbldap_state->ldap_struct, LDAP_OPT_ERROR_STRING,
				&ld_error);
		DEBUG(0, ("ldapsam_search_one_group: "
			  "Problem during the LDAP search: LDAP error: %s (%s)\n",
			  ld_error?ld_error:"(unknown)", ldap_err2string(rc)));
		DEBUGADD(3, ("ldapsam_search_one_group: Query was: %s, %s\n",
			  lp_ldap_group_suffix(), filter));
		SAFE_FREE(ld_error);
	}

	return rc;
}

/**********************************************************************
 *********************************************************************/

static BOOL init_group_from_ldap(struct ldapsam_privates *ldap_state,
				 GROUP_MAP *map, LDAPMessage *entry)
{
	pstring temp;

	if (ldap_state == NULL || map == NULL || entry == NULL ||
			ldap_state->smbldap_state->ldap_struct == NULL) {
		DEBUG(0, ("init_group_from_ldap: NULL parameters found!\n"));
		return False;
	}

	if (!smbldap_get_single_pstring(ldap_state->smbldap_state->ldap_struct, entry, 
			get_attr_key2string(groupmap_attr_list, LDAP_ATTR_GIDNUMBER), temp)) {
		DEBUG(0, ("init_group_from_ldap: Mandatory attribute %s not found\n", 
			get_attr_key2string( groupmap_attr_list, LDAP_ATTR_GIDNUMBER)));
		return False;
	}
	DEBUG(2, ("init_group_from_ldap: Entry found for group: %s\n", temp));

	map->gid = (gid_t)atol(temp);

	if (!smbldap_get_single_pstring(ldap_state->smbldap_state->ldap_struct, entry, 
			get_attr_key2string( groupmap_attr_list, LDAP_ATTR_GROUP_SID), temp)) {
		DEBUG(0, ("init_group_from_ldap: Mandatory attribute %s not found\n",
			get_attr_key2string( groupmap_attr_list, LDAP_ATTR_GROUP_SID)));
		return False;
	}
	
	if (!string_to_sid(&map->sid, temp)) {
		DEBUG(1, ("SID string [%s] could not be read as a valid SID\n", temp));
		return False;
	}

	if (!smbldap_get_single_pstring(ldap_state->smbldap_state->ldap_struct, entry, 
			get_attr_key2string( groupmap_attr_list, LDAP_ATTR_GROUP_TYPE), temp)) {
		DEBUG(0, ("init_group_from_ldap: Mandatory attribute %s not found\n",
			get_attr_key2string( groupmap_attr_list, LDAP_ATTR_GROUP_TYPE)));
		return False;
	}
	map->sid_name_use = (enum SID_NAME_USE)atol(temp);

	if ((map->sid_name_use < SID_NAME_USER) ||
			(map->sid_name_use > SID_NAME_UNKNOWN)) {
		DEBUG(0, ("init_group_from_ldap: Unknown Group type: %d\n", map->sid_name_use));
		return False;
	}

	if (!smbldap_get_single_pstring(ldap_state->smbldap_state->ldap_struct, entry, 
			get_attr_key2string( groupmap_attr_list, LDAP_ATTR_DISPLAY_NAME), temp)) {
		temp[0] = '\0';
		if (!smbldap_get_single_pstring(ldap_state->smbldap_state->ldap_struct, entry, 
			get_attr_key2string( groupmap_attr_list, LDAP_ATTR_CN), temp)) 
		{
			DEBUG(0, ("init_group_from_ldap: Attributes cn not found either \
for gidNumber(%lu)\n",(unsigned long)map->gid));
			return False;
		}
	}
	fstrcpy(map->nt_name, temp);

	if (!smbldap_get_single_pstring(ldap_state->smbldap_state->ldap_struct, entry, 
			get_attr_key2string( groupmap_attr_list, LDAP_ATTR_DESC), temp)) {
		temp[0] = '\0';
	}
	fstrcpy(map->comment, temp);

	return True;
}

/**********************************************************************
 *********************************************************************/

static BOOL init_ldap_from_group(LDAP *ldap_struct,
				 LDAPMessage *existing,
				 LDAPMod ***mods,
				 const GROUP_MAP *map)
{
	pstring tmp;

	if (mods == NULL || map == NULL) {
		DEBUG(0, ("init_ldap_from_group: NULL parameters found!\n"));
		return False;
	}

	*mods = NULL;

	sid_to_string(tmp, &map->sid);

	smbldap_make_mod(ldap_struct, existing, mods, 
		get_attr_key2string(groupmap_attr_list, LDAP_ATTR_GROUP_SID), tmp);
	pstr_sprintf(tmp, "%i", map->sid_name_use);
	smbldap_make_mod(ldap_struct, existing, mods, 
		get_attr_key2string(groupmap_attr_list, LDAP_ATTR_GROUP_TYPE), tmp);

	smbldap_make_mod(ldap_struct, existing, mods, 
		get_attr_key2string( groupmap_attr_list, LDAP_ATTR_DISPLAY_NAME), map->nt_name);
	smbldap_make_mod(ldap_struct, existing, mods, 
		get_attr_key2string( groupmap_attr_list, LDAP_ATTR_DESC), map->comment);

	return True;
}

/**********************************************************************
 *********************************************************************/

static NTSTATUS ldapsam_getgroup(struct pdb_methods *methods,
				 const char *filter,
				 GROUP_MAP *map)
{
	struct ldapsam_privates *ldap_state =
		(struct ldapsam_privates *)methods->private_data;
	LDAPMessage *result = NULL;
	LDAPMessage *entry = NULL;
	int count;

	if (ldapsam_search_one_group(ldap_state, filter, &result)
	    != LDAP_SUCCESS) {
		return NT_STATUS_NO_SUCH_GROUP;
	}

	count = ldap_count_entries(ldap_state->smbldap_state->ldap_struct, result);

	if (count < 1) {
		DEBUG(4, ("ldapsam_getgroup: Did not find group\n"));
		ldap_msgfree(result);
		return NT_STATUS_NO_SUCH_GROUP;
	}

	if (count > 1) {
		DEBUG(1, ("ldapsam_getgroup: Duplicate entries for filter %s: count=%d\n",
			  filter, count));
		ldap_msgfree(result);
		return NT_STATUS_NO_SUCH_GROUP;
	}

	entry = ldap_first_entry(ldap_state->smbldap_state->ldap_struct, result);

	if (!entry) {
		ldap_msgfree(result);
		return NT_STATUS_UNSUCCESSFUL;
	}

	if (!init_group_from_ldap(ldap_state, map, entry)) {
		DEBUG(1, ("ldapsam_getgroup: init_group_from_ldap failed for group filter %s\n",
			  filter));
		ldap_msgfree(result);
		return NT_STATUS_NO_SUCH_GROUP;
	}

	ldap_msgfree(result);
	return NT_STATUS_OK;
}

/**********************************************************************
 *********************************************************************/

static NTSTATUS ldapsam_getgrsid(struct pdb_methods *methods, GROUP_MAP *map,
				 DOM_SID sid)
{
	pstring filter;

	pstr_sprintf(filter, "(&(objectClass=%s)(%s=%s))",
		LDAP_OBJ_GROUPMAP, 
		get_attr_key2string(groupmap_attr_list, LDAP_ATTR_GROUP_SID),
		sid_string_static(&sid));

	return ldapsam_getgroup(methods, filter, map);
}

/**********************************************************************
 *********************************************************************/

static NTSTATUS ldapsam_getgrgid(struct pdb_methods *methods, GROUP_MAP *map,
				 gid_t gid)
{
	pstring filter;

	pstr_sprintf(filter, "(&(objectClass=%s)(%s=%lu))",
		LDAP_OBJ_GROUPMAP,
		get_attr_key2string(groupmap_attr_list, LDAP_ATTR_GIDNUMBER),
		(unsigned long)gid);

	return ldapsam_getgroup(methods, filter, map);
}

/**********************************************************************
 *********************************************************************/

static NTSTATUS ldapsam_getgrnam(struct pdb_methods *methods, GROUP_MAP *map,
				 const char *name)
{
	pstring filter;
	char *escape_name = escape_ldap_string_alloc(name);

	if (!escape_name) {
		return NT_STATUS_NO_MEMORY;
	}

	pstr_sprintf(filter, "(&(objectClass=%s)(|(%s=%s)(%s=%s)))",
		LDAP_OBJ_GROUPMAP,
		get_attr_key2string(groupmap_attr_list, LDAP_ATTR_DISPLAY_NAME), escape_name,
		get_attr_key2string(groupmap_attr_list, LDAP_ATTR_CN), escape_name);

	SAFE_FREE(escape_name);

	return ldapsam_getgroup(methods, filter, map);
}

static void add_rid_to_array_unique(TALLOC_CTX *mem_ctx,
				    uint32 rid, uint32 **pp_rids, size_t *p_num)
{
	size_t i;

	for (i=0; i<*p_num; i++) {
		if ((*pp_rids)[i] == rid)
			return;
	}
	
	*pp_rids = TALLOC_REALLOC_ARRAY(mem_ctx, *pp_rids, uint32, *p_num+1);

	if (*pp_rids == NULL)
		return;

	(*pp_rids)[*p_num] = rid;
	*p_num += 1;
}

static BOOL ldapsam_extract_rid_from_entry(LDAP *ldap_struct,
					   LDAPMessage *entry,
					   const DOM_SID *domain_sid,
					   uint32 *rid)
{
	fstring str;
	DOM_SID sid;

	if (!smbldap_get_single_attribute(ldap_struct, entry, "sambaSID",
					  str, sizeof(str)-1)) {
		DEBUG(10, ("Could not find sambaSID attribute\n"));
		return False;
	}

	if (!string_to_sid(&sid, str)) {
		DEBUG(10, ("Could not convert string %s to sid\n", str));
		return False;
	}

	if (sid_compare_domain(&sid, domain_sid) != 0) {
		DEBUG(10, ("SID %s is not in expected domain %s\n",
			   str, sid_string_static(domain_sid)));
		return False;
	}

	if (!sid_peek_rid(&sid, rid)) {
		DEBUG(10, ("Could not peek into RID\n"));
		return False;
	}

	return True;
}

static NTSTATUS ldapsam_enum_group_members(struct pdb_methods *methods,
					   TALLOC_CTX *mem_ctx,
					   const DOM_SID *group,
					   uint32 **pp_member_rids,
					   size_t *p_num_members)
{
	struct ldapsam_privates *ldap_state =
		(struct ldapsam_privates *)methods->private_data;
	struct smbldap_state *conn = ldap_state->smbldap_state;
	pstring filter;
	int rc, count;
	LDAPMessage *msg = NULL;
	LDAPMessage *entry;
	char **values = NULL;
	char **memberuid;
	char *sid_filter = NULL;
	char *tmp;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	if (!lp_parm_bool(-1, "ldapsam", "trusted", False))
		return pdb_default_enum_group_members(methods, mem_ctx, group,
						      pp_member_rids,
						      p_num_members);

	*pp_member_rids = NULL;
	*p_num_members = 0;

	pstr_sprintf(filter,
		     "(&(objectClass=sambaSamAccount)"
		     "(sambaPrimaryGroupSid=%s))",
		     sid_string_static(group));

	{
		const char *attrs[] = { "sambaSID", NULL };
		rc = smbldap_search(conn, lp_ldap_user_suffix(),
				    LDAP_SCOPE_SUBTREE, filter, attrs, 0,
				    &msg);
	}

	if (rc != LDAP_SUCCESS)
		goto done;

	for (entry = ldap_first_entry(conn->ldap_struct, msg);
	     entry != NULL;
	     entry = ldap_next_entry(conn->ldap_struct, entry))
	{
		uint32 rid;

		if (!ldapsam_extract_rid_from_entry(conn->ldap_struct,
						    entry,
						    get_global_sam_sid(),
						    &rid)) {
			DEBUG(2, ("Could not find sid from ldap entry\n"));
			continue;
		}

		add_rid_to_array_unique(mem_ctx, rid, pp_member_rids,
					p_num_members);
	}

	if (msg != NULL)
		ldap_msgfree(msg);

	pstr_sprintf(filter,
		     "(&(objectClass=sambaGroupMapping)"
		     "(objectClass=posixGroup)"
		     "(sambaSID=%s))",
		     sid_string_static(group));

	{
		const char *attrs[] = { "memberUid", NULL };
		rc = smbldap_search(conn, lp_ldap_user_suffix(),
				    LDAP_SCOPE_SUBTREE, filter, attrs, 0,
				    &msg);
	}

	if (rc != LDAP_SUCCESS)
		goto done;

	count = ldap_count_entries(conn->ldap_struct, msg);

	if (count > 1) {
		DEBUG(1, ("Found more than one groupmap entry for %s\n",
			  sid_string_static(group)));
		goto done;
	}

	if (count == 0) {
		result = NT_STATUS_OK;
		goto done;
	}

	entry = ldap_first_entry(conn->ldap_struct, msg);
	if (entry == NULL)
		goto done;

	values = ldap_get_values(conn->ldap_struct, msg, "memberUid");
	if (values == NULL) {
		result = NT_STATUS_OK;
		goto done;
	}

	sid_filter = SMB_STRDUP("(&(objectClass=sambaSamAccount)(|");
	if (sid_filter == NULL) {
		result = NT_STATUS_NO_MEMORY;
		goto done;
	}

	for (memberuid = values; *memberuid != NULL; memberuid += 1) {
		tmp = sid_filter;
		asprintf(&sid_filter, "%s(uid=%s)", tmp, *memberuid);
		free(tmp);
		if (sid_filter == NULL) {
			result = NT_STATUS_NO_MEMORY;
			goto done;
		}
	}

	tmp = sid_filter;
	asprintf(&sid_filter, "%s))", sid_filter);
	free(tmp);
	if (sid_filter == NULL) {
		result = NT_STATUS_NO_MEMORY;
		goto done;
	}

	{
		const char *attrs[] = { "sambaSID", NULL };
		rc = smbldap_search(conn, lp_ldap_user_suffix(),
				    LDAP_SCOPE_SUBTREE, sid_filter, attrs, 0,
				    &msg);
	}

	if (rc != LDAP_SUCCESS)
		goto done;

	for (entry = ldap_first_entry(conn->ldap_struct, msg);
	     entry != NULL;
	     entry = ldap_next_entry(conn->ldap_struct, entry))
	{
		fstring str;
		DOM_SID sid;
		uint32 rid;

		if (!smbldap_get_single_attribute(conn->ldap_struct,
						  entry, "sambaSID",
						  str, sizeof(str)-1))
			continue;

		if (!string_to_sid(&sid, str))
			goto done;

		if (!sid_check_is_in_our_domain(&sid)) {
			DEBUG(1, ("Inconsistent SAM -- group member uid not "
				  "in our domain\n"));
			continue;
		}

		sid_peek_rid(&sid, &rid);

		add_rid_to_array_unique(mem_ctx, rid, pp_member_rids,
					p_num_members);
	}

	result = NT_STATUS_OK;
	
 done:
	SAFE_FREE(sid_filter);

	if (values != NULL)
		ldap_value_free(values);

	if (msg != NULL)
		ldap_msgfree(msg);

	return result;
}

static NTSTATUS ldapsam_enum_group_memberships(struct pdb_methods *methods,
					       const char *username,
					       gid_t primary_gid,
					       DOM_SID **pp_sids, gid_t **pp_gids,
					       size_t *p_num_groups)
{
	struct ldapsam_privates *ldap_state =
		(struct ldapsam_privates *)methods->private_data;
	struct smbldap_state *conn = ldap_state->smbldap_state;
	pstring filter;
	const char *attrs[] = { "gidNumber", "sambaSID", NULL };
	char *escape_name;
	int rc;
	LDAPMessage *msg = NULL;
	LDAPMessage *entry;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	size_t num_sids, num_gids;

	if (!lp_parm_bool(-1, "ldapsam", "trusted", False))
		return pdb_default_enum_group_memberships(methods, username,
							  primary_gid, pp_sids,
							  pp_gids, p_num_groups);

	*pp_sids = NULL;
	num_sids = 0;

	escape_name = escape_ldap_string_alloc(username);

	if (escape_name == NULL)
		return NT_STATUS_NO_MEMORY;

	pstr_sprintf(filter, "(&(objectClass=posixGroup)"
		     "(|(memberUid=%s)(gidNumber=%d)))",
		     username, primary_gid);

	rc = smbldap_search(conn, lp_ldap_group_suffix(),
			    LDAP_SCOPE_SUBTREE, filter, attrs, 0, &msg);

	if (rc != LDAP_SUCCESS)
		goto done;

	num_gids = 0;
	*pp_gids = NULL;

	num_sids = 0;
	*pp_sids = NULL;

	/* We need to add the primary group as the first gid/sid */

	add_gid_to_array_unique(NULL, primary_gid, pp_gids, &num_gids);

	/* This sid will be replaced later */

	add_sid_to_array_unique(NULL, &global_sid_NULL, pp_sids, &num_sids);

	for (entry = ldap_first_entry(conn->ldap_struct, msg);
	     entry != NULL;
	     entry = ldap_next_entry(conn->ldap_struct, entry))
	{
		fstring str;
		DOM_SID sid;
		gid_t gid;
		char *end;

		if (!smbldap_get_single_attribute(conn->ldap_struct,
						  entry, "sambaSID",
						  str, sizeof(str)-1))
			continue;

		if (!string_to_sid(&sid, str))
			goto done;

		if (!smbldap_get_single_attribute(conn->ldap_struct,
						  entry, "gidNumber",
						  str, sizeof(str)-1))
			continue;

		gid = strtoul(str, &end, 10);

		if (PTR_DIFF(end, str) != strlen(str))
			goto done;

		if (gid == primary_gid) {
			sid_copy(&(*pp_sids)[0], &sid);
		} else {
			add_gid_to_array_unique(NULL, gid, pp_gids, &num_gids);
			add_sid_to_array_unique(NULL, &sid, pp_sids, &num_sids);
		}
	}

	if (sid_compare(&global_sid_NULL, &(*pp_sids)[0]) == 0) {
		DEBUG(3, ("primary group of [%s] not found\n", username));
		goto done;
	}

	*p_num_groups = num_sids;

	result = NT_STATUS_OK;

 done:

	SAFE_FREE(escape_name);
	if (msg != NULL)
		ldap_msgfree(msg);

	return result;
}

/**********************************************************************
 *********************************************************************/

static int ldapsam_search_one_group_by_gid(struct ldapsam_privates *ldap_state,
					   gid_t gid,
					   LDAPMessage **result)
{
	pstring filter;

	pstr_sprintf(filter, "(&(|(objectClass=%s)(objectclass=%s))(%s=%lu))", 
		LDAP_OBJ_POSIXGROUP, LDAP_OBJ_IDMAP_ENTRY,
		get_attr_key2string(groupmap_attr_list, LDAP_ATTR_GIDNUMBER),
		(unsigned long)gid);

	return ldapsam_search_one_group(ldap_state, filter, result);
}

/**********************************************************************
 *********************************************************************/

static NTSTATUS ldapsam_add_group_mapping_entry(struct pdb_methods *methods,
						GROUP_MAP *map)
{
	struct ldapsam_privates *ldap_state =
		(struct ldapsam_privates *)methods->private_data;
	LDAPMessage *result = NULL;
	LDAPMod **mods = NULL;
	int count;

	char *tmp;
	pstring dn;
	LDAPMessage *entry;

	GROUP_MAP dummy;

	int rc;

	if (NT_STATUS_IS_OK(ldapsam_getgrgid(methods, &dummy,
					     map->gid))) {
		DEBUG(0, ("ldapsam_add_group_mapping_entry: Group %ld already exists in LDAP\n", (unsigned long)map->gid));
		return NT_STATUS_UNSUCCESSFUL;
	}

	rc = ldapsam_search_one_group_by_gid(ldap_state, map->gid, &result);
	if (rc != LDAP_SUCCESS) {
		ldap_msgfree(result);
		return NT_STATUS_UNSUCCESSFUL;
	}

	count = ldap_count_entries(ldap_state->smbldap_state->ldap_struct, result);

	if ( count == 0 ) {
		/* There's no posixGroup account, let's try to find an
		 * appropriate idmap entry for aliases */

		pstring suffix;
		pstring filter;
		const char **attr_list;

		ldap_msgfree(result);

		pstrcpy( suffix, lp_ldap_idmap_suffix() );
		pstr_sprintf(filter, "(&(objectClass=%s)(%s=%u))",
			     LDAP_OBJ_IDMAP_ENTRY, LDAP_ATTRIBUTE_GIDNUMBER,
			     map->gid);
		
		attr_list = get_attr_list( sidmap_attr_list );
		rc = smbldap_search(ldap_state->smbldap_state, suffix,
				    LDAP_SCOPE_SUBTREE, filter, attr_list,
				    0, &result);

		free_attr_list(attr_list);

		if (rc != LDAP_SUCCESS) {
			DEBUG(3,("Failure looking up entry (%s)\n",
				 ldap_err2string(rc) ));
			ldap_msgfree(result);
			return NT_STATUS_UNSUCCESSFUL;
		}
	}
			   
	count = ldap_count_entries(ldap_state->smbldap_state->ldap_struct, result);
	if ( count == 0 ) {
		ldap_msgfree(result);
		return NT_STATUS_UNSUCCESSFUL;
	}

	if (count > 1) {
		DEBUG(2, ("ldapsam_add_group_mapping_entry: Group %lu must exist exactly once in LDAP\n",
			  (unsigned long)map->gid));
		ldap_msgfree(result);
		return NT_STATUS_UNSUCCESSFUL;
	}

	entry = ldap_first_entry(ldap_state->smbldap_state->ldap_struct, result);
	tmp = smbldap_get_dn(ldap_state->smbldap_state->ldap_struct, entry);
	if (!tmp) {
		ldap_msgfree(result);
		return NT_STATUS_UNSUCCESSFUL;
	}
	pstrcpy(dn, tmp);
	SAFE_FREE(tmp);

	if (!init_ldap_from_group(ldap_state->smbldap_state->ldap_struct,
				  result, &mods, map)) {
		DEBUG(0, ("ldapsam_add_group_mapping_entry: init_ldap_from_group failed!\n"));
		ldap_mods_free(mods, True);
		ldap_msgfree(result);
		return NT_STATUS_UNSUCCESSFUL;
	}

	ldap_msgfree(result);

	if (mods == NULL) {
		DEBUG(0, ("ldapsam_add_group_mapping_entry: mods is empty\n"));
		return NT_STATUS_UNSUCCESSFUL;
	}

	smbldap_set_mod(&mods, LDAP_MOD_ADD, "objectClass", LDAP_OBJ_GROUPMAP );

	rc = smbldap_modify(ldap_state->smbldap_state, dn, mods);
	ldap_mods_free(mods, True);

	if (rc != LDAP_SUCCESS) {
		char *ld_error = NULL;
		ldap_get_option(ldap_state->smbldap_state->ldap_struct, LDAP_OPT_ERROR_STRING,
				&ld_error);
		DEBUG(0, ("ldapsam_add_group_mapping_entry: failed to add group %lu error: %s (%s)\n", (unsigned long)map->gid, 
			  ld_error ? ld_error : "(unknown)", ldap_err2string(rc)));
		SAFE_FREE(ld_error);
		return NT_STATUS_UNSUCCESSFUL;
	}

	DEBUG(2, ("ldapsam_add_group_mapping_entry: successfully modified group %lu in LDAP\n", (unsigned long)map->gid));
	return NT_STATUS_OK;
}

/**********************************************************************
 *********************************************************************/

static NTSTATUS ldapsam_update_group_mapping_entry(struct pdb_methods *methods,
						   GROUP_MAP *map)
{
	struct ldapsam_privates *ldap_state =
		(struct ldapsam_privates *)methods->private_data;
	int rc;
	char *dn = NULL;
	LDAPMessage *result = NULL;
	LDAPMessage *entry = NULL;
	LDAPMod **mods = NULL;

	rc = ldapsam_search_one_group_by_gid(ldap_state, map->gid, &result);

	if (rc != LDAP_SUCCESS) {
		return NT_STATUS_UNSUCCESSFUL;
	}

	if (ldap_count_entries(ldap_state->smbldap_state->ldap_struct, result) == 0) {
		DEBUG(0, ("ldapsam_update_group_mapping_entry: No group to modify!\n"));
		ldap_msgfree(result);
		return NT_STATUS_UNSUCCESSFUL;
	}

	entry = ldap_first_entry(ldap_state->smbldap_state->ldap_struct, result);

	if (!init_ldap_from_group(ldap_state->smbldap_state->ldap_struct,
				  result, &mods, map)) {
		DEBUG(0, ("ldapsam_update_group_mapping_entry: init_ldap_from_group failed\n"));
		ldap_msgfree(result);
		if (mods != NULL)
			ldap_mods_free(mods,True);
		return NT_STATUS_UNSUCCESSFUL;
	}

	if (mods == NULL) {
		DEBUG(4, ("ldapsam_update_group_mapping_entry: mods is empty: nothing to do\n"));
		ldap_msgfree(result);
		return NT_STATUS_OK;
	}

	dn = smbldap_get_dn(ldap_state->smbldap_state->ldap_struct, entry);
	if (!dn) {
		ldap_msgfree(result);
		return NT_STATUS_UNSUCCESSFUL;
	}
	rc = smbldap_modify(ldap_state->smbldap_state, dn, mods);
	SAFE_FREE(dn);

	ldap_mods_free(mods, True);
	ldap_msgfree(result);

	if (rc != LDAP_SUCCESS) {
		char *ld_error = NULL;
		ldap_get_option(ldap_state->smbldap_state->ldap_struct, LDAP_OPT_ERROR_STRING,
				&ld_error);
		DEBUG(0, ("ldapsam_update_group_mapping_entry: failed to modify group %lu error: %s (%s)\n", (unsigned long)map->gid, 
			  ld_error ? ld_error : "(unknown)", ldap_err2string(rc)));
		SAFE_FREE(ld_error);
		return NT_STATUS_UNSUCCESSFUL;
	}

	DEBUG(2, ("ldapsam_update_group_mapping_entry: successfully modified group %lu in LDAP\n", (unsigned long)map->gid));
	return NT_STATUS_OK;
}

/**********************************************************************
 *********************************************************************/

static NTSTATUS ldapsam_delete_group_mapping_entry(struct pdb_methods *methods,
						   DOM_SID sid)
{
	struct ldapsam_privates *ldap_state = (struct ldapsam_privates *)methods->private_data;
	pstring sidstring, filter;
	LDAPMessage *result = NULL;
	int rc;
	NTSTATUS ret;
	const char **attr_list;

	sid_to_string(sidstring, &sid);
	
	pstr_sprintf(filter, "(&(objectClass=%s)(%s=%s))", 
		LDAP_OBJ_GROUPMAP, LDAP_ATTRIBUTE_SID, sidstring);

	rc = ldapsam_search_one_group(ldap_state, filter, &result);

	if (rc != LDAP_SUCCESS) {
		return NT_STATUS_NO_SUCH_GROUP;
	}

	attr_list = get_attr_list( groupmap_attr_list_to_delete );
	ret = ldapsam_delete_entry(ldap_state, result, LDAP_OBJ_GROUPMAP, attr_list);
	free_attr_list ( attr_list );

	ldap_msgfree(result);

	return ret;
}

/**********************************************************************
 *********************************************************************/

static NTSTATUS ldapsam_setsamgrent(struct pdb_methods *my_methods, BOOL update)
{
	struct ldapsam_privates *ldap_state = (struct ldapsam_privates *)my_methods->private_data;
	fstring filter;
	int rc;
	const char **attr_list;

	pstr_sprintf( filter, "(objectclass=%s)", LDAP_OBJ_GROUPMAP);
	attr_list = get_attr_list( groupmap_attr_list );
	rc = smbldap_search(ldap_state->smbldap_state, lp_ldap_group_suffix(),
			    LDAP_SCOPE_SUBTREE, filter,
			    attr_list, 0, &ldap_state->result);
	free_attr_list( attr_list );

	if (rc != LDAP_SUCCESS) {
		DEBUG(0, ("ldapsam_setsamgrent: LDAP search failed: %s\n", ldap_err2string(rc)));
		DEBUG(3, ("ldapsam_setsamgrent: Query was: %s, %s\n", lp_ldap_group_suffix(), filter));
		ldap_msgfree(ldap_state->result);
		ldap_state->result = NULL;
		return NT_STATUS_UNSUCCESSFUL;
	}

	DEBUG(2, ("ldapsam_setsamgrent: %d entries in the base!\n",
		  ldap_count_entries(ldap_state->smbldap_state->ldap_struct,
				     ldap_state->result)));

	ldap_state->entry = ldap_first_entry(ldap_state->smbldap_state->ldap_struct, ldap_state->result);
	ldap_state->index = 0;

	return NT_STATUS_OK;
}

/**********************************************************************
 *********************************************************************/

static void ldapsam_endsamgrent(struct pdb_methods *my_methods)
{
	ldapsam_endsampwent(my_methods);
}

/**********************************************************************
 *********************************************************************/

static NTSTATUS ldapsam_getsamgrent(struct pdb_methods *my_methods,
				    GROUP_MAP *map)
{
	NTSTATUS ret = NT_STATUS_UNSUCCESSFUL;
	struct ldapsam_privates *ldap_state = (struct ldapsam_privates *)my_methods->private_data;
	BOOL bret = False;

	while (!bret) {
		if (!ldap_state->entry)
			return ret;
		
		ldap_state->index++;
		bret = init_group_from_ldap(ldap_state, map, ldap_state->entry);
		
		ldap_state->entry = ldap_next_entry(ldap_state->smbldap_state->ldap_struct,
					    ldap_state->entry);	
	}

	return NT_STATUS_OK;
}

/**********************************************************************
 *********************************************************************/

static NTSTATUS ldapsam_enum_group_mapping(struct pdb_methods *methods,
					   enum SID_NAME_USE sid_name_use,
					   GROUP_MAP **pp_rmap, size_t *p_num_entries,
					   BOOL unix_only)
{
	GROUP_MAP map;
	GROUP_MAP *mapt;
	size_t entries = 0;

	*p_num_entries = 0;
	*pp_rmap = NULL;

	if (!NT_STATUS_IS_OK(ldapsam_setsamgrent(methods, False))) {
		DEBUG(0, ("ldapsam_enum_group_mapping: Unable to open passdb\n"));
		return NT_STATUS_ACCESS_DENIED;
	}

	while (NT_STATUS_IS_OK(ldapsam_getsamgrent(methods, &map))) {
		if (sid_name_use != SID_NAME_UNKNOWN &&
		    sid_name_use != map.sid_name_use) {
			DEBUG(11,("ldapsam_enum_group_mapping: group %s is not of the requested type\n", map.nt_name));
			continue;
		}
		if (unix_only==ENUM_ONLY_MAPPED && map.gid==-1) {
			DEBUG(11,("ldapsam_enum_group_mapping: group %s is non mapped\n", map.nt_name));
			continue;
		}

		mapt=SMB_REALLOC_ARRAY((*pp_rmap), GROUP_MAP, entries+1);
		if (!mapt) {
			DEBUG(0,("ldapsam_enum_group_mapping: Unable to enlarge group map!\n"));
			SAFE_FREE(*pp_rmap);
			return NT_STATUS_UNSUCCESSFUL;
		}
		else
			(*pp_rmap) = mapt;

		mapt[entries] = map;

		entries += 1;

	}
	ldapsam_endsamgrent(methods);

	*p_num_entries = entries;

	return NT_STATUS_OK;
}

static NTSTATUS ldapsam_modify_aliasmem(struct pdb_methods *methods,
					const DOM_SID *alias,
					const DOM_SID *member,
					int modop)
{
	struct ldapsam_privates *ldap_state =
		(struct ldapsam_privates *)methods->private_data;
	char *dn;
	LDAPMessage *result = NULL;
	LDAPMessage *entry = NULL;
	int count;
	LDAPMod **mods = NULL;
	int rc;

	pstring filter;

	pstr_sprintf(filter, "(&(|(objectClass=%s)(objectclass=%s))(%s=%s))",
		     LDAP_OBJ_GROUPMAP, LDAP_OBJ_IDMAP_ENTRY,
		     get_attr_key2string(groupmap_attr_list,
					 LDAP_ATTR_GROUP_SID),
		     sid_string_static(alias));

	if (ldapsam_search_one_group(ldap_state, filter,
				     &result) != LDAP_SUCCESS)
		return NT_STATUS_NO_SUCH_ALIAS;

	count = ldap_count_entries(ldap_state->smbldap_state->ldap_struct,
				   result);

	if (count < 1) {
		DEBUG(4, ("ldapsam_modify_aliasmem: Did not find alias\n"));
		ldap_msgfree(result);
		return NT_STATUS_NO_SUCH_ALIAS;
	}

	if (count > 1) {
		DEBUG(1, ("ldapsam_modify_aliasmem: Duplicate entries for filter %s: "
			  "count=%d\n", filter, count));
		ldap_msgfree(result);
		return NT_STATUS_NO_SUCH_ALIAS;
	}

	entry = ldap_first_entry(ldap_state->smbldap_state->ldap_struct,
				 result);

	if (!entry) {
		ldap_msgfree(result);
		return NT_STATUS_UNSUCCESSFUL;
	}

	dn = smbldap_get_dn(ldap_state->smbldap_state->ldap_struct, entry);
	if (!dn) {
		ldap_msgfree(result);
		return NT_STATUS_UNSUCCESSFUL;
	}

	smbldap_set_mod(&mods, modop,
			get_attr_key2string(groupmap_attr_list,
					    LDAP_ATTR_SID_LIST),
			sid_string_static(member));

	rc = smbldap_modify(ldap_state->smbldap_state, dn, mods);

	ldap_mods_free(mods, True);
	ldap_msgfree(result);

	if (rc != LDAP_SUCCESS) {
		char *ld_error = NULL;
		ldap_get_option(ldap_state->smbldap_state->ldap_struct,
				LDAP_OPT_ERROR_STRING,&ld_error);
		
		DEBUG(0, ("ldapsam_modify_aliasmem: Could not modify alias "
			  "for %s, error: %s (%s)\n", dn, ldap_err2string(rc),
			  ld_error?ld_error:"unknown"));
		SAFE_FREE(ld_error);
		SAFE_FREE(dn);
		return NT_STATUS_UNSUCCESSFUL;
	}

	SAFE_FREE(dn);

	return NT_STATUS_OK;
}

static NTSTATUS ldapsam_add_aliasmem(struct pdb_methods *methods,
				     const DOM_SID *alias,
				     const DOM_SID *member)
{
	return ldapsam_modify_aliasmem(methods, alias, member, LDAP_MOD_ADD);
}

static NTSTATUS ldapsam_del_aliasmem(struct pdb_methods *methods,
				     const DOM_SID *alias,
				     const DOM_SID *member)
{
	return ldapsam_modify_aliasmem(methods, alias, member,
				       LDAP_MOD_DELETE);
}

static NTSTATUS ldapsam_enum_aliasmem(struct pdb_methods *methods,
				      const DOM_SID *alias, DOM_SID **pp_members,
				      size_t *p_num_members)
{
	struct ldapsam_privates *ldap_state =
		(struct ldapsam_privates *)methods->private_data;
	LDAPMessage *result = NULL;
	LDAPMessage *entry = NULL;
	int count;
	char **values;
	int i;
	pstring filter;
	size_t num_members = 0;

	*pp_members = NULL;
	*p_num_members = 0;

	pstr_sprintf(filter, "(&(|(objectClass=%s)(objectclass=%s))(%s=%s))",
		     LDAP_OBJ_GROUPMAP, LDAP_OBJ_IDMAP_ENTRY,
		     get_attr_key2string(groupmap_attr_list,
					 LDAP_ATTR_GROUP_SID),
		     sid_string_static(alias));

	if (ldapsam_search_one_group(ldap_state, filter,
				     &result) != LDAP_SUCCESS)
		return NT_STATUS_NO_SUCH_ALIAS;

	count = ldap_count_entries(ldap_state->smbldap_state->ldap_struct,
				   result);

	if (count < 1) {
		DEBUG(4, ("ldapsam_enum_aliasmem: Did not find alias\n"));
		ldap_msgfree(result);
		return NT_STATUS_NO_SUCH_ALIAS;
	}

	if (count > 1) {
		DEBUG(1, ("ldapsam_enum_aliasmem: Duplicate entries for filter %s: "
			  "count=%d\n", filter, count));
		ldap_msgfree(result);
		return NT_STATUS_NO_SUCH_ALIAS;
	}

	entry = ldap_first_entry(ldap_state->smbldap_state->ldap_struct,
				 result);

	if (!entry) {
		ldap_msgfree(result);
		return NT_STATUS_UNSUCCESSFUL;
	}

	values = ldap_get_values(ldap_state->smbldap_state->ldap_struct,
				 entry,
				 get_attr_key2string(groupmap_attr_list,
						     LDAP_ATTR_SID_LIST));

	if (values == NULL) {
		ldap_msgfree(result);
		return NT_STATUS_OK;
	}

	count = ldap_count_values(values);

	for (i=0; i<count; i++) {
		DOM_SID member;

		if (!string_to_sid(&member, values[i]))
			continue;

		add_sid_to_array(NULL, &member, pp_members, &num_members);
	}

	*p_num_members = num_members;
	ldap_value_free(values);
	ldap_msgfree(result);

	return NT_STATUS_OK;
}

static NTSTATUS ldapsam_alias_memberships(struct pdb_methods *methods,
					  TALLOC_CTX *mem_ctx,
					  const DOM_SID *domain_sid,
					  const DOM_SID *members,
					  size_t num_members,
					  uint32 **pp_alias_rids,
					  size_t *p_num_alias_rids)
{
	struct ldapsam_privates *ldap_state =
		(struct ldapsam_privates *)methods->private_data;
	LDAP *ldap_struct;

	const char *attrs[] = { LDAP_ATTRIBUTE_SID, NULL };

	LDAPMessage *result = NULL;
	LDAPMessage *entry = NULL;
	int i;
	int rc;
	char *filter;

	/* This query could be further optimized by adding a
	   (&(sambaSID=<domain-sid>*)) so that only those aliases that are
	   asked for in the getuseraliases are returned. */	   

	filter = talloc_asprintf(mem_ctx,
				 "(&(|(objectclass=%s)(objectclass=%s))(|",
				 LDAP_OBJ_GROUPMAP, LDAP_OBJ_IDMAP_ENTRY);

	for (i=0; i<num_members; i++)
		filter = talloc_asprintf(mem_ctx, "%s(sambaSIDList=%s)",
					 filter,
					 sid_string_static(&members[i]));

	filter = talloc_asprintf(mem_ctx, "%s))", filter);

	rc = smbldap_search(ldap_state->smbldap_state, lp_ldap_group_suffix(),
			    LDAP_SCOPE_SUBTREE, filter, attrs, 0, &result);

	if (rc != LDAP_SUCCESS)
		return NT_STATUS_UNSUCCESSFUL;

	ldap_struct = ldap_state->smbldap_state->ldap_struct;

	for (entry = ldap_first_entry(ldap_struct, result);
	     entry != NULL;
	     entry = ldap_next_entry(ldap_struct, entry))
	{
		fstring sid_str;
		DOM_SID sid;
		uint32 rid;

		if (!smbldap_get_single_attribute(ldap_struct, entry,
						  LDAP_ATTRIBUTE_SID,
						  sid_str,
						  sizeof(sid_str)-1))
			continue;

		if (!string_to_sid(&sid, sid_str))
			continue;

		if (!sid_peek_check_rid(domain_sid, &sid, &rid))
			continue;

		add_rid_to_array_unique(mem_ctx, rid, pp_alias_rids,
					p_num_alias_rids);
	}

	ldap_msgfree(result);
	return NT_STATUS_OK;
}

static NTSTATUS ldapsam_set_account_policy_in_ldap(struct pdb_methods *methods, int policy_index, uint32 value)
{
	NTSTATUS ntstatus = NT_STATUS_UNSUCCESSFUL;
	int rc;
	LDAPMod **mods = NULL;
	fstring value_string;
	const char *policy_attr = NULL;

	struct ldapsam_privates *ldap_state =
		(struct ldapsam_privates *)methods->private_data;

	const char *attrs[2];

	DEBUG(10,("ldapsam_set_account_policy_in_ldap\n"));

	if (!ldap_state->domain_dn) {
		return NT_STATUS_INVALID_PARAMETER;
	}

	policy_attr = get_account_policy_attr(policy_index);
	if (policy_attr == NULL) {
		DEBUG(0,("ldapsam_set_account_policy_in_ldap: invalid policy\n"));
		return ntstatus;
	}

	attrs[0] = policy_attr;
	attrs[1] = NULL;

	slprintf(value_string, sizeof(value_string) - 1, "%i", value);

	smbldap_set_mod(&mods, LDAP_MOD_REPLACE, policy_attr, value_string);

	rc = smbldap_modify(ldap_state->smbldap_state, ldap_state->domain_dn, mods);

	ldap_mods_free(mods, True);

	if (rc != LDAP_SUCCESS) {
		char *ld_error = NULL;
		ldap_get_option(ldap_state->smbldap_state->ldap_struct,
				LDAP_OPT_ERROR_STRING,&ld_error);
		
		DEBUG(0, ("ldapsam_set_account_policy_in_ldap: Could not set account policy "
			  "for %s, error: %s (%s)\n", ldap_state->domain_dn, ldap_err2string(rc),
			  ld_error?ld_error:"unknown"));
		SAFE_FREE(ld_error);
		return ntstatus;
	}

	if (!cache_account_policy_set(policy_index, value)) {
		DEBUG(0,("ldapsam_set_account_policy_in_ldap: failed to update local tdb cache\n"));
		return ntstatus;
	}

	return NT_STATUS_OK;
}

static NTSTATUS ldapsam_set_account_policy(struct pdb_methods *methods, int policy_index, uint32 value)
{
	if (!account_policy_migrated(False)) {
		return (account_policy_set(policy_index, value)) ? NT_STATUS_OK : NT_STATUS_UNSUCCESSFUL;
	}

	return ldapsam_set_account_policy_in_ldap(methods, policy_index, value);
}

static NTSTATUS ldapsam_get_account_policy_from_ldap(struct pdb_methods *methods, int policy_index, uint32 *value)
{
	NTSTATUS ntstatus = NT_STATUS_UNSUCCESSFUL;
	LDAPMessage *result = NULL;
	LDAPMessage *entry = NULL;
	int count;
	int rc;
	char **vals = NULL;
	const char *policy_attr = NULL;

	struct ldapsam_privates *ldap_state =
		(struct ldapsam_privates *)methods->private_data;

	const char *attrs[2];

	DEBUG(10,("ldapsam_get_account_policy_from_ldap\n"));

	if (!ldap_state->domain_dn) {
		return NT_STATUS_INVALID_PARAMETER;
	}

	policy_attr = get_account_policy_attr(policy_index);
	if (!policy_attr) {
		DEBUG(0,("ldapsam_get_account_policy_from_ldap: invalid policy index: %d\n", policy_index));
		return ntstatus;
	}

	attrs[0] = policy_attr;
	attrs[1] = NULL;

	rc = smbldap_search(ldap_state->smbldap_state, ldap_state->domain_dn,
			    LDAP_SCOPE_BASE, "(objectclass=*)", attrs, 0, &result);

	if (rc != LDAP_SUCCESS) {
		char *ld_error = NULL;
		ldap_get_option(ldap_state->smbldap_state->ldap_struct,
				LDAP_OPT_ERROR_STRING,&ld_error);
		
		DEBUG(3, ("ldapsam_get_account_policy_from_ldap: Could not get account policy "
			  "for %s, error: %s (%s)\n", ldap_state->domain_dn, ldap_err2string(rc),
			  ld_error?ld_error:"unknown"));
		SAFE_FREE(ld_error);
		return ntstatus;
	}

	count = ldap_count_entries(ldap_state->smbldap_state->ldap_struct, result);
	if (count < 1) {
		goto out;
	}

	entry = ldap_first_entry(ldap_state->smbldap_state->ldap_struct, result);
	if (entry == NULL) {
		goto out;
	}

	vals = ldap_get_values(ldap_state->smbldap_state->ldap_struct, entry, policy_attr);
	if (vals == NULL) {
		goto out;
	}

	*value = (uint32)atol(vals[0]);
	
	ntstatus = NT_STATUS_OK;

out:
	if (vals)
		ldap_value_free(vals);
	ldap_msgfree(result);

	return ntstatus;
}

/* wrapper around ldapsam_get_account_policy_from_ldap(), handles tdb as cache 

   - if user hasn't decided to use account policies inside LDAP just reuse the old tdb values
   
   - if there is a valid cache entry, return that
   - if there is an LDAP entry, update cache and return 
   - otherwise set to default, update cache and return

   Guenther
*/
static NTSTATUS ldapsam_get_account_policy(struct pdb_methods *methods, int policy_index, uint32 *value)
{
	NTSTATUS ntstatus = NT_STATUS_UNSUCCESSFUL;

	if (!account_policy_migrated(False)) {
		return (account_policy_get(policy_index, value)) ? NT_STATUS_OK : NT_STATUS_UNSUCCESSFUL;
	}

	if (cache_account_policy_get(policy_index, value)) {
		DEBUG(11,("ldapsam_get_account_policy: got valid value from cache\n"));
		return NT_STATUS_OK;
	}

	ntstatus = ldapsam_get_account_policy_from_ldap(methods, policy_index, value);
	if (NT_STATUS_IS_OK(ntstatus)) {
		goto update_cache;
	}

	DEBUG(10,("ldapsam_get_account_policy: failed to retrieve from ldap\n"));

#if 0
	/* should we automagically migrate old tdb value here ? */
	if (account_policy_get(policy_index, value))
		goto update_ldap;

	DEBUG(10,("ldapsam_get_account_policy: no tdb for %d, trying default\n", policy_index));
#endif

	if (!account_policy_get_default(policy_index, value)) {
		return ntstatus;
	}
	
/* update_ldap: */
 
 	ntstatus = ldapsam_set_account_policy(methods, policy_index, *value);
	if (!NT_STATUS_IS_OK(ntstatus)) {
		return ntstatus;
	}
		
 update_cache:
 
	if (!cache_account_policy_set(policy_index, *value)) {
		DEBUG(0,("ldapsam_get_account_policy: failed to update local tdb as a cache\n"));
		return NT_STATUS_UNSUCCESSFUL;
	}

	return NT_STATUS_OK;
}

static NTSTATUS ldapsam_lookup_rids(struct pdb_methods *methods,
				    const DOM_SID *domain_sid,
				    int num_rids,
				    uint32 *rids,
				    const char **names,
				    uint32 *attrs)
{
	struct ldapsam_privates *ldap_state =
		(struct ldapsam_privates *)methods->private_data;
	LDAP *ldap_struct = ldap_state->smbldap_state->ldap_struct;
	LDAPMessage *msg = NULL;
	LDAPMessage *entry;
	char *allsids = NULL;
	char *tmp;
	int i, rc, num_mapped;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	if (!lp_parm_bool(-1, "ldapsam", "trusted", False))
		return pdb_default_lookup_rids(methods, domain_sid,
					       num_rids, rids, names, attrs);

	if (!sid_equal(domain_sid, get_global_sam_sid())) {
		/* TODO: Sooner or later we need to look up BUILTIN rids as
		 * well. -- vl */
		goto done;
	}

	for (i=0; i<num_rids; i++)
		attrs[i] = SID_NAME_UNKNOWN;

	allsids = SMB_STRDUP("");
	if (allsids == NULL) return NT_STATUS_NO_MEMORY;

	for (i=0; i<num_rids; i++) {
		DOM_SID sid;
		sid_copy(&sid, domain_sid);
		sid_append_rid(&sid, rids[i]);
		tmp = allsids;
		asprintf(&allsids, "%s(sambaSid=%s)", allsids,
			 sid_string_static(&sid));
		if (allsids == NULL) return NT_STATUS_NO_MEMORY;
		free(tmp);
	}

	/* First look for users */

	{
		char *filter;
		const char *ldap_attrs[] = { "uid", "sambaSid", NULL };

		asprintf(&filter, ("(&(objectClass=sambaSamAccount)(|%s))"),
			 allsids);
		if (filter == NULL) return NT_STATUS_NO_MEMORY;

		rc = smbldap_search(ldap_state->smbldap_state,
				    lp_ldap_user_suffix(),
				    LDAP_SCOPE_SUBTREE, filter, ldap_attrs, 0,
				    &msg);

		SAFE_FREE(filter);
	}

	if (rc != LDAP_SUCCESS)
		goto done;

	num_mapped = 0;

	for (entry = ldap_first_entry(ldap_struct, msg);
	     entry != NULL;
	     entry = ldap_next_entry(ldap_struct, entry))
	{
		uint32 rid;
		int rid_index;
		fstring str;

		if (!ldapsam_extract_rid_from_entry(ldap_struct, entry,
						    get_global_sam_sid(),
						    &rid)) {
			DEBUG(2, ("Could not find sid from ldap entry\n"));
			continue;
		}

		if (!smbldap_get_single_attribute(ldap_struct, entry,
						  "uid", str, sizeof(str)-1)) {
			DEBUG(2, ("Could not retrieve uid attribute\n"));
			continue;
		}

		for (rid_index = 0; rid_index < num_rids; rid_index++) {
			if (rid == rids[rid_index])
				break;
		}

		if (rid_index == num_rids) {
			DEBUG(2, ("Got a RID not asked for: %d\n", rid));
			continue;
		}

		attrs[rid_index] = SID_NAME_USER;
		names[rid_index] = talloc_strdup(names, str);
		if (names[rid_index] == NULL) return NT_STATUS_NO_MEMORY;

		num_mapped += 1;
	}

	if (num_mapped == num_rids) {
		/* No need to look for groups anymore -- we're done */
		result = NT_STATUS_OK;
		goto done;
	}

	if (msg != NULL) {
		ldap_msgfree(msg);
	}

	/* Same game for groups */

	{
		char *filter;
		const char *ldap_attrs[] = { "cn", "sambaSid", NULL };

		asprintf(&filter, ("(&(objectClass=sambaGroupMapping)(|%s))"),
			 allsids);
		if (filter == NULL) return NT_STATUS_NO_MEMORY;

		rc = smbldap_search(ldap_state->smbldap_state,
				    lp_ldap_group_suffix(),
				    LDAP_SCOPE_SUBTREE, filter, ldap_attrs, 0,
				    &msg);

		SAFE_FREE(filter);
	}

	if (rc != LDAP_SUCCESS)
		goto done;

	for (entry = ldap_first_entry(ldap_struct, msg);
	     entry != NULL;
	     entry = ldap_next_entry(ldap_struct, entry))
	{
		uint32 rid;
		int rid_index;
		fstring str;

		if (!ldapsam_extract_rid_from_entry(ldap_struct, entry,
						    get_global_sam_sid(),
						    &rid)) {
			DEBUG(2, ("Could not find sid from ldap entry\n"));
			continue;
		}

		if (!smbldap_get_single_attribute(ldap_struct, entry,
						  "cn", str, sizeof(str)-1)) {
			DEBUG(2, ("Could not retrieve cn attribute\n"));
			continue;
		}

		for (rid_index = 0; rid_index < num_rids; rid_index++) {
			if (rid == rids[rid_index])
				break;
		}

		if (rid_index == num_rids) {
			DEBUG(2, ("Got a RID not asked for: %d\n", rid));
			continue;
		}

		attrs[rid_index] = SID_NAME_DOM_GRP;
		names[rid_index] = talloc_strdup(names, str);
		if (names[rid_index] == NULL) return NT_STATUS_NO_MEMORY;
		num_mapped += 1;
	}

	result = NT_STATUS_NONE_MAPPED;

	if (num_mapped > 0)
		result = (num_mapped == num_rids) ?
			NT_STATUS_OK : STATUS_SOME_UNMAPPED;
 done:
	SAFE_FREE(allsids);

	if (msg != NULL)
		ldap_msgfree(msg);

	return result;
}

char *get_ldap_filter(TALLOC_CTX *mem_ctx, const char *username)
{
	char *filter = NULL;
	char *escaped = NULL;
	char *result = NULL;

	asprintf(&filter, "(&%s(objectclass=sambaSamAccount))",
		 "(uid=%u)");
	if (filter == NULL) goto done;

	escaped = escape_ldap_string_alloc(username);
	if (escaped == NULL) goto done;

	filter = realloc_string_sub(filter, "%u", username);
	result = talloc_strdup(mem_ctx, filter);

 done:
	SAFE_FREE(filter);
	SAFE_FREE(escaped);

	return result;
}

const char **talloc_attrs(TALLOC_CTX *mem_ctx, ...)
{
	int i, num = 0;
	va_list ap;
	const char **result;

	va_start(ap, mem_ctx);
	while (va_arg(ap, const char *) != NULL)
		num += 1;
	va_end(ap);

	result = TALLOC_ARRAY(mem_ctx, const char *, num+1);

	va_start(ap, mem_ctx);
	for (i=0; i<num; i++)
		result[i] = talloc_strdup(mem_ctx, va_arg(ap, const char*));
	va_end(ap);

	result[num] = NULL;
	return result;
}

struct ldap_search_state {
	struct smbldap_state *connection;

	uint16 acct_flags;
	uint16 group_type;

	const char *base;
	int scope;
	const char *filter;
	const char **attrs;
	int attrsonly;
	void *pagedresults_cookie;

	LDAPMessage *entries, *current_entry;
	BOOL (*ldap2displayentry)(struct ldap_search_state *state,
				  TALLOC_CTX *mem_ctx,
				  LDAP *ld, LDAPMessage *entry,
				  struct samr_displayentry *result);
};

static BOOL ldapsam_search_firstpage(struct pdb_search *search)
{
	struct ldap_search_state *state = search->private_data;
	LDAP *ld;
	int rc = LDAP_OPERATIONS_ERROR;

	state->entries = NULL;

	if (state->connection->paged_results) {
		rc = smbldap_search_paged(state->connection, state->base,
					  state->scope, state->filter,
					  state->attrs, state->attrsonly,
					  lp_ldap_page_size(), &state->entries,
					  &state->pagedresults_cookie);
	}

	if ((rc != LDAP_SUCCESS) || (state->entries == NULL)) {

		if (state->entries != NULL) {
			/* Left over from unsuccessful paged attempt */
			ldap_msgfree(state->entries);
			state->entries = NULL;
		}

		rc = smbldap_search(state->connection, state->base,
				    state->scope, state->filter, state->attrs,
				    state->attrsonly, &state->entries);

		if ((rc != LDAP_SUCCESS) || (state->entries == NULL))
			return False;

		/* Ok, the server was lying. It told us it could do paged
		 * searches when it could not. */
		state->connection->paged_results = False;
	}

        ld = state->connection->ldap_struct;
        if ( ld == NULL) {
                DEBUG(5, ("Don't have an LDAP connection right after a "
			  "search\n"));
                return False;
        }
        state->current_entry = ldap_first_entry(ld, state->entries);

	if (state->current_entry == NULL) {
		ldap_msgfree(state->entries);
		state->entries = NULL;
	}

	return True;
}

static BOOL ldapsam_search_nextpage(struct pdb_search *search)
{
	struct ldap_search_state *state = search->private_data;
	LDAP *ld = state->connection->ldap_struct;
	int rc;

	if (!state->connection->paged_results) {
		/* There is no next page when there are no paged results */
		return False;
	}

	rc = smbldap_search_paged(state->connection, state->base,
				  state->scope, state->filter, state->attrs,
				  state->attrsonly, lp_ldap_page_size(),
				  &state->entries,
				  &state->pagedresults_cookie);

	if ((rc != LDAP_SUCCESS) || (state->entries == NULL))
		return False;

	state->current_entry = ldap_first_entry(ld, state->entries);

	if (state->current_entry == NULL) {
		ldap_msgfree(state->entries);
		state->entries = NULL;
	}

	return True;
}

static BOOL ldapsam_search_next_entry(struct pdb_search *search,
				      struct samr_displayentry *entry)
{
	struct ldap_search_state *state = search->private_data;
	LDAP *ld = state->connection->ldap_struct;
	BOOL result;

 retry:
	if ((state->entries == NULL) && (state->pagedresults_cookie == NULL))
		return False;

	if ((state->entries == NULL) &&
	    !ldapsam_search_nextpage(search))
		    return False;

	result = state->ldap2displayentry(state, search->mem_ctx, ld,
					  state->current_entry, entry);

	if (!result) {
		char *dn;
		dn = ldap_get_dn(ld, state->current_entry);
		DEBUG(5, ("Skipping entry %s\n", dn != NULL ? dn : "<NULL>"));
		if (dn != NULL) ldap_memfree(dn);
	}

	state->current_entry = ldap_next_entry(ld, state->current_entry);

	if (state->current_entry == NULL) {
		ldap_msgfree(state->entries);
		state->entries = NULL;
	}

	if (!result) goto retry;

	return True;
}

static void ldapsam_search_end(struct pdb_search *search)
{
	struct ldap_search_state *state = search->private_data;
	int rc;

	if (state->pagedresults_cookie == NULL)
		return;

	if (state->entries != NULL)
		ldap_msgfree(state->entries);

	state->entries = NULL;
	state->current_entry = NULL;

	if (!state->connection->paged_results)
		return;

	/* Tell the LDAP server we're not interested in the rest anymore. */

	rc = smbldap_search_paged(state->connection, state->base, state->scope,
				  state->filter, state->attrs,
				  state->attrsonly, 0, &state->entries,
				  &state->pagedresults_cookie);

	if (rc != LDAP_SUCCESS)
		DEBUG(5, ("Could not end search properly\n"));

	return;
}

static BOOL ldapuser2displayentry(struct ldap_search_state *state,
				  TALLOC_CTX *mem_ctx,
				  LDAP *ld, LDAPMessage *entry,
				  struct samr_displayentry *result)
{
	char **vals;
	DOM_SID sid;
	uint16 acct_flags;

	vals = ldap_get_values(ld, entry, "sambaAcctFlags");
	if ((vals == NULL) || (vals[0] == NULL)) {
		DEBUG(5, ("\"sambaAcctFlags\" not found\n"));
		return False;
	}
	acct_flags = pdb_decode_acct_ctrl(vals[0]);
	ldap_value_free(vals);

	if ((state->acct_flags != 0) &&
	    ((state->acct_flags & acct_flags) == 0))
		return False;		

	result->acct_flags = acct_flags;
	result->account_name = "";
	result->fullname = "";
	result->description = "";

	vals = ldap_get_values(ld, entry, "uid");
	if ((vals == NULL) || (vals[0] == NULL)) {
		DEBUG(5, ("\"uid\" not found\n"));
		return False;
	}
	pull_utf8_talloc(mem_ctx,
			 CONST_DISCARD(char **, &result->account_name),
			 vals[0]);
	ldap_value_free(vals);

	vals = ldap_get_values(ld, entry, "displayName");
	if ((vals == NULL) || (vals[0] == NULL))
		DEBUG(8, ("\"displayName\" not found\n"));
	else
		pull_utf8_talloc(mem_ctx,
				 CONST_DISCARD(char **, &result->fullname),
				 vals[0]);
	ldap_value_free(vals);

	vals = ldap_get_values(ld, entry, "description");
	if ((vals == NULL) || (vals[0] == NULL))
		DEBUG(8, ("\"description\" not found\n"));
	else
		pull_utf8_talloc(mem_ctx,
				 CONST_DISCARD(char **, &result->description),
				 vals[0]);
	ldap_value_free(vals);

	if ((result->account_name == NULL) ||
	    (result->fullname == NULL) ||
	    (result->description == NULL)) {
		DEBUG(0, ("talloc failed\n"));
		return False;
	}
	
	vals = ldap_get_values(ld, entry, "sambaSid");
	if ((vals == NULL) || (vals[0] == NULL)) {
		DEBUG(0, ("\"objectSid\" not found\n"));
		return False;
	}

	if (!string_to_sid(&sid, vals[0])) {
		DEBUG(0, ("Could not convert %s to SID\n", vals[0]));
		ldap_value_free(vals);
		return False;
	}
	ldap_value_free(vals);

	if (!sid_peek_check_rid(get_global_sam_sid(), &sid, &result->rid)) {
		DEBUG(0, ("sid %s does not belong to our domain\n", sid_string_static(&sid)));
		return False;
	}

	return True;
}


static BOOL ldapsam_search_users(struct pdb_methods *methods,
				 struct pdb_search *search,
				 uint16 acct_flags)
{
	struct ldapsam_privates *ldap_state = methods->private_data;
	struct ldap_search_state *state;

	state = TALLOC_P(search->mem_ctx, struct ldap_search_state);
	if (state == NULL) {
		DEBUG(0, ("talloc failed\n"));
		return False;
	}

	state->connection = ldap_state->smbldap_state;

	if ((acct_flags != 0) && ((acct_flags & ACB_NORMAL) != 0))
		state->base = lp_ldap_user_suffix();
	else if ((acct_flags != 0) &&
		 ((acct_flags & (ACB_WSTRUST|ACB_SVRTRUST|ACB_DOMTRUST)) != 0))
		state->base = lp_ldap_machine_suffix();
	else
		state->base = lp_ldap_suffix();

	state->acct_flags = acct_flags;
	state->base = talloc_strdup(search->mem_ctx, state->base);
	state->scope = LDAP_SCOPE_SUBTREE;
	state->filter = get_ldap_filter(search->mem_ctx, "*");
	state->attrs = talloc_attrs(search->mem_ctx, "uid", "sambaSid",
				    "displayName", "description",
				    "sambaAcctFlags", NULL);
	state->attrsonly = 0;
	state->pagedresults_cookie = NULL;
	state->entries = NULL;
	state->ldap2displayentry = ldapuser2displayentry;

	if ((state->filter == NULL) || (state->attrs == NULL)) {
		DEBUG(0, ("talloc failed\n"));
		return False;
	}

	search->private_data = state;
	search->next_entry = ldapsam_search_next_entry;
	search->search_end = ldapsam_search_end;

	return ldapsam_search_firstpage(search);
}

static BOOL ldapgroup2displayentry(struct ldap_search_state *state,
				   TALLOC_CTX *mem_ctx,
				   LDAP *ld, LDAPMessage *entry,
				   struct samr_displayentry *result)
{
	char **vals;
	DOM_SID sid;
	uint16 group_type;

	result->account_name = "";
	result->fullname = "";
	result->description = "";


	vals = ldap_get_values(ld, entry, "sambaGroupType");
	if ((vals == NULL) || (vals[0] == NULL)) {
		DEBUG(5, ("\"sambaGroupType\" not found\n"));
		if (vals != NULL) {
			ldap_value_free(vals);
		}
		return False;
	}

	group_type = atoi(vals[0]);

	if ((state->group_type != 0) &&
	    ((state->group_type != group_type))) {
		ldap_value_free(vals);
		return False;
	}

	ldap_value_free(vals);

	/* display name is the NT group name */

	vals = ldap_get_values(ld, entry, "displayName");
	if ((vals == NULL) || (vals[0] == NULL)) {
		DEBUG(8, ("\"displayName\" not found\n"));

		/* fallback to the 'cn' attribute */
		vals = ldap_get_values(ld, entry, "cn");
		if ((vals == NULL) || (vals[0] == NULL)) {
			DEBUG(5, ("\"cn\" not found\n"));
			return False;
		}
		pull_utf8_talloc(mem_ctx, CONST_DISCARD(char **, &result->account_name), vals[0]);
	}
	else {
		pull_utf8_talloc(mem_ctx, CONST_DISCARD(char **, &result->account_name), vals[0]);
	}

	ldap_value_free(vals);

	vals = ldap_get_values(ld, entry, "description");
	if ((vals == NULL) || (vals[0] == NULL))
		DEBUG(8, ("\"description\" not found\n"));
	else
		pull_utf8_talloc(mem_ctx,
				 CONST_DISCARD(char **, &result->description),
				 vals[0]);
	ldap_value_free(vals);

	if ((result->account_name == NULL) ||
	    (result->fullname == NULL) ||
	    (result->description == NULL)) {
		DEBUG(0, ("talloc failed\n"));
		return False;
	}
	
	vals = ldap_get_values(ld, entry, "sambaSid");
	if ((vals == NULL) || (vals[0] == NULL)) {
		DEBUG(0, ("\"objectSid\" not found\n"));
		if (vals != NULL) {
			ldap_value_free(vals);
		}
		return False;
	}

	if (!string_to_sid(&sid, vals[0])) {
		DEBUG(0, ("Could not convert %s to SID\n", vals[0]));
		return False;
	}

	ldap_value_free(vals);

	switch (group_type) {
		case SID_NAME_DOM_GRP:
		case SID_NAME_ALIAS:

			if (!sid_peek_check_rid(get_global_sam_sid(), &sid, &result->rid)) {
				DEBUG(0, ("%s is not in our domain\n", sid_string_static(&sid)));
				return False;
			}
			break;
	
		case SID_NAME_WKN_GRP:

			if (!sid_peek_check_rid(&global_sid_Builtin, &sid, &result->rid)) {

				DEBUG(0, ("%s is not in builtin sid\n", sid_string_static(&sid)));
				return False;
			}
			break;

		default:
			DEBUG(0,("unkown group type: %d\n", group_type));
			return False;
	}
	
	return True;
}

static BOOL ldapsam_search_grouptype(struct pdb_methods *methods,
				     struct pdb_search *search,
				     enum SID_NAME_USE type)
{
	struct ldapsam_privates *ldap_state = methods->private_data;
	struct ldap_search_state *state;

	state = TALLOC_P(search->mem_ctx, struct ldap_search_state);
	if (state == NULL) {
		DEBUG(0, ("talloc failed\n"));
		return False;
	}

	state->connection = ldap_state->smbldap_state;

	state->base = talloc_strdup(search->mem_ctx, lp_ldap_group_suffix());
	state->connection = ldap_state->smbldap_state;
	state->scope = LDAP_SCOPE_SUBTREE;
	state->filter =	talloc_asprintf(search->mem_ctx,
					"(&(objectclass=sambaGroupMapping)"
					"(sambaGroupType=%d))", type);
	state->attrs = talloc_attrs(search->mem_ctx, "cn", "sambaSid",
				    "displayName", "description", "sambaGroupType", NULL);
	state->attrsonly = 0;
	state->pagedresults_cookie = NULL;
	state->entries = NULL;
	state->group_type = type;
	state->ldap2displayentry = ldapgroup2displayentry;

	if ((state->filter == NULL) || (state->attrs == NULL)) {
		DEBUG(0, ("talloc failed\n"));
		return False;
	}

	search->private_data = state;
	search->next_entry = ldapsam_search_next_entry;
	search->search_end = ldapsam_search_end;

	return ldapsam_search_firstpage(search);
}

static BOOL ldapsam_search_groups(struct pdb_methods *methods,
				  struct pdb_search *search)
{
	return ldapsam_search_grouptype(methods, search, SID_NAME_DOM_GRP);
}

static BOOL ldapsam_search_aliases(struct pdb_methods *methods,
				   struct pdb_search *search,
				   const DOM_SID *sid)
{
	if (sid_check_is_domain(sid))
		return ldapsam_search_grouptype(methods, search,
						SID_NAME_ALIAS);

	if (sid_check_is_builtin(sid))
		return ldapsam_search_grouptype(methods, search,
						SID_NAME_WKN_GRP);

	DEBUG(5, ("Don't know SID %s\n", sid_string_static(sid)));
	return False;
}

/**********************************************************************
 Housekeeping
 *********************************************************************/

static void free_private_data(void **vp) 
{
	struct ldapsam_privates **ldap_state = (struct ldapsam_privates **)vp;

	smbldap_free_struct(&(*ldap_state)->smbldap_state);

	if ((*ldap_state)->result != NULL) {
		ldap_msgfree((*ldap_state)->result);
		(*ldap_state)->result = NULL;
	}
	if ((*ldap_state)->domain_dn != NULL) {
		SAFE_FREE((*ldap_state)->domain_dn);
	}

	*ldap_state = NULL;

	/* No need to free any further, as it is talloc()ed */
}

/**********************************************************************
 Intitalise the parts of the pdb_context that are common to all pdb_ldap modes
 *********************************************************************/

static NTSTATUS pdb_init_ldapsam_common(PDB_CONTEXT *pdb_context, PDB_METHODS **pdb_method, 
					const char *location)
{
	NTSTATUS nt_status;
	struct ldapsam_privates *ldap_state;

	if (!NT_STATUS_IS_OK(nt_status = make_pdb_methods(pdb_context->mem_ctx, pdb_method))) {
		return nt_status;
	}

	(*pdb_method)->name = "ldapsam";

	(*pdb_method)->setsampwent = ldapsam_setsampwent;
	(*pdb_method)->endsampwent = ldapsam_endsampwent;
	(*pdb_method)->getsampwent = ldapsam_getsampwent;
	(*pdb_method)->getsampwnam = ldapsam_getsampwnam;
	(*pdb_method)->getsampwsid = ldapsam_getsampwsid;
	(*pdb_method)->add_sam_account = ldapsam_add_sam_account;
	(*pdb_method)->update_sam_account = ldapsam_update_sam_account;
	(*pdb_method)->delete_sam_account = ldapsam_delete_sam_account;
	(*pdb_method)->rename_sam_account = ldapsam_rename_sam_account;

	(*pdb_method)->getgrsid = ldapsam_getgrsid;
	(*pdb_method)->getgrgid = ldapsam_getgrgid;
	(*pdb_method)->getgrnam = ldapsam_getgrnam;
	(*pdb_method)->add_group_mapping_entry = ldapsam_add_group_mapping_entry;
	(*pdb_method)->update_group_mapping_entry = ldapsam_update_group_mapping_entry;
	(*pdb_method)->delete_group_mapping_entry = ldapsam_delete_group_mapping_entry;
	(*pdb_method)->enum_group_mapping = ldapsam_enum_group_mapping;
	(*pdb_method)->enum_group_members = ldapsam_enum_group_members;
	(*pdb_method)->enum_group_memberships = ldapsam_enum_group_memberships;
	(*pdb_method)->lookup_rids = ldapsam_lookup_rids;

	(*pdb_method)->get_account_policy = ldapsam_get_account_policy;
	(*pdb_method)->set_account_policy = ldapsam_set_account_policy;

	(*pdb_method)->get_seq_num = ldapsam_get_seq_num;

	/* TODO: Setup private data and free */

	ldap_state = TALLOC_ZERO_P(pdb_context->mem_ctx, struct ldapsam_privates);
	if (!ldap_state) {
		DEBUG(0, ("pdb_init_ldapsam_common: talloc() failed for ldapsam private_data!\n"));
		return NT_STATUS_NO_MEMORY;
	}

	if (!NT_STATUS_IS_OK(nt_status = 
			     smbldap_init(pdb_context->mem_ctx, location, 
					  &ldap_state->smbldap_state)));

	ldap_state->domain_name = talloc_strdup(pdb_context->mem_ctx, get_global_sam_name());
	if (!ldap_state->domain_name) {
		return NT_STATUS_NO_MEMORY;
	}

	(*pdb_method)->private_data = ldap_state;

	(*pdb_method)->free_private_data = free_private_data;

	return NT_STATUS_OK;
}

/**********************************************************************
 Initialise the 'compat' mode for pdb_ldap
 *********************************************************************/

NTSTATUS pdb_init_ldapsam_compat(PDB_CONTEXT *pdb_context, PDB_METHODS **pdb_method, const char *location)
{
	NTSTATUS nt_status;
	struct ldapsam_privates *ldap_state;

#ifdef WITH_LDAP_SAMCONFIG
	if (!location) {
		int ldap_port = lp_ldap_port();
			
		/* remap default port if not using SSL (ie clear or TLS) */
		if ( (lp_ldap_ssl() != LDAP_SSL_ON) && (ldap_port == 636) ) {
			ldap_port = 389;
		}

		location = talloc_asprintf(pdb_context->mem_ctx, "%s://%s:%d", lp_ldap_ssl() == LDAP_SSL_ON ? "ldaps" : "ldap", lp_ldap_server(), ldap_port);
		if (!location) {
			return NT_STATUS_NO_MEMORY;
		}
	}
#endif

	if (!NT_STATUS_IS_OK(nt_status = pdb_init_ldapsam_common(pdb_context, pdb_method, location))) {
		return nt_status;
	}

	(*pdb_method)->name = "ldapsam_compat";

	ldap_state = (*pdb_method)->private_data;
	ldap_state->schema_ver = SCHEMAVER_SAMBAACCOUNT;

	sid_copy(&ldap_state->domain_sid, get_global_sam_sid());

	return NT_STATUS_OK;
}

/**********************************************************************
 Initialise the normal mode for pdb_ldap
 *********************************************************************/

NTSTATUS pdb_init_ldapsam(PDB_CONTEXT *pdb_context, PDB_METHODS **pdb_method, const char *location)
{
	NTSTATUS nt_status;
	struct ldapsam_privates *ldap_state;
	uint32 alg_rid_base;
	pstring alg_rid_base_string;
	LDAPMessage *result = NULL;
	LDAPMessage *entry = NULL;
	DOM_SID ldap_domain_sid;
	DOM_SID secrets_domain_sid;
	pstring domain_sid_string;
	char *dn;

	if (!NT_STATUS_IS_OK(nt_status = pdb_init_ldapsam_common(pdb_context, pdb_method, location))) {
		return nt_status;
	}

	(*pdb_method)->name = "ldapsam";

	(*pdb_method)->add_aliasmem = ldapsam_add_aliasmem;
	(*pdb_method)->del_aliasmem = ldapsam_del_aliasmem;
	(*pdb_method)->enum_aliasmem = ldapsam_enum_aliasmem;
	(*pdb_method)->enum_alias_memberships = ldapsam_alias_memberships;
	(*pdb_method)->search_users = ldapsam_search_users;
	(*pdb_method)->search_groups = ldapsam_search_groups;
	(*pdb_method)->search_aliases = ldapsam_search_aliases;

	ldap_state = (*pdb_method)->private_data;
	ldap_state->schema_ver = SCHEMAVER_SAMBASAMACCOUNT;

	/* Try to setup the Domain Name, Domain SID, algorithmic rid base */
	
	nt_status = smbldap_search_domain_info(ldap_state->smbldap_state, &result, 
					       ldap_state->domain_name, True);
	
	if ( !NT_STATUS_IS_OK(nt_status) ) {
		DEBUG(2, ("pdb_init_ldapsam: WARNING: Could not get domain info, nor add one to the domain\n"));
		DEBUGADD(2, ("pdb_init_ldapsam: Continuing on regardless, will be unable to allocate new users/groups, \
and will risk BDCs having inconsistant SIDs\n"));
		sid_copy(&ldap_state->domain_sid, get_global_sam_sid());
		return NT_STATUS_OK;
	}

	/* Given that the above might fail, everything below this must be optional */
	
	entry = ldap_first_entry(ldap_state->smbldap_state->ldap_struct, result);
	if (!entry) {
		DEBUG(0, ("pdb_init_ldapsam: Could not get domain info entry\n"));
		ldap_msgfree(result);
		return NT_STATUS_UNSUCCESSFUL;
	}

	dn = smbldap_get_dn(ldap_state->smbldap_state->ldap_struct, entry);
	if (!dn) {
		return NT_STATUS_UNSUCCESSFUL;
	}

	ldap_state->domain_dn = smb_xstrdup(dn);
	ldap_memfree(dn);

	if (smbldap_get_single_pstring(ldap_state->smbldap_state->ldap_struct, entry, 
				 get_userattr_key2string(ldap_state->schema_ver, LDAP_ATTR_USER_SID), 
				 domain_sid_string)) {
		BOOL found_sid;
		if (!string_to_sid(&ldap_domain_sid, domain_sid_string)) {
			DEBUG(1, ("pdb_init_ldapsam: SID [%s] could not be read as a valid SID\n", domain_sid_string));
			return NT_STATUS_INVALID_PARAMETER;
		}
		found_sid = secrets_fetch_domain_sid(ldap_state->domain_name, &secrets_domain_sid);
		if (!found_sid || !sid_equal(&secrets_domain_sid, &ldap_domain_sid)) {
			fstring new_sid_str, old_sid_str;
			DEBUG(1, ("pdb_init_ldapsam: Resetting SID for domain %s based on pdb_ldap results %s -> %s\n",
				  ldap_state->domain_name, 
				  sid_to_string(old_sid_str, &secrets_domain_sid),
				  sid_to_string(new_sid_str, &ldap_domain_sid)));
			
			/* reset secrets.tdb sid */
			secrets_store_domain_sid(ldap_state->domain_name, &ldap_domain_sid);
			DEBUG(1, ("New global sam SID: %s\n", sid_to_string(new_sid_str, get_global_sam_sid())));
		}
		sid_copy(&ldap_state->domain_sid, &ldap_domain_sid);
	}

	if (smbldap_get_single_pstring(ldap_state->smbldap_state->ldap_struct, entry, 
				 get_attr_key2string( dominfo_attr_list, LDAP_ATTR_ALGORITHMIC_RID_BASE ),
				 alg_rid_base_string)) {
		alg_rid_base = (uint32)atol(alg_rid_base_string);
		if (alg_rid_base != algorithmic_rid_base()) {
			DEBUG(0, ("The value of 'algorithmic RID base' has changed since the LDAP\n"
				  "database was initialised.  Aborting. \n"));
			ldap_msgfree(result);
			return NT_STATUS_UNSUCCESSFUL;
		}
	}
	ldap_msgfree(result);

	return NT_STATUS_OK;
}

NTSTATUS pdb_ldap_init(void)
{
	NTSTATUS nt_status;
	if (!NT_STATUS_IS_OK(nt_status = smb_register_passdb(PASSDB_INTERFACE_VERSION, "ldapsam", pdb_init_ldapsam)))
		return nt_status;

	if (!NT_STATUS_IS_OK(nt_status = smb_register_passdb(PASSDB_INTERFACE_VERSION, "ldapsam_compat", pdb_init_ldapsam_compat)))
		return nt_status;

	/* Let pdb_nds register backends */
	pdb_nds_init();

	return NT_STATUS_OK;
}
