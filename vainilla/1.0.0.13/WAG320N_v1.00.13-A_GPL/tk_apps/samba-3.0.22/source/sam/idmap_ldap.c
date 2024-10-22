/*
   Unix SMB/CIFS implementation.

   idmap LDAP backend

   Copyright (C) Tim Potter 		2000
   Copyright (C) Jim McDonough <jmcd@us.ibm.com>	2003
   Copyright (C) Simo Sorce 		2003
   Copyright (C) Gerald Carter 		2003

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

#include "includes.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_IDMAP


#include <lber.h>
#include <ldap.h>

#include "smbldap.h"

struct ldap_idmap_state {
	struct smbldap_state *smbldap_state;
	TALLOC_CTX *mem_ctx;
};

static struct ldap_idmap_state ldap_state;

/* number tries while allocating new id */
#define LDAP_MAX_ALLOC_ID 128


/***********************************************************************
 This function cannot be called to modify a mapping, only set a new one
***********************************************************************/

static NTSTATUS ldap_set_mapping(const DOM_SID *sid, unid_t id, int id_type)
{
	pstring dn;
	pstring id_str;
	fstring type;
	LDAPMod **mods = NULL;
	int rc = -1;
	int ldap_op;
	fstring sid_string;
	LDAPMessage *entry = NULL;

	sid_to_string( sid_string, sid );

	ldap_op = LDAP_MOD_ADD;
	pstr_sprintf(dn, "%s=%s,%s", get_attr_key2string( sidmap_attr_list, LDAP_ATTR_SID),
		 sid_string, lp_ldap_idmap_suffix());

	if ( id_type & ID_USERID )
		fstrcpy( type, get_attr_key2string( sidmap_attr_list, LDAP_ATTR_UIDNUMBER ) );
	else
		fstrcpy( type, get_attr_key2string( sidmap_attr_list, LDAP_ATTR_GIDNUMBER ) );

	pstr_sprintf(id_str, "%lu", ((id_type & ID_USERID) ? (unsigned long)id.uid :
						 (unsigned long)id.gid));	
	
	smbldap_set_mod( &mods, LDAP_MOD_ADD, "objectClass", LDAP_OBJ_IDMAP_ENTRY );

	smbldap_make_mod( ldap_state.smbldap_state->ldap_struct, 
			  entry, &mods, type, id_str );

	smbldap_make_mod( ldap_state.smbldap_state->ldap_struct,
			  entry, &mods,  
			  get_attr_key2string(sidmap_attr_list, LDAP_ATTR_SID), 
			  sid_string );

	/* There may well be nothing at all to do */

	if (mods) {
		smbldap_set_mod( &mods, LDAP_MOD_ADD, "objectClass", LDAP_OBJ_SID_ENTRY );
		rc = smbldap_add(ldap_state.smbldap_state, dn, mods);
		ldap_mods_free( mods, True );	
	} else {
		rc = LDAP_SUCCESS;
	}

	if (rc != LDAP_SUCCESS) {
		char *ld_error = NULL;
		ldap_get_option(ldap_state.smbldap_state->ldap_struct, LDAP_OPT_ERROR_STRING,
				&ld_error);
		DEBUG(0,("ldap_set_mapping_internals: Failed to %s mapping from %s to %lu [%s]\n",
			 (ldap_op == LDAP_MOD_ADD) ? "add" : "replace",
			 sid_string, (unsigned long)((id_type & ID_USERID) ? id.uid : id.gid), type));
		DEBUG(0, ("ldap_set_mapping_internals: Error was: %s (%s)\n", 
			ld_error ? ld_error : "(NULL)", ldap_err2string (rc)));
		return NT_STATUS_UNSUCCESSFUL;
	}
		
	DEBUG(10,("ldap_set_mapping: Successfully created mapping from %s to %lu [%s]\n",
		sid_string, ((id_type & ID_USERID) ? (unsigned long)id.uid : 
			     (unsigned long)id.gid), type));

	return NT_STATUS_OK;
}

