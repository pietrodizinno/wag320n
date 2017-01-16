/* 
   Unix SMB/CIFS implementation.
   Locking functions
   Copyright (C) Andrew Tridgell 1992-2000
   Copyright (C) Jeremy Allison 1992-2000
   Copyright (C) Volker Lendecke 2005
   
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

   Revision History:

   12 aug 96: Erik.Devriendt@te6.siemens.be
   added support for shared memory implementation of share mode locking

   May 1997. Jeremy Allison (jallison@whistle.com). Modified share mode
   locking to deal with multiple share modes per open file.

   September 1997. Jeremy Allison (jallison@whistle.com). Added oplock
   support.

   rewrtten completely to use new tdb code. Tridge, Dec '99

   Added POSIX locking support. Jeremy Allison (jeremy@valinux.com), Apr. 2000.
*/

#include "includes.h"
uint16 global_smbpid;

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_LOCKING

/* the locking database handle */
static TDB_CONTEXT *tdb;

struct locking_data {
        union {
		struct {
			int num_share_mode_entries;
			BOOL delete_on_close;
		} s;
                struct share_mode_entry dummy; /* Needed for alignment. */
        } u;
        /* the following two entries are implicit
           struct share_mode_entry modes[num_share_mode_entries];
           char file_name[];
        */
};

/****************************************************************************
 Debugging aid :-).
****************************************************************************/

static const char *lock_type_name(enum brl_type lock_type)
{
	return (lock_type == READ_LOCK) ? "READ" : "WRITE";
}

/****************************************************************************
 Utility function called to see if a file region is locked.
****************************************************************************/

BOOL is_locked(files_struct *fsp,connection_struct *conn,
	       SMB_BIG_UINT count,SMB_BIG_UINT offset, 
	       enum brl_type lock_type)
{
	int snum = SNUM(conn);
	int strict_locking = lp_strict_locking(snum);
	BOOL ret;
	
	if (count == 0)
		return(False);

	if (!lp_locking(snum) || !strict_locking)
		return(False);

	if (strict_locking == Auto) {
		if  (EXCLUSIVE_OPLOCK_TYPE(fsp->oplock_type) && (lock_type == READ_LOCK || lock_type == WRITE_LOCK)) {
			DEBUG(10,("is_locked: optimisation - exclusive oplock on file %s\n", fsp->fsp_name ));
			ret = 0;
		} else if ((fsp->oplock_type == LEVEL_II_OPLOCK) &&
			   (lock_type == READ_LOCK)) {
			DEBUG(10,("is_locked: optimisation - level II oplock on file %s\n", fsp->fsp_name ));
			ret = 0;
		} else {
			ret = !brl_locktest(fsp->dev, fsp->inode, fsp->fnum,
				     global_smbpid, procid_self(), conn->cnum, 
				     offset, count, lock_type);
		}
	} else {
		ret = !brl_locktest(fsp->dev, fsp->inode, fsp->fnum,
				global_smbpid, procid_self(), conn->cnum,
				offset, count, lock_type);
	}

	DEBUG(10,("is_locked: brl start=%.0f len=%.0f %s for file %s\n",
			(double)offset, (double)count, ret ? "locked" : "unlocked",
			fsp->fsp_name ));

	/*
	 * There is no lock held by an SMB daemon, check to
	 * see if there is a POSIX lock from a UNIX or NFS process.
	 */

	if(!ret && lp_posix_locking(snum)) {
		ret = is_posix_locked(fsp, offset, count, lock_type);

		DEBUG(10,("is_locked: posix start=%.0f len=%.0f %s for file %s\n",
				(double)offset, (double)count, ret ? "locked" : "unlocked",
				fsp->fsp_name ));
	}

	return ret;
}

/****************************************************************************
 Utility function called by locking requests.
****************************************************************************/

