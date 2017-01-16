/* 
   Unix SMB/CIFS implementation.

   Winbind child daemons

   Copyright (C) Andrew Tridgell 2002
   Copyright (C) Volker Lendecke 2004,2005
   
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

/*
 * We fork a child per domain to be able to act non-blocking in the main
 * winbind daemon. A domain controller thousands of miles away being being
 * slow replying with a 10.000 user list should not hold up netlogon calls
 * that can be handled locally.
 */

#include "includes.h"
#include "winbindd.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_WINBIND

/* Read some data from a client connection */

static void child_read_request(struct winbindd_cli_state *state)
{
	ssize_t len;

	/* Read data */

	len = read_data(state->sock, (char *)&state->request,
			sizeof(state->request));

	if (len != sizeof(state->request)) {
		DEBUG(0, ("Got invalid request length: %d\n", (int)len));
		state->finished = True;
		return;
	}

	if (state->request.extra_len == 0) {
		state->request.extra_data = NULL;
		return;
	}

	DEBUG(10, ("Need to read %d extra bytes\n", (int)state->request.extra_len));

	state->request.extra_data =
		SMB_MALLOC_ARRAY(char, state->request.extra_len + 1);

	if (state->request.extra_data == NULL) {
		DEBUG(0, ("malloc failed\n"));
		state->finished = True;
		return;
	}

	/* Ensure null termination */
	state->request.extra_data[state->request.extra_len] = '\0';

	len = read_data(state->sock, state->request.extra_data,
			state->request.extra_len);

	if (len != state->request.extra_len) {
		DEBUG(0, ("Could not read extra data\n"));
		state->finished = True;
		return;
	}
}

/*
 * Machinery for async requests sent to children. You set up a
 * winbindd_request, select a child to query, and issue a async_request
 * call. When the request is completed, the callback function you specified is
 * called back with the private pointer you gave to async_request.
 */

struct winbindd_async_request {
	struct winbindd_async_request *next, *prev;
	TALLOC_CTX *mem_ctx;
	struct winbindd_child *child;
	struct winbindd_request *request;
	struct winbindd_response *response;
	void (*continuation)(void *private_data, BOOL success);
	void *private_data;
};

static void async_main_request_sent(void *private_data, BOOL success);
static void async_request_sent(void *private_data, BOOL success);
static void async_reply_recv(void *private_data, BOOL success);
static void schedule_async_request(struct winbindd_child *child);

void async_request(TALLOC_CTX *mem_ctx, struct winbindd_child *child,
		   struct winbindd_request *request,
		   struct winbindd_response *response,
		   void (*continuation)(void *private_data, BOOL success),
		   void *private_data)
{
	struct winbindd_async_request *state, *tmp;

	SMB_ASSERT(continuation != NULL);

	state = TALLOC_P(mem_ctx, struct winbindd_async_request);

	if (state == NULL) {
		DEBUG(0, ("talloc failed\n"));
		continuation(private_data, False);
		return;
	}

	state->mem_ctx = mem_ctx;
	state->child = child;
	state->request = request;
	state->response = response;
	state->continuation = continuation;
	state->private_data = private_data;

	DLIST_ADD_END(child->requests, state, tmp);

	schedule_async_request(child);

	return;
}

static void async_main_request_sent(void *private_data, BOOL success)
{
	struct winbindd_async_request *state =
		talloc_get_type_abort(private_data, struct winbindd_async_request);

	if (!success) {
		DEBUG(5, ("Could not send async request\n"));

		state->response->length = sizeof(struct winbindd_response);
		state->response->result = WINBINDD_ERROR;
		state->continuation(state->private_data, False);
		return;
	}

	if (state->request->extra_len == 0) {
		async_request_sent(private_data, True);
		return;
	}

	setup_async_write(&state->child->event, state->request->extra_data,
			  state->request->extra_len,
			  async_request_sent, state);
}