/**********************************************************************
 Even if the sambaDomain attribute in LDAP tells us that this RID is 
 safe to use, always check before use.  
*********************************************************************/

static BOOL sid_in_use(struct ldap_idmap_state *state, 
		       const DOM_SID *sid, int *error) 
{
	fstring filter;
	fstring sid_string;
	LDAPMessage *result = NULL;
	int rc;
	const char *sid_attr[] = {LDAP_ATTRIBUTE_SID, NULL};

	slprintf(filter, sizeof(filter)-1, "(%s=%s)", LDAP_ATTRIBUTE_SID, sid_to_string(sid_string, sid));

	rc = smbldap_search_suffix(state->smbldap_state, 
				   filter, sid_attr, &result);

	if (rc != LDAP_SUCCESS)	{
		char *ld_error = NULL;
		ldap_get_option(state->smbldap_state->ldap_struct, LDAP_OPT_ERROR_STRING, &ld_error);
		DEBUG(2, ("Failed to check if sid %s is alredy in use: %s\n",
			  sid_string, ld_error));
		SAFE_FREE(ld_error);

		*error = rc;
		return True;
	}
	
	if ((ldap_count_entries(state->smbldap_state->ldap_struct, result)) > 0) {
		DEBUG(3, ("Sid %s already in use - trying next RID\n",
			  sid_string));
		ldap_msgfree(result);
		return True;
	}

	ldap_msgfree(result);

	/* good, sid is not in use */
	return False;
}

/**********************************************************************
 Set the new nextRid attribute, and return one we can use.

 This also checks that this RID is actually free - in case the admin
 manually stole it :-).
*********************************************************************/