static NTSTATUS do_lock(files_struct *fsp,connection_struct *conn, uint16 lock_pid,
		 SMB_BIG_UINT count,SMB_BIG_UINT offset,enum brl_type lock_type, BOOL *my_lock_ctx)
{
	NTSTATUS status = NT_STATUS_LOCK_NOT_GRANTED;

	if (!lp_locking(SNUM(conn)))
		return NT_STATUS_OK;

	/* NOTE! 0 byte long ranges ARE allowed and should be stored  */

	DEBUG(10,("do_lock: lock type %s start=%.0f len=%.0f requested for file %s\n",
		  lock_type_name(lock_type), (double)offset, (double)count, fsp->fsp_name ));

	if (OPEN_FSP(fsp) && fsp->can_lock && (fsp->conn == conn)) {
		status = brl_lock(fsp->dev, fsp->inode, fsp->fnum,
				  lock_pid, procid_self(), conn->cnum, 
				  offset, count, 
				  lock_type, my_lock_ctx);

		if (NT_STATUS_IS_OK(status) && lp_posix_locking(SNUM(conn))) {

			/*
			 * Try and get a POSIX lock on this range.
			 * Note that this is ok if it is a read lock
			 * overlapping on a different fd. JRA.
			 */

			if (!set_posix_lock(fsp, offset, count, lock_type)) {
				if (errno == EACCES || errno == EAGAIN)
					status = NT_STATUS_FILE_LOCK_CONFLICT;
				else
					status = map_nt_error_from_unix(errno);

				/*
				 * We failed to map - we must now remove the brl
				 * lock entry.
				 */
				(void)brl_unlock(fsp->dev, fsp->inode, fsp->fnum,
								lock_pid, procid_self(), conn->cnum, 
								offset, count, False,
								NULL, NULL);
			}
		}
	}

	return status;
}

/****************************************************************************
 Utility function called by locking requests. This is *DISGUSTING*. It also
 appears to be "What Windows Does" (tm). Andrew, ever wonder why Windows 2000
 is so slow on the locking tests...... ? This is the reason. Much though I hate
 it, we need this. JRA.
****************************************************************************/

NTSTATUS do_lock_spin(files_struct *fsp,connection_struct *conn, uint16 lock_pid,
		 SMB_BIG_UINT count,SMB_BIG_UINT offset,enum brl_type lock_type, BOOL *my_lock_ctx)
{
	int j, maxj = lp_lock_spin_count();
	int sleeptime = lp_lock_sleep_time();
	NTSTATUS status, ret;

	if (maxj <= 0)
		maxj = 1;

	ret = NT_STATUS_OK; /* to keep dumb compilers happy */

	for (j = 0; j < maxj; j++) {
		status = do_lock(fsp, conn, lock_pid, count, offset, lock_type, my_lock_ctx);
		if (!NT_STATUS_EQUAL(status, NT_STATUS_LOCK_NOT_GRANTED) &&
		    !NT_STATUS_EQUAL(status, NT_STATUS_FILE_LOCK_CONFLICT)) {
			return status;
		}
		/* if we do fail then return the first error code we got */
		if (j == 0) {
			ret = status;
			/* Don't spin if we blocked ourselves. */
			if (*my_lock_ctx)
				return ret;
		}
		if (sleeptime)
			sys_usleep(sleeptime);
	}
	return ret;
}

/* Struct passed to brl_unlock. */
struct posix_unlock_data_struct {
	files_struct *fsp;
	SMB_BIG_UINT offset;
	SMB_BIG_UINT count;
};

/****************************************************************************
 Function passed to brl_unlock to allow POSIX unlock to be done first.
****************************************************************************/

static void posix_unlock(void *pre_data)
{
	struct posix_unlock_data_struct *pdata = (struct posix_unlock_data_struct *)pre_data;

	if (lp_posix_locking(SNUM(pdata->fsp->conn)))
		release_posix_lock(pdata->fsp, pdata->offset, pdata->count);
}

/****************************************************************************
 Utility function called by unlocking requests.
****************************************************************************/