static void async_request_sent(void *private_data_data, BOOL success)
{
	struct winbindd_async_request *state =
		talloc_get_type_abort(private_data_data, struct winbindd_async_request);

	if (!success) {
		DEBUG(5, ("Could not send async request\n"));

		state->response->length = sizeof(struct winbindd_response);
		state->response->result = WINBINDD_ERROR;
		state->continuation(state->private_data, False);
		return;
	}

	/* Request successfully sent to the child, setup the wait for reply */

	setup_async_read(&state->child->event,
			 &state->response->result,
			 sizeof(state->response->result),
			 async_reply_recv, state);
}

static void async_reply_recv(void *private_data, BOOL success)
{
	struct winbindd_async_request *state =
		talloc_get_type_abort(private_data, struct winbindd_async_request);
	struct winbindd_child *child = state->child;

	state->response->length = sizeof(struct winbindd_response);

	if (!success) {
		DEBUG(5, ("Could not receive async reply\n"));
		state->response->result = WINBINDD_ERROR;
		return;
	}

	SMB_ASSERT(cache_retrieve_response(child->pid,
					   state->response));

	DLIST_REMOVE(child->requests, state);

	schedule_async_request(child);

	state->continuation(state->private_data, True);
}

static BOOL fork_domain_child(struct winbindd_child *child);

static void schedule_async_request(struct winbindd_child *child)
{
	struct winbindd_async_request *request = child->requests;

	if (request == NULL) {
		return;
	}

	if (child->event.flags != 0) {
		return;		/* Busy */
	}

	if ((child->pid == 0) && (!fork_domain_child(child))) {
		/* Cancel all outstanding requests */

		while (request != NULL) {
			/* request might be free'd in the continuation */
			struct winbindd_async_request *next = request->next;
			request->continuation(request->private_data, False);
			request = next;
		}
		return;
	}

	setup_async_write(&child->event, request->request,
			  sizeof(*request->request),
			  async_main_request_sent, request);
	return;
}

struct domain_request_state {
	TALLOC_CTX *mem_ctx;
	struct winbindd_domain *domain;
	struct winbindd_request *request;
	struct winbindd_response *response;
	void (*continuation)(void *private_data_data, BOOL success);
	void *private_data_data;
};

static void domain_init_recv(void *private_data_data, BOOL success);

void async_domain_request(TALLOC_CTX *mem_ctx,
			  struct winbindd_domain *domain,
			  struct winbindd_request *request,
			  struct winbindd_response *response,
			  void (*continuation)(void *private_data_data, BOOL success),
			  void *private_data_data)
{
	struct domain_request_state *state;

	if (domain->initialized) {
		async_request(mem_ctx, &domain->child, request, response,
			      continuation, private_data_data);
		return;
	}

	state = TALLOC_P(mem_ctx, struct domain_request_state);
	if (state == NULL) {
		DEBUG(0, ("talloc failed\n"));
		continuation(private_data_data, False);
		return;
	}

	state->mem_ctx = mem_ctx;
	state->domain = domain;
	state->request = request;
	state->response = response;
	state->continuation = continuation;
	state->private_data_data = private_data_data;

	init_child_connection(domain, domain_init_recv, state);
}

static void recvfrom_child(void *private_data_data, BOOL success)
{
	struct winbindd_cli_state *state =
		talloc_get_type_abort(private_data_data, struct winbindd_cli_state);
	enum winbindd_result result = state->response.result;

	/* This is an optimization: The child has written directly to the
	 * response buffer. The request itself is still in pending state,
	 * state that in the result code. */

	state->response.result = WINBINDD_PENDING;

	if ((!success) || (result != WINBINDD_OK)) {
		request_error(state);
		return;
	}

	request_ok(state);
}

void sendto_child(struct winbindd_cli_state *state,
		  struct winbindd_child *child)
{
	async_request(state->mem_ctx, child, &state->request,
		      &state->response, recvfrom_child, state);
}

void sendto_domain(struct winbindd_cli_state *state,
		   struct winbindd_domain *domain)
{
	async_domain_request(state->mem_ctx, domain,
			     &state->request, &state->response,
			     recvfrom_child, state);
}