static NTSTATUS ldap_next_rid(struct ldap_idmap_state *state, uint32 *rid, 
                              int rid_type)
{
	NTSTATUS ret = NT_STATUS_UNSUCCESSFUL;
	LDAPMessage *domain_result = NULL;
	LDAPMessage *entry  = NULL;
	char *dn;
	LDAPMod **mods = NULL;
	fstring old_rid_string;
	fstring next_rid_string;
	fstring algorithmic_rid_base_string;
	uint32 next_rid;
	uint32 alg_rid_base;
	int attempts = 0;
	char *ld_error = NULL;

	while (attempts < 10) {
		if (!NT_STATUS_IS_OK(ret = smbldap_search_domain_info(state->smbldap_state, 
				&domain_result, get_global_sam_name(), True))) {
			return ret;
		}
	
		entry = ldap_first_entry(state->smbldap_state->ldap_struct, domain_result);
		if (!entry) {
			DEBUG(0, ("Could not get domain info entry\n"));
			ldap_msgfree(domain_result);
			return ret;
		}

		if ((dn = smbldap_get_dn(state->smbldap_state->ldap_struct, entry)) == NULL) {
			DEBUG(0, ("Could not get domain info DN\n"));
			ldap_msgfree(domain_result);
			return ret;
		}

		/* yes, we keep 3 seperate counters, one for rids between 1000 (BASE_RID) and 
		   algorithmic_rid_base.  The other two are to avoid stomping on the
		   different sets of algorithmic RIDs */
		
		if (smbldap_get_single_pstring(state->smbldap_state->ldap_struct, entry,
					 get_attr_key2string(dominfo_attr_list, LDAP_ATTR_ALGORITHMIC_RID_BASE),
					 algorithmic_rid_base_string)) {
			
			alg_rid_base = (uint32)atol(algorithmic_rid_base_string);
		} else {
			alg_rid_base = algorithmic_rid_base();
			/* Try to make the modification atomically by enforcing the
			   old value in the delete mod. */
			slprintf(algorithmic_rid_base_string, sizeof(algorithmic_rid_base_string)-1, "%d", alg_rid_base);
			smbldap_make_mod(state->smbldap_state->ldap_struct, entry, &mods, 
					 get_attr_key2string(dominfo_attr_list, LDAP_ATTR_ALGORITHMIC_RID_BASE), 
					 algorithmic_rid_base_string);
		}

		next_rid = 0;

		if (alg_rid_base > BASE_RID) {
			/* we have a non-default 'algorithmic rid base', so we have 'low' rids that we 
			   can allocate to new users */
			if (smbldap_get_single_pstring(state->smbldap_state->ldap_struct, entry,
						 get_attr_key2string(dominfo_attr_list, LDAP_ATTR_NEXT_RID),
						 old_rid_string)) {
				*rid = (uint32)atol(old_rid_string);
			} else {
				*rid = BASE_RID;
			}

			next_rid = *rid+1;
			if (next_rid >= alg_rid_base) {
				ldap_msgfree(domain_result);
				return NT_STATUS_UNSUCCESSFUL;
			}
			
			slprintf(next_rid_string, sizeof(next_rid_string)-1, "%d", next_rid);
				
			/* Try to make the modification atomically by enforcing the
			   old value in the delete mod. */
			smbldap_make_mod(state->smbldap_state->ldap_struct, entry, &mods, 
					 get_attr_key2string(dominfo_attr_list, LDAP_ATTR_NEXT_RID), 
					 next_rid_string);
		}

		if (!next_rid) { /* not got one already */
			switch (rid_type) {
			case USER_RID_TYPE:
				if (smbldap_get_single_pstring(state->smbldap_state->ldap_struct, entry,
							 get_attr_key2string(dominfo_attr_list, LDAP_ATTR_NEXT_USERRID),
							 old_rid_string)) {
					*rid = (uint32)atol(old_rid_string);					
				}
				break;
			case GROUP_RID_TYPE:
				if (smbldap_get_single_pstring(state->smbldap_state->ldap_struct, entry, 
							 get_attr_key2string(dominfo_attr_list, LDAP_ATTR_NEXT_GROUPRID),
							 old_rid_string)) {
					*rid = (uint32)atol(old_rid_string);
				}
				break;
			}
			
			/* This is the core of the whole routine. If we had
			   scheme-style closures, there would be a *lot* less code
			   duplication... */

			next_rid = *rid+RID_MULTIPLIER;
			slprintf(next_rid_string, sizeof(next_rid_string)-1, "%d", next_rid);
			
			switch (rid_type) {
			case USER_RID_TYPE:
				/* Try to make the modification atomically by enforcing the
				   old value in the delete mod. */
				smbldap_make_mod(state->smbldap_state->ldap_struct, entry, &mods, 
						 get_attr_key2string(dominfo_attr_list, LDAP_ATTR_NEXT_USERRID), 
						 next_rid_string);
				break;
				
			case GROUP_RID_TYPE:
				/* Try to make the modification atomically by enforcing the
				   old value in the delete mod. */
				smbldap_make_mod(state->smbldap_state->ldap_struct, entry, &mods,
						 get_attr_key2string(dominfo_attr_list, LDAP_ATTR_NEXT_GROUPRID),
						 next_rid_string);
				break;
			}
		}

		if ((smbldap_modify(state->smbldap_state, dn, mods)) == LDAP_SUCCESS) {
			DOM_SID dom_sid;
			DOM_SID sid;
			pstring domain_sid_string;
			int error = 0;

			if (!smbldap_get_single_pstring(state->smbldap_state->ldap_struct, domain_result,
					get_attr_key2string(dominfo_attr_list, LDAP_ATTR_DOM_SID),
					domain_sid_string)) {
				ldap_mods_free(mods, True);
				SAFE_FREE(dn);
				ldap_msgfree(domain_result);
				return ret;
			}

			if (!string_to_sid(&dom_sid, domain_sid_string)) { 
				ldap_mods_free(mods, True);
				SAFE_FREE(dn);
				ldap_msgfree(domain_result);
				return ret;
			}

			ldap_mods_free(mods, True);
			mods = NULL;
			SAFE_FREE(dn);
			ldap_msgfree(domain_result);

			sid_copy(&sid, &dom_sid);
			sid_append_rid(&sid, *rid);

			/* check RID is not in use */
			if (sid_in_use(state, &sid, &error)) {
				if (error) {
					return ret;
				}
				continue;
			}

			return NT_STATUS_OK;
		}

		ld_error = NULL;
		ldap_get_option(state->smbldap_state->ldap_struct, LDAP_OPT_ERROR_STRING, &ld_error);
		DEBUG(2, ("Failed to modify rid: %s\n", ld_error ? ld_error : "(NULL"));
		SAFE_FREE(ld_error);

		ldap_mods_free(mods, True);
		mods = NULL;

		SAFE_FREE(dn);

		ldap_msgfree(domain_result);
		domain_result = NULL;

		{
			/* Sleep for a random timeout */
			unsigned sleeptime = (sys_random()*sys_getpid()*attempts);
			attempts += 1;
			
			sleeptime %= 100;
			smb_msleep(sleeptime);
		}
	}

	DEBUG(0, ("Failed to set new RID\n"));
	return ret;
}