NTSTATUS do_unlock(files_struct *fsp,connection_struct *conn, uint16 lock_pid,
		   SMB_BIG_UINT count,SMB_BIG_UINT offset)
{
	BOOL ok = False;
	struct posix_unlock_data_struct posix_data;
	
	if (!lp_locking(SNUM(conn)))
		return NT_STATUS_OK;
	
	if (!OPEN_FSP(fsp) || !fsp->can_lock || (fsp->conn != conn)) {
		return NT_STATUS_INVALID_HANDLE;
	}
	
	DEBUG(10,("do_unlock: unlock start=%.0f len=%.0f requested for file %s\n",
		  (double)offset, (double)count, fsp->fsp_name ));

	/*
	 * Remove the existing lock record from the tdb lockdb
	 * before looking at POSIX locks. If this record doesn't
	 * match then don't bother looking to remove POSIX locks.
	 */

	posix_data.fsp = fsp;
	posix_data.offset = offset;
	posix_data.count = count;

	ok = brl_unlock(fsp->dev, fsp->inode, fsp->fnum,
			lock_pid, procid_self(), conn->cnum, offset, count,
			False, posix_unlock, (void *)&posix_data);
   
	if (!ok) {
		DEBUG(10,("do_unlock: returning ERRlock.\n" ));
		return NT_STATUS_RANGE_NOT_LOCKED;
	}
	return NT_STATUS_OK;
}

/****************************************************************************
 Remove any locks on this fd. Called from file_close().
****************************************************************************/

void locking_close_file(files_struct *fsp)
{
	struct process_id pid = procid_self();

	if (!lp_locking(SNUM(fsp->conn)))
		return;

	/*
	 * Just release all the brl locks, no need to release individually.
	 */

	brl_close(fsp->dev, fsp->inode, pid, fsp->conn->cnum, fsp->fnum);

	if(lp_posix_locking(SNUM(fsp->conn))) {

	 	/* 
		 * Release all the POSIX locks.
		 */
		posix_locking_close_file(fsp);

	}
}

/****************************************************************************
 Initialise the locking functions.
****************************************************************************/

static int open_read_only;

BOOL locking_init(int read_only)
{
	brl_init(read_only);

	if (tdb)
		return True;

	tdb = tdb_open_log(lock_path("locking.tdb"), 
			SMB_OPEN_DATABASE_TDB_HASH_SIZE, TDB_DEFAULT|(read_only?0x0:TDB_CLEAR_IF_FIRST), 
			read_only?O_RDONLY:O_RDWR|O_CREAT,
			0644);

	if (!tdb) {
		DEBUG(0,("ERROR: Failed to initialise locking database\n"));
		return False;
	}

	if (!posix_locking_init(read_only))
		return False;

	open_read_only = read_only;

	return True;
}

/*******************************************************************
 Deinitialize the share_mode management.
******************************************************************/

BOOL locking_end(void)
{
	BOOL ret = True;

	brl_shutdown(open_read_only);
	if (tdb) {
		if (tdb_close(tdb) != 0)
			ret = False;
	}

	return ret;
}

/*******************************************************************
 Form a static locking key for a dev/inode pair.
******************************************************************/

/* key and data records in the tdb locking database */
struct locking_key {
	SMB_DEV_T dev;
	SMB_INO_T ino;
};

/*******************************************************************
 Form a static locking key for a dev/inode pair.
******************************************************************/

static TDB_DATA locking_key(SMB_DEV_T dev, SMB_INO_T inode)
{
	static struct locking_key key;
	TDB_DATA kbuf;

	memset(&key, '\0', sizeof(key));
	key.dev = dev;
	key.ino = inode;
	kbuf.dptr = (char *)&key;
	kbuf.dsize = sizeof(key);
	return kbuf;
}

/*******************************************************************
 Print out a share mode.
********************************************************************/

char *share_mode_str(int num, struct share_mode_entry *e)
{
	static pstring share_str;

	slprintf(share_str, sizeof(share_str)-1, "share_mode_entry[%d]: %s "
		 "pid = %s, share_access = 0x%x, private_options = 0x%x, "
		 "access_mask = 0x%x, mid = 0x%x, type= 0x%x, file_id = %lu, "
		 "dev = 0x%x, inode = %.0f",
		 num,
		 e->op_type == UNUSED_SHARE_MODE_ENTRY ? "UNUSED" : "",
		 procid_str_static(&e->pid),
		 e->share_access, e->private_options,
		 e->access_mask, e->op_mid, e->op_type, e->share_file_id,
		 (unsigned int)e->dev, (double)e->inode );

	return share_str;
}