static void domain_init_recv(void *private_data_data, BOOL success)
{
	struct domain_request_state *state =
		talloc_get_type_abort(private_data_data, struct domain_request_state);

	if (!success) {
		DEBUG(5, ("Domain init returned an error\n"));
		state->continuation(state->private_data_data, False);
		return;
	}

	async_request(state->mem_ctx, &state->domain->child,
		      state->request, state->response,
		      state->continuation, state->private_data_data);
}

struct winbindd_child_dispatch_table {
	enum winbindd_cmd cmd;
	enum winbindd_result (*fn)(struct winbindd_domain *domain,
				   struct winbindd_cli_state *state);
	const char *winbindd_cmd_name;
};

static struct winbindd_child_dispatch_table child_dispatch_table[] = {
	
	{ WINBINDD_LOOKUPSID,            winbindd_dual_lookupsid,             "LOOKUPSID" },
	{ WINBINDD_LOOKUPNAME,           winbindd_dual_lookupname,            "LOOKUPNAME" },
	{ WINBINDD_LIST_TRUSTDOM,        winbindd_dual_list_trusted_domains,  "LIST_TRUSTDOM" },
	{ WINBINDD_INIT_CONNECTION,      winbindd_dual_init_connection,       "INIT_CONNECTION" },
	{ WINBINDD_GETDCNAME,            winbindd_dual_getdcname,             "GETDCNAME" },
	{ WINBINDD_SHOW_SEQUENCE,        winbindd_dual_show_sequence,         "SHOW_SEQUENCE" },
	{ WINBINDD_PAM_AUTH,             winbindd_dual_pam_auth,              "PAM_AUTH" },
	{ WINBINDD_PAM_AUTH_CRAP,        winbindd_dual_pam_auth_crap,         "AUTH_CRAP" },
	{ WINBINDD_CHECK_MACHACC,        winbindd_dual_check_machine_acct,    "CHECK_MACHACC" },
	{ WINBINDD_DUAL_SID2UID,         winbindd_dual_sid2uid,               "DUAL_SID2UID" },
	{ WINBINDD_DUAL_SID2GID,         winbindd_dual_sid2gid,               "DUAL_SID2GID" },
	{ WINBINDD_DUAL_UID2NAME,        winbindd_dual_uid2name,              "DUAL_UID2NAME" },
	{ WINBINDD_DUAL_NAME2UID,        winbindd_dual_name2uid,              "DUAL_NAME2UID" },
	{ WINBINDD_DUAL_GID2NAME,        winbindd_dual_gid2name,              "DUAL_GID2NAME" },
	{ WINBINDD_DUAL_NAME2GID,        winbindd_dual_name2gid,              "DUAL_NAME2GID" },
	{ WINBINDD_DUAL_IDMAPSET,        winbindd_dual_idmapset,              "DUAL_IDMAPSET" },
	{ WINBINDD_DUAL_USERINFO,        winbindd_dual_userinfo,              "DUAL_USERINFO" },
	{ WINBINDD_ALLOCATE_RID,         winbindd_dual_allocate_rid,          "ALLOCATE_RID" },
	{ WINBINDD_ALLOCATE_RID_AND_GID, winbindd_dual_allocate_rid_and_gid,  "ALLOCATE_RID_AND_GID" },
	{ WINBINDD_GETUSERDOMGROUPS,     winbindd_dual_getuserdomgroups,      "GETUSERDOMGROUPS" },
	{ WINBINDD_DUAL_GETSIDALIASES,   winbindd_dual_getsidaliases,         "GETSIDALIASES" },
	/* End of list */

	{ WINBINDD_NUM_CMDS, NULL, "NONE" }
};

static void child_process_request(struct winbindd_domain *domain,
				  struct winbindd_cli_state *state)
{
	struct winbindd_child_dispatch_table *table;

	/* Free response data - we may be interrupted and receive another
	   command before being able to send this data off. */

	state->response.result = WINBINDD_ERROR;
	state->response.length = sizeof(struct winbindd_response);