/*****************************************************************************
 Allocate a new RID
*****************************************************************************/

static NTSTATUS ldap_allocate_rid(uint32 *rid, int rid_type)
{
	return ldap_next_rid( &ldap_state, rid, rid_type );
}

/*****************************************************************************
 Allocate a new uid or gid
*****************************************************************************/

static NTSTATUS ldap_allocate_id(unid_t *id, int id_type)
{
	NTSTATUS ret = NT_STATUS_UNSUCCESSFUL;
	int rc = LDAP_SERVER_DOWN;
	int count = 0;
	LDAPMessage *result = NULL;
	LDAPMessage *entry = NULL;
	pstring id_str, new_id_str;
	LDAPMod **mods = NULL;
	const char *type;
	char *dn = NULL;
	const char **attr_list;
	pstring filter;
	uid_t	luid, huid;
	gid_t	lgid, hgid;


	type = (id_type & ID_USERID) ?
		get_attr_key2string( idpool_attr_list, LDAP_ATTR_UIDNUMBER ) : 
		get_attr_key2string( idpool_attr_list, LDAP_ATTR_GIDNUMBER );

	pstr_sprintf(filter, "(objectClass=%s)", LDAP_OBJ_IDPOOL);

	attr_list = get_attr_list( idpool_attr_list );

	rc = smbldap_search(ldap_state.smbldap_state, lp_ldap_idmap_suffix(),
			       LDAP_SCOPE_SUBTREE, filter,
			       attr_list, 0, &result);
	free_attr_list( attr_list );
	 
	if (rc != LDAP_SUCCESS) {
		DEBUG(0,("ldap_allocate_id: %s object not found\n", LDAP_OBJ_IDPOOL));
		goto out;
	}
	
	count = ldap_count_entries(ldap_state.smbldap_state->ldap_struct, result);
	if (count != 1) {
		DEBUG(0,("ldap_allocate_id: single %s object not found\n", LDAP_OBJ_IDPOOL));
		goto out;
	}

	dn = smbldap_get_dn(ldap_state.smbldap_state->ldap_struct, result);
	if (!dn) {
		goto out;
	}
	entry = ldap_first_entry(ldap_state.smbldap_state->ldap_struct, result);

	if (!smbldap_get_single_pstring(ldap_state.smbldap_state->ldap_struct, entry, type, id_str)) {
		DEBUG(0,("ldap_allocate_id: %s attribute not found\n",
			 type));
		goto out;
	}

	/* this must succeed or else we wouldn't have initialized */
		
	lp_idmap_uid( &luid, &huid);
	lp_idmap_gid( &lgid, &hgid);
	
	/* make sure we still have room to grow */
	
	if (id_type & ID_USERID) {
		id->uid = strtoul(id_str, NULL, 10);
		if (id->uid > huid ) {
			DEBUG(0,("ldap_allocate_id: Cannot allocate uid above %lu!\n", 
				 (unsigned long)huid));
			goto out;
		}
	}
	else { 
		id->gid = strtoul(id_str, NULL, 10);
		if (id->gid > hgid ) {
			DEBUG(0,("ldap_allocate_id: Cannot allocate gid above %lu!\n", 
				 (unsigned long)hgid));
			goto out;
		}
	}
	
	pstr_sprintf(new_id_str, "%lu", 
		 ((id_type & ID_USERID) ? (unsigned long)id->uid : 
		  (unsigned long)id->gid) + 1);
		 
	smbldap_set_mod( &mods, LDAP_MOD_DELETE, type, id_str );		 
	smbldap_set_mod( &mods, LDAP_MOD_ADD, type, new_id_str );

	if (mods == NULL) {
		DEBUG(0,("ldap_allocate_id: smbldap_set_mod() failed.\n"));
		goto out;		
	}

	rc = smbldap_modify(ldap_state.smbldap_state, dn, mods);

	ldap_mods_free( mods, True );
	if (rc != LDAP_SUCCESS) {
		DEBUG(1,("ldap_allocate_id: Failed to allocate new %s.  ldap_modify() failed.\n",
			type));
		goto out;
	}
	
	ret = NT_STATUS_OK;
out:
	SAFE_FREE(dn);
	if (result != NULL)
		ldap_msgfree(result);

	return ret;
}