/*******************************************************************
 Print out a share mode table.
********************************************************************/

static void print_share_mode_table(struct locking_data *data)
{
	int num_share_modes = data->u.s.num_share_mode_entries;
	struct share_mode_entry *shares =
		(struct share_mode_entry *)(data + 1);
	int i;

	for (i = 0; i < num_share_modes; i++) {
		struct share_mode_entry entry;

		memcpy(&entry, &shares[i], sizeof(struct share_mode_entry));
		DEBUG(10,("print_share_mode_table: %s\n",
			  share_mode_str(i, &entry)));
	}
}

/*******************************************************************
 Get all share mode entries for a dev/inode pair.
********************************************************************/

static BOOL parse_share_modes(TDB_DATA dbuf, struct share_mode_lock *lck)
{
	struct locking_data *data;
	int i;

	if (dbuf.dsize < sizeof(struct locking_data)) {
		DEBUG(0, ("parse_share_modes: buffer too short\n"));
		return False;
	}

	data = (struct locking_data *)dbuf.dptr;

	lck->delete_on_close = data->u.s.delete_on_close;
	lck->num_share_modes = data->u.s.num_share_mode_entries;

	DEBUG(10, ("parse_share_modes: delete_on_close: %d, "
		   "num_share_modes: %d\n", lck->delete_on_close,
		   lck->num_share_modes));

	if ((lck->num_share_modes < 0) || (lck->num_share_modes > 1000000)) {
		DEBUG(0, ("invalid number of share modes: %d\n",
			  lck->num_share_modes));
		return False;
	}

	lck->share_modes = NULL;

	if (lck->num_share_modes != 0) {

		if (dbuf.dsize < (sizeof(struct locking_data) +
				  (lck->num_share_modes *
				   sizeof(struct share_mode_entry)))) {
			DEBUG(0, ("parse_share_modes: buffer too short\n"));
			return False;
		}
				  
		lck->share_modes = talloc_memdup(lck, dbuf.dptr+sizeof(*data),
						 lck->num_share_modes *
						 sizeof(struct share_mode_entry));

		if (lck->share_modes == NULL) {
			DEBUG(0, ("talloc failed\n"));
			return False;
		}
	}

	/* Save off the associated service path and filename. */
	lck->servicepath = talloc_strdup(lck, dbuf.dptr + sizeof(*data) +
				      (lck->num_share_modes *
				      sizeof(struct share_mode_entry)));

	lck->filename = talloc_strdup(lck, dbuf.dptr + sizeof(*data) +
				      (lck->num_share_modes *
				      sizeof(struct share_mode_entry)) +
				      strlen(lck->servicepath) + 1 );

	/*
	 * Ensure that each entry has a real process attached.
	 */

	for (i = 0; i < lck->num_share_modes; i++) {
		struct share_mode_entry *entry_p = &lck->share_modes[i];
		DEBUG(10,("parse_share_modes: %s\n",
			  share_mode_str(i, entry_p) ));
		if (!process_exists(entry_p->pid)) {
			DEBUG(10,("parse_share_modes: deleted %s\n",
				  share_mode_str(i, entry_p) ));
			entry_p->op_type = UNUSED_SHARE_MODE_ENTRY;
			lck->modified = True;
		}
	}

	return True;
}