	state->mem_ctx = talloc_init("winbind request");
	if (state->mem_ctx == NULL)
		return;

	/* Process command */

	for (table = child_dispatch_table; table->fn; table++) {
		if (state->request.cmd == table->cmd) {
			DEBUG(10,("process_request: request fn %s\n",
				  table->winbindd_cmd_name ));
			state->response.result = table->fn(domain, state);
			break;
		}
	}

	if (!table->fn) {
		DEBUG(10,("process_request: unknown request fn number %d\n",
			  (int)state->request.cmd ));
		state->response.result = WINBINDD_ERROR;
	}

	talloc_destroy(state->mem_ctx);
}

void setup_domain_child(struct winbindd_domain *domain,
			struct winbindd_child *child,
			const char *explicit_logfile)
{
	if (explicit_logfile != NULL) {
		pstr_sprintf(child->logfilename, "%s/log.winbindd-%s",
			     dyn_LOGFILEBASE, explicit_logfile);
	} else if (domain != NULL) {
		pstr_sprintf(child->logfilename, "%s/log.wb-%s",
			     dyn_LOGFILEBASE, domain->name);
	} else {
		smb_panic("Internal error: domain == NULL && "
			  "explicit_logfile == NULL");
	}

	child->domain = domain;
}

struct winbindd_child *children = NULL;

void winbind_child_died(pid_t pid)
{
	struct winbindd_child *child;

	for (child = children; child != NULL; child = child->next) {
		if (child->pid == pid) {
			break;
		}
	}

	if (child == NULL) {
		DEBUG(0, ("Unknown child %d died!\n", pid));
		return;
	}

	remove_fd_event(&child->event);
	close(child->event.fd);
	child->event.fd = 0;
	child->event.flags = 0;
	child->pid = 0;

	schedule_async_request(child);
}

static BOOL fork_domain_child(struct winbindd_child *child)
{
	int fdpair[2];
	struct winbindd_cli_state state;
	extern BOOL override_logfile;

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, fdpair) != 0) {
		DEBUG(0, ("Could not open child pipe: %s\n",
			  strerror(errno)));
		return False;
	}

	ZERO_STRUCT(state);
	state.pid = getpid();

	child->pid = sys_fork();

	if (child->pid == -1) {
		DEBUG(0, ("Could not fork: %s\n", strerror(errno)));
		return False;
	}

	if (child->pid != 0) {
		/* Parent */
		close(fdpair[0]);
		child->next = child->prev = NULL;
		DLIST_ADD(children, child);
		child->event.fd = fdpair[1];
		child->event.flags = 0;
		child->requests = NULL;
		add_fd_event(&child->event);
		return True;
	}

	/* Child */

	state.sock = fdpair[0];
	close(fdpair[1]);

	/* tdb needs special fork handling */
	if (tdb_reopen_all() == -1) {
		DEBUG(0,("tdb_reopen_all failed.\n"));
		_exit(0);
	}

	close_conns_after_fork();

	if (!override_logfile) {
		lp_set_logfile(child->logfilename);
		reopen_logs();
	}
	
	while (1) {
		/* free up any talloc memory */
		lp_talloc_free();
		main_loop_talloc_free();

		/* fetch a request from the main daemon */
		child_read_request(&state);

		if (state.finished) {
			/* we lost contact with our parent */
			exit(0);
		}

		DEBUG(4,("child daemon request %d\n", (int)state.request.cmd));

		ZERO_STRUCT(state.response);
		state.request.null_term = '\0';
		child_process_request(child->domain, &state);

		SAFE_FREE(state.request.extra_data);

		cache_store_response(sys_getpid(), &state.response);

		SAFE_FREE(state.response.extra_data);

		/* We just send the result code back, the result
		 * structure needs to be fetched via the
		 * winbindd_cache. Hmm. That needs fixing... */

		if (write_data(state.sock, (void *)&state.response.result,
			       sizeof(state.response.result)) !=
		    sizeof(state.response.result)) {
			DEBUG(0, ("Could not write result\n"));
			exit(1);
		}
	}
}