/*****************************************************************************
 get a sid from an id
*****************************************************************************/

static NTSTATUS ldap_get_sid_from_id(DOM_SID *sid, unid_t id, int id_type)
{
	LDAPMessage *result = NULL;
	LDAPMessage *entry = NULL;
	pstring sid_str;
	pstring filter;
	pstring suffix;
	const char *type;
	int rc;
	int count;
	NTSTATUS ret = NT_STATUS_UNSUCCESSFUL;
	const char **attr_list;

	if ( id_type & ID_USERID ) 
		type = get_attr_key2string( idpool_attr_list, LDAP_ATTR_UIDNUMBER );
	else 
		type = get_attr_key2string( idpool_attr_list, LDAP_ATTR_GIDNUMBER );

	pstrcpy( suffix, lp_ldap_idmap_suffix() );
	pstr_sprintf(filter, "(&(objectClass=%s)(%s=%lu))",
		LDAP_OBJ_IDMAP_ENTRY, type,  
		((id_type & ID_USERID) ? (unsigned long)id.uid : (unsigned long)id.gid));
		
	attr_list = get_attr_list( sidmap_attr_list );
	rc = smbldap_search(ldap_state.smbldap_state, suffix, LDAP_SCOPE_SUBTREE, 
		filter, attr_list, 0, &result);

	if (rc != LDAP_SUCCESS) {
		DEBUG(3,("ldap_get_isd_from_id: Failure looking up entry (%s)\n",
			ldap_err2string(rc) ));
		goto out;
	}
			   
	count = ldap_count_entries(ldap_state.smbldap_state->ldap_struct, result);

	if (count != 1) {
		DEBUG(0,("ldap_get_sid_from_id: mapping not found for %s: %lu\n", 
			type, ((id_type & ID_USERID) ? (unsigned long)id.uid : 
			       (unsigned long)id.gid)));
		goto out;
	}
	
	entry = ldap_first_entry(ldap_state.smbldap_state->ldap_struct, result);
	
	if ( !smbldap_get_single_pstring(ldap_state.smbldap_state->ldap_struct, entry, LDAP_ATTRIBUTE_SID, sid_str) )
		goto out;
	   
	if (!string_to_sid(sid, sid_str))
		goto out;

	ret = NT_STATUS_OK;
out:
	free_attr_list( attr_list );	 

	if (result)
		ldap_msgfree(result);

	return ret;
}

/***********************************************************************
 Get an id from a sid 
***********************************************************************/