static TDB_DATA unparse_share_modes(struct share_mode_lock *lck)
{
	TDB_DATA result;
	int num_valid = 0;
	int i;
	struct locking_data *data;
	ssize_t offset;
	ssize_t sp_len;

	result.dptr = NULL;
	result.dsize = 0;

	for (i=0; i<lck->num_share_modes; i++) {
		if (!is_unused_share_mode_entry(&lck->share_modes[i])) {
			num_valid += 1;
		}
	}

	if (num_valid == 0) {
		return result;
	}

	sp_len = strlen(lck->servicepath);

	result.dsize = sizeof(*data) +
		lck->num_share_modes * sizeof(struct share_mode_entry) +
		sp_len + 1 +
		strlen(lck->filename) + 1;
	result.dptr = talloc_size(lck, result.dsize);

	if (result.dptr == NULL) {
		smb_panic("talloc failed\n");
	}

	data = (struct locking_data *)result.dptr;
	ZERO_STRUCTP(data);
	data->u.s.num_share_mode_entries = lck->num_share_modes;
	data->u.s.delete_on_close = lck->delete_on_close;
	DEBUG(10, ("unparse_share_modes: del: %d, num: %d\n",
		   data->u.s.delete_on_close,
		   data->u.s.num_share_mode_entries));
	memcpy(result.dptr + sizeof(*data), lck->share_modes,
	       sizeof(struct share_mode_entry)*lck->num_share_modes);
	offset = sizeof(*data) +
		sizeof(struct share_mode_entry)*lck->num_share_modes;
	safe_strcpy(result.dptr + offset, lck->servicepath,
		    result.dsize - offset - 1);
	offset += sp_len + 1;
	safe_strcpy(result.dptr + offset, lck->filename,
		    result.dsize - offset - 1);

	if (DEBUGLEVEL >= 10) {
		print_share_mode_table(data);
	}

	return result;
}

static int share_mode_lock_destructor(void *p)
{
	struct share_mode_lock *lck =
		talloc_get_type_abort(p, struct share_mode_lock);
	TDB_DATA key = locking_key(lck->dev, lck->ino);
	TDB_DATA data;

	if (!lck->modified) {
		goto done;
	}

	data = unparse_share_modes(lck);

	if (data.dptr == NULL) {
		if (!lck->fresh) {
			/* There has been an entry before, delete it */
			if (tdb_delete(tdb, key) == -1) {
				smb_panic("Could not delete share entry\n");
			}
		}
		goto done;
	}

	if (tdb_store(tdb, key, data, TDB_REPLACE) == -1) {
		smb_panic("Could not store share mode entry\n");
	}

 done:
	tdb_chainunlock(tdb, key);

	return 0;
}

struct share_mode_lock *get_share_mode_lock(TALLOC_CTX *mem_ctx,
					    SMB_DEV_T dev, SMB_INO_T ino,
						const char *servicepath,
					    const char *fname)
{
	struct share_mode_lock *lck;
	TDB_DATA key = locking_key(dev, ino);
	TDB_DATA data;

	lck = TALLOC_P(mem_ctx, struct share_mode_lock);
	if (lck == NULL) {
		DEBUG(0, ("talloc failed\n"));
		return NULL;
	}

	/* Ensure we set every field here as the destructor must be
	   valid even if parse_share_modes fails. */

	lck->servicepath = NULL;
	lck->filename = NULL;
	lck->dev = dev;
	lck->ino = ino;
	lck->num_share_modes = 0;
	lck->share_modes = NULL;
	lck->delete_on_close = False;
	lck->fresh = False;
	lck->modified = False;

	if (tdb_chainlock(tdb, key) != 0) {
		DEBUG(3, ("Could not lock share entry\n"));
		talloc_free(lck);
		return NULL;
	}

	/* We must set the destructor immediately after the chainlock
	   ensure the lock is cleaned up on any of the error return
	   paths below. */

	talloc_set_destructor(lck, share_mode_lock_destructor);

	data = tdb_fetch(tdb, key);
	lck->fresh = (data.dptr == NULL);

	if (lck->fresh) {

		if (fname == NULL || servicepath == NULL) {
			talloc_free(lck);
			return NULL;
		}
		lck->filename = talloc_strdup(lck, fname);
		lck->servicepath = talloc_strdup(lck, servicepath);
		if (lck->filename == NULL || lck->servicepath == NULL) {
			DEBUG(0, ("talloc failed\n"));
			talloc_free(lck);
			return NULL;
		}
	} else {
		if (!parse_share_modes(data, lck)) {
			DEBUG(0, ("Could not parse share modes\n"));
			talloc_free(lck);
			SAFE_FREE(data.dptr);
			return NULL;
		}
	}

	SAFE_FREE(data.dptr);

	return lck;
}