static NTSTATUS ldap_get_id_from_sid(unid_t *id, int *id_type, const DOM_SID *sid)
{
	LDAPMessage *result = NULL;
	LDAPMessage *entry = NULL;
	pstring sid_str;
	pstring filter;
	pstring id_str;
	const char *suffix;	
	const char *type;
	int rc;
	int count;
	const char **attr_list;
	char *dn = NULL;
	NTSTATUS ret = NT_STATUS_UNSUCCESSFUL;

	sid_to_string(sid_str, sid);

	DEBUG(8,("ldap_get_id_from_sid: %s (%s)\n", sid_str,
		(*id_type & ID_GROUPID ? "group" : "user") ));

	suffix = lp_ldap_idmap_suffix();
	pstr_sprintf(filter, "(&(objectClass=%s)(%s=%s))", 
		LDAP_OBJ_IDMAP_ENTRY, LDAP_ATTRIBUTE_SID, sid_str);
			
	if ( *id_type & ID_GROUPID ) 
		type = get_attr_key2string( sidmap_attr_list, LDAP_ATTR_GIDNUMBER );
	else 
		type = get_attr_key2string( sidmap_attr_list, LDAP_ATTR_UIDNUMBER );

	/* do the search and check for errors */

	attr_list = get_attr_list( sidmap_attr_list );
	rc = smbldap_search(ldap_state.smbldap_state, suffix, LDAP_SCOPE_SUBTREE, 
		filter, attr_list, 0, &result);
			
	if (rc != LDAP_SUCCESS) {
		DEBUG(3,("ldap_get_id_from_sid: Failure looking up idmap entry (%s)\n",
			ldap_err2string(rc) ));
		goto out;
	}
			
	/* check for the number of entries returned */

	count = ldap_count_entries(ldap_state.smbldap_state->ldap_struct, result);
	   
	if ( count > 1 ) {
		DEBUG(0, ("ldap_get_id_from_sid: (2nd) search %s returned [%d] entries!\n",
			filter, count));
		goto out;
	}
	
	/* try to allocate a new id if we still haven't found one */

	if ( !count ) {
		int i;

		if (*id_type & ID_QUERY_ONLY) {
			DEBUG(5,("ldap_get_id_from_sid: No matching entry found and QUERY_ONLY flag set\n"));
			goto out;
		}

		DEBUG(8,("ldap_get_id_from_sid: Allocating new id\n"));
		
		for (i = 0; i < LDAP_MAX_ALLOC_ID; i++) {
			ret = ldap_allocate_id(id, *id_type);
			if ( NT_STATUS_IS_OK(ret) )
				break;
		}
		
		if ( !NT_STATUS_IS_OK(ret) ) {
			DEBUG(0,("ldap_allocate_id: cannot acquire id lock!\n"));
			goto out;
		}

		DEBUG(10,("ldap_get_id_from_sid: Allocated new %cid [%ul]\n",
			(*id_type & ID_GROUPID ? 'g' : 'u'), (uint32)id->uid ));
	
		ret = ldap_set_mapping(sid, *id, *id_type);

		/* all done */

		goto out;
	}

	DEBUG(10,("ldap_get_id_from_sid: success\n"));

	entry = ldap_first_entry(ldap_state.smbldap_state->ldap_struct, result);
	
	dn = smbldap_get_dn(ldap_state.smbldap_state->ldap_struct, result);
	if (!dn)
		goto out;

	DEBUG(10, ("Found mapping entry at dn=%s, looking for %s\n", dn, type));
		
	if ( smbldap_get_single_pstring(ldap_state.smbldap_state->ldap_struct, entry, type, id_str) ) {
		if ( (*id_type & ID_USERID) )
			id->uid = strtoul(id_str, NULL, 10);
		else
			id->gid = strtoul(id_str, NULL, 10);
		
		ret = NT_STATUS_OK;
		goto out;
	}
	
out:
	free_attr_list( attr_list );
	if (result)
		ldap_msgfree(result);
	SAFE_FREE(dn);
	
	return ret;
}

/**********************************************************************
 Verify the sambaUnixIdPool entry in the directiry.  
**********************************************************************/