BOOL get_delete_on_close_flag(SMB_DEV_T dev, SMB_INO_T inode,
			      const char *fname)
{
	BOOL result;
	struct share_mode_lock *lck = get_share_mode_lock(NULL, dev, inode, NULL, NULL);
	if (!lck) {
		return False;
	}
	result = lck->delete_on_close;
	talloc_free(lck);
	return result;
}

BOOL is_valid_share_mode_entry(const struct share_mode_entry *e)
{
	int num_props = 0;

	num_props += ((e->op_type == NO_OPLOCK) ? 1 : 0);
	num_props += (EXCLUSIVE_OPLOCK_TYPE(e->op_type) ? 1 : 0);
	num_props += (LEVEL_II_OPLOCK_TYPE(e->op_type) ? 1 : 0);

	SMB_ASSERT(num_props <= 1);
	return (num_props != 0);
}

BOOL is_deferred_open_entry(const struct share_mode_entry *e)
{
	return (e->op_type == DEFERRED_OPEN_ENTRY);
}

BOOL is_unused_share_mode_entry(const struct share_mode_entry *e)
{
	return (e->op_type == UNUSED_SHARE_MODE_ENTRY);
}

/*******************************************************************
 Fill a share mode entry.
********************************************************************/

static void fill_share_mode_entry(struct share_mode_entry *e,
				  files_struct *fsp,
				  uint16 mid, uint16 op_type)
{
	ZERO_STRUCTP(e);
	e->pid = procid_self();
	e->share_access = fsp->share_access;
	e->private_options = fsp->fh->private_options;
	e->access_mask = fsp->access_mask;
	e->op_mid = mid;
	e->op_type = op_type;
	e->time.tv_sec = fsp->open_time.tv_sec;
	e->time.tv_usec = fsp->open_time.tv_usec;
	e->share_file_id = fsp->file_id;
	e->dev = fsp->dev;
	e->inode = fsp->inode;
}

static void fill_deferred_open_entry(struct share_mode_entry *e,
				     const struct timeval request_time,
				     SMB_DEV_T dev, SMB_INO_T ino, uint16 mid)
{
	ZERO_STRUCTP(e);
	e->pid = procid_self();
	e->op_mid = mid;
	e->op_type = DEFERRED_OPEN_ENTRY;
	e->time.tv_sec = request_time.tv_sec;
	e->time.tv_usec = request_time.tv_usec;
	e->dev = dev;
	e->inode = ino;
}

static void add_share_mode_entry(struct share_mode_lock *lck,
				 const struct share_mode_entry *entry)
{
	int i;

	for (i=0; i<lck->num_share_modes; i++) {
		struct share_mode_entry *e = &lck->share_modes[i];
		if (is_unused_share_mode_entry(e)) {
			*e = *entry;
			break;
		}
	}

	if (i == lck->num_share_modes) {
		/* No unused entry found */
		ADD_TO_ARRAY(lck, struct share_mode_entry, *entry,
			     &lck->share_modes, &lck->num_share_modes);
	}
	lck->modified = True;
}

void set_share_mode(struct share_mode_lock *lck, files_struct *fsp,
		    uint16 mid, uint16 op_type)
{
	struct share_mode_entry entry;
	fill_share_mode_entry(&entry, fsp, mid, op_type);
	add_share_mode_entry(lck, &entry);
}

void add_deferred_open(struct share_mode_lock *lck, uint16 mid,
		       struct timeval request_time,
		       SMB_DEV_T dev, SMB_INO_T ino)
{
	struct share_mode_entry entry;
	fill_deferred_open_entry(&entry, request_time, dev, ino, mid);
	add_share_mode_entry(lck, &entry);
}