static NTSTATUS verify_idpool( void )
{
	fstring filter;
	int rc;
	const char **attr_list;
	LDAPMessage *result = NULL;
	LDAPMod **mods = NULL;
	int count;
	
	fstr_sprintf( filter, "(objectclass=%s)", LDAP_OBJ_IDPOOL );
	
	attr_list = get_attr_list( idpool_attr_list );
	rc = smbldap_search(ldap_state.smbldap_state, lp_ldap_idmap_suffix(), 
		LDAP_SCOPE_SUBTREE, filter, attr_list, 0, &result);
	free_attr_list ( attr_list );

	if (rc != LDAP_SUCCESS)
		return NT_STATUS_UNSUCCESSFUL;

	count = ldap_count_entries(ldap_state.smbldap_state->ldap_struct, result);

	ldap_msgfree(result);

	if ( count > 1 ) {
		DEBUG(0,("ldap_idmap_init: multiple entries returned from %s (base == %s)\n",
			filter, lp_ldap_idmap_suffix() ));
		return NT_STATUS_UNSUCCESSFUL;
	}
	else if (count == 0) {
		uid_t	luid, huid;
		gid_t	lgid, hgid;
		fstring uid_str, gid_str;
		
		if ( !lp_idmap_uid(&luid, &huid) || !lp_idmap_gid( &lgid, &hgid ) ) {
			DEBUG(0,("ldap_idmap_init: idmap uid/gid parameters not specified\n"));
			return NT_STATUS_UNSUCCESSFUL;
		}
		
		fstr_sprintf( uid_str, "%lu", (unsigned long)luid );
		fstr_sprintf( gid_str, "%lu", (unsigned long)lgid );

		smbldap_set_mod( &mods, LDAP_MOD_ADD, "objectClass", LDAP_OBJ_IDPOOL );
		smbldap_set_mod( &mods, LDAP_MOD_ADD, 
			get_attr_key2string(idpool_attr_list, LDAP_ATTR_UIDNUMBER), uid_str );
		smbldap_set_mod( &mods, LDAP_MOD_ADD,
			get_attr_key2string(idpool_attr_list, LDAP_ATTR_GIDNUMBER), gid_str );
		if (mods) {
			rc = smbldap_modify(ldap_state.smbldap_state, lp_ldap_idmap_suffix(), mods);
			ldap_mods_free( mods, True );
		} else {
			return NT_STATUS_UNSUCCESSFUL;
		}
	}

	return ( rc==LDAP_SUCCESS ? NT_STATUS_OK : NT_STATUS_UNSUCCESSFUL );
}

/*****************************************************************************
 Initialise idmap database. 
*****************************************************************************/

static NTSTATUS ldap_idmap_init( char *params )
{
	NTSTATUS nt_status;

	ldap_state.mem_ctx = talloc_init("idmap_ldap");
	if (!ldap_state.mem_ctx) {
		return NT_STATUS_NO_MEMORY;
	}

	/* assume location is the only parameter */
	if (!NT_STATUS_IS_OK(nt_status = 
			     smbldap_init(ldap_state.mem_ctx, params, 
					  &ldap_state.smbldap_state))) {
		talloc_destroy(ldap_state.mem_ctx);
		return nt_status;
	}

	/* see if the idmap suffix and sub entries exists */
	
	nt_status = verify_idpool();	
	if ( !NT_STATUS_IS_OK(nt_status) )
		return nt_status;
		
	return NT_STATUS_OK;
}

/*****************************************************************************
 End the LDAP session
*****************************************************************************/

static NTSTATUS ldap_idmap_close(void)
{

	smbldap_free_struct(&(ldap_state).smbldap_state);
	talloc_destroy(ldap_state.mem_ctx);
	
	DEBUG(5,("The connection to the LDAP server was closed\n"));
	/* maybe free the results here --metze */
	
	return NT_STATUS_OK;
}


/* This function doesn't make as much sense in an LDAP world since the calling
   node doesn't really control the ID ranges */
static void ldap_idmap_status(void)
{
	DEBUG(0, ("LDAP IDMAP Status not available\n"));
}

static struct idmap_methods ldap_methods = {
	ldap_idmap_init,
	ldap_allocate_rid,
	ldap_allocate_id,
	ldap_get_sid_from_id,
	ldap_get_id_from_sid,
	ldap_set_mapping,
	ldap_idmap_close,
	ldap_idmap_status

};

NTSTATUS idmap_ldap_init(void)
{
	return smb_register_idmap(SMB_IDMAP_INTERFACE_VERSION, "ldap", &ldap_methods);
}