/*******************************************************************
 Check if two share mode entries are identical, ignoring oplock 
 and mid info and desired_access.
********************************************************************/

static BOOL share_modes_identical(struct share_mode_entry *e1,
				  struct share_mode_entry *e2)
{
#if 1 /* JRA PARANOIA TEST - REMOVE LATER */
	if (procid_equal(&e1->pid, &e2->pid) &&
	    e1->share_file_id == e2->share_file_id &&
	    e1->dev == e2->dev &&
	    e1->inode == e2->inode &&
	    (e1->share_access) != (e2->share_access)) {
		DEBUG(0,("PANIC: share_modes_identical: share_mode "
			 "mismatch (e1 = 0x%x, e2 = 0x%x). Logic error.\n",
			 (unsigned int)e1->share_access,
			 (unsigned int)e2->share_access ));
		smb_panic("PANIC: share_modes_identical logic error.\n");
	}
#endif

	return (procid_equal(&e1->pid, &e2->pid) &&
		(e1->share_access) == (e2->share_access) &&
		e1->dev == e2->dev &&
		e1->inode == e2->inode &&
		e1->share_file_id == e2->share_file_id );
}

static BOOL deferred_open_identical(struct share_mode_entry *e1,
				    struct share_mode_entry *e2)
{
	return (procid_equal(&e1->pid, &e2->pid) &&
		(e1->op_mid == e2->op_mid) &&
		(e1->dev == e2->dev) &&
		(e1->inode == e2->inode));
}

static struct share_mode_entry *find_share_mode_entry(struct share_mode_lock *lck,
						      struct share_mode_entry *entry)
{
	int i;

	for (i=0; i<lck->num_share_modes; i++) {
		struct share_mode_entry *e = &lck->share_modes[i];
		if (is_valid_share_mode_entry(entry) &&
		    is_valid_share_mode_entry(e) &&
		    share_modes_identical(e, entry)) {
			return e;
		}
		if (is_deferred_open_entry(entry) &&
		    is_deferred_open_entry(e) &&
		    deferred_open_identical(e, entry)) {
			return e;
		}
	}
	return NULL;
}

/*******************************************************************
 Del the share mode of a file for this process. Return the number of
 entries left.
********************************************************************/

BOOL del_share_mode(struct share_mode_lock *lck, files_struct *fsp)
{
	struct share_mode_entry entry, *e;

	fill_share_mode_entry(&entry, fsp, 0, 0);

	e = find_share_mode_entry(lck, &entry);
	if (e == NULL) {
		return False;
	}

	e->op_type = UNUSED_SHARE_MODE_ENTRY;
	lck->modified = True;
	return True;
}

void del_deferred_open_entry(struct share_mode_lock *lck, uint16 mid)
{
	struct share_mode_entry entry, *e;

	fill_deferred_open_entry(&entry, timeval_zero(),
				 lck->dev, lck->ino, mid);

	e = find_share_mode_entry(lck, &entry);
	if (e == NULL) {
		return;
	}

	e->op_type = UNUSED_SHARE_MODE_ENTRY;
	lck->modified = True;
}

/*******************************************************************
 Remove an oplock mid and mode entry from a share mode.
********************************************************************/

BOOL remove_share_oplock(struct share_mode_lock *lck, files_struct *fsp)
{
	struct share_mode_entry entry, *e;

	fill_share_mode_entry(&entry, fsp, 0, 0);

	e = find_share_mode_entry(lck, &entry);
	if (e == NULL) {
		return False;
	}

	e->op_mid = 0;
	e->op_type = NO_OPLOCK;
	lck->modified = True;
	return True;
}

/*******************************************************************
 Downgrade a oplock type from exclusive to level II.
********************************************************************/

BOOL downgrade_share_oplock(struct share_mode_lock *lck, files_struct *fsp)
{
	struct share_mode_entry entry, *e;

	fill_share_mode_entry(&entry, fsp, 0, 0);

	e = find_share_mode_entry(lck, &entry);
	if (e == NULL) {
		return False;
	}

	e->op_type = LEVEL_II_OPLOCK;
	lck->modified = True;
	return True;
}

/****************************************************************************
 Deal with the internal needs of setting the delete on close flag. Note that
 as the tdb locking is recursive, it is safe to call this from within 
 open_file_shared. JRA.
****************************************************************************/

NTSTATUS can_set_delete_on_close(files_struct *fsp, BOOL delete_on_close,
				 uint32 dosmode)
{
	if (!delete_on_close) {
		return NT_STATUS_OK;
	}

	/*
	 * Only allow delete on close for writable files.
	 */

	if ((dosmode & aRONLY) &&
	    !lp_delete_readonly(SNUM(fsp->conn))) {
		DEBUG(10,("can_set_delete_on_close: file %s delete on close "
			  "flag set but file attribute is readonly.\n",
			  fsp->fsp_name ));
		return NT_STATUS_CANNOT_DELETE;
	}

	/*
	 * Only allow delete on close for writable shares.
	 */

	if (!CAN_WRITE(fsp->conn)) {
		DEBUG(10,("can_set_delete_on_close: file %s delete on "
			  "close flag set but write access denied on share.\n",
			  fsp->fsp_name ));
		return NT_STATUS_ACCESS_DENIED;
	}

	/*
	 * Only allow delete on close for files/directories opened with delete
	 * intent.
	 */

	if (!(fsp->access_mask & DELETE_ACCESS)) {
		DEBUG(10,("can_set_delete_on_close: file %s delete on "
			  "close flag set but delete access denied.\n",
			  fsp->fsp_name ));
		return NT_STATUS_ACCESS_DENIED;
	}

	return NT_STATUS_OK;
}

/****************************************************************************
 Sets the delete on close flag over all share modes on this file.
 Modify the share mode entry for all files open
 on this device and inode to tell other smbds we have
 changed the delete on close flag. This will be noticed
 in the close code, the last closer will delete the file
 if flag is set.
****************************************************************************/

BOOL set_delete_on_close(files_struct *fsp, BOOL delete_on_close)
{
	struct share_mode_lock *lck;
	
	DEBUG(10,("set_delete_on_close: %s delete on close flag for "
		  "fnum = %d, file %s\n",
		  delete_on_close ? "Adding" : "Removing", fsp->fnum,
		  fsp->fsp_name ));

	if (fsp->is_stat) {
		return True;
	}

	lck = get_share_mode_lock(NULL, fsp->dev, fsp->inode, NULL, NULL);
	if (lck == NULL) {
		return False;
	}
	if (lck->delete_on_close != delete_on_close) {
		lck->delete_on_close = delete_on_close;
		lck->modified = True;
	}

	talloc_free(lck);
	return True;
}

static int traverse_fn(TDB_CONTEXT *the_tdb, TDB_DATA kbuf, TDB_DATA dbuf, 
                       void *state)
{
	struct locking_data *data;
	struct share_mode_entry *shares;
	const char *sharepath;
	const char *fname;
	int i;
	void (*traverse_callback)(struct share_mode_entry *, const char *, const char *) = state;

	/* Ensure this is a locking_key record. */
	if (kbuf.dsize != sizeof(struct locking_key))
		return 0;

	data = (struct locking_data *)dbuf.dptr;
	shares = (struct share_mode_entry *)(dbuf.dptr + sizeof(*data));
	sharepath = dbuf.dptr + sizeof(*data) +
		data->u.s.num_share_mode_entries*sizeof(*shares);
	fname = dbuf.dptr + sizeof(*data) +
		data->u.s.num_share_mode_entries*sizeof(*shares) +
		strlen(sharepath) + 1;

	for (i=0;i<data->u.s.num_share_mode_entries;i++) {
		traverse_callback(&shares[i], sharepath, fname);
	}
	return 0;
}

/*******************************************************************
 Call the specified function on each entry under management by the
 share mode system.
********************************************************************/

int share_mode_forall(void (*fn)(const struct share_mode_entry *, const char *, const char *))
{
	if (tdb == NULL)
		return 0;
	return tdb_traverse(tdb, traverse_fn, fn);
}
