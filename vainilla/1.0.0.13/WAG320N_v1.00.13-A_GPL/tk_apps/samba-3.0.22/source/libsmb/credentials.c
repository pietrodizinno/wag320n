/* 
   Unix SMB/CIFS implementation.
   code to manipulate domain credentials
   Copyright (C) Andrew Tridgell 1997-1998
   Largely rewritten by Jeremy Allison 2005.
   
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

/****************************************************************************
 Represent a credential as a string.
****************************************************************************/

char *credstr(const uchar *cred)
{
	static fstring buf;
	slprintf(buf, sizeof(buf) - 1, "%02X%02X%02X%02X%02X%02X%02X%02X",
		cred[0], cred[1], cred[2], cred[3], 
		cred[4], cred[5], cred[6], cred[7]);
	return buf;
}


/****************************************************************************
 Setup the session key. 
 Input: 8 byte challenge block
       8 byte server challenge block
      16 byte md4 encrypted password
 Output:
      16 byte session key (last 8 bytes zero).
****************************************************************************/

static void cred_create_session_key(const DOM_CHAL *clnt_chal_in,
			const DOM_CHAL *srv_chal_in,
			const uchar *pass_in, 
			uchar session_key_out[16])
{
	uint32 sum[2];
	unsigned char sum2[8];

	sum[0] = IVAL(clnt_chal_in->data, 0) + IVAL(srv_chal_in->data, 0);
	sum[1] = IVAL(clnt_chal_in->data, 4) + IVAL(srv_chal_in->data, 4);

	SIVAL(sum2,0,sum[0]);
	SIVAL(sum2,4,sum[1]);

	cred_hash1(session_key_out, sum2, pass_in);
	memset(&session_key_out[8], '\0', 8);

	/* debug output */
	DEBUG(4,("cred_create_session_key\n"));

	DEBUG(5,("	clnt_chal_in: %s\n", credstr(clnt_chal_in->data)));
	DEBUG(5,("	srv_chal_in : %s\n", credstr(srv_chal_in->data)));
	DEBUG(5,("	clnt+srv : %s\n", credstr(sum2)));
	DEBUG(5,("	sess_key_out : %s\n", credstr(session_key_out)));
}

/****************************************************************************
 Utility function to step credential chain one forward.
 Deliberately doesn't update the seed. See reseed comment below.
****************************************************************************/

static void creds_step(struct dcinfo *dc)
{
	DOM_CHAL time_chal;

	DEBUG(5,("\tsequence = 0x%x\n", (unsigned int)dc->sequence ));

	DEBUG(5,("\tseed:        %s\n", credstr(dc->seed_chal.data) ));

	SIVAL(time_chal.data, 0, IVAL(dc->seed_chal.data, 0) + dc->sequence);
	SIVAL(time_chal.data, 4, IVAL(dc->seed_chal.data, 4));
                                                                                                   
	DEBUG(5,("\tseed+seq   %s\n", credstr(time_chal.data) ));

	cred_hash2(dc->clnt_chal.data, time_chal.data, dc->sess_key);

	DEBUG(5,("\tCLIENT      %s\n", credstr(dc->clnt_chal.data) ));

	SIVAL(time_chal.data, 0, IVAL(dc->seed_chal.data, 0) + dc->sequence + 1);
	SIVAL(time_chal.data, 4, IVAL(dc->seed_chal.data, 4));

	DEBUG(5,("\tseed+seq+1   %s\n", credstr(time_chal.data) ));

	cred_hash2(dc->srv_chal.data, time_chal.data, dc->sess_key);

	DEBUG(5,("\tSERVER      %s\n", credstr(dc->srv_chal.data) ));
}


/****************************************************************************
 Create a server credential struct.
****************************************************************************/

void creds_server_init(struct dcinfo *dc,
			DOM_CHAL *clnt_chal,
			DOM_CHAL *srv_chal,
			const char mach_pw[16],
			DOM_CHAL *init_chal_out)
{
	DEBUG(10,("creds_server_init: client chal : %s\n", credstr(clnt_chal->data) ));
	DEBUG(10,("creds_server_init: server chal : %s\n", credstr(srv_chal->data) ));
	dump_data_pw("creds_server_init: machine pass", (const unsigned char *)mach_pw, 16);

	/* Just in case this isn't already there */
	memcpy(dc->mach_pw, mach_pw, 16);

	/* Generate the session key. */
	cred_create_session_key(clnt_chal,		/* Stored client challenge. */
				srv_chal,		/* Stored server challenge. */
				dc->mach_pw,		  /* input machine password. */
				dc->sess_key);		  /* output session key. */

	dump_data_pw("creds_server_init: session key", dc->sess_key, 16);

	/* Generate the next client and server creds. */
	cred_hash2(dc->clnt_chal.data,			/* output */
			clnt_chal->data,		/* input */
			dc->sess_key);			/* input */

	cred_hash2(dc->srv_chal.data,			/* output */
			srv_chal->data,			/* input */
			dc->sess_key);			/* input */

	/* Seed is the client chal. */
	memcpy(dc->seed_chal.data, dc->clnt_chal.data, 8);

	DEBUG(10,("creds_server_init: clnt : %s\n", credstr(dc->clnt_chal.data) ));
	DEBUG(10,("creds_server_init: server : %s\n", credstr(dc->srv_chal.data) ));
	DEBUG(10,("creds_server_init: seed : %s\n", credstr(dc->seed_chal.data) ));

	memcpy(init_chal_out->data, dc->srv_chal.data, 8);
}

/****************************************************************************
 Check a credential sent by the client.
****************************************************************************/

BOOL creds_server_check(const struct dcinfo *dc, const DOM_CHAL *rcv_cli_chal_in)
{
	if (memcmp(dc->clnt_chal.data, rcv_cli_chal_in->data, 8)) {
		DEBUG(5,("creds_server_check: challenge : %s\n", credstr(rcv_cli_chal_in->data)));
		DEBUG(5,("calculated: %s\n", credstr(dc->clnt_chal.data)));
		DEBUG(2,("creds_server_check: credentials check failed.\n"));
		return False;
	}
	DEBUG(10,("creds_server_check: credentials check OK.\n"));
	return True;
}

/****************************************************************************
 Replace current seed chal. Internal function - due to split server step below.
****************************************************************************/

static void creds_reseed(struct dcinfo *dc)
{
	DOM_CHAL time_chal;

	SIVAL(time_chal.data, 0, IVAL(dc->seed_chal.data, 0) + dc->sequence + 1);
	SIVAL(time_chal.data, 4, IVAL(dc->seed_chal.data, 4));

	dc->seed_chal = time_chal;

	DEBUG(5,("cred_reseed: seed %s\n", credstr(dc->seed_chal.data) ));
}

/****************************************************************************
 Step the server credential chain one forward. 
****************************************************************************/

BOOL creds_server_step(struct dcinfo *dc, const DOM_CRED *received_cred, DOM_CRED *cred_out)
{
	dc->sequence = received_cred->timestamp.time;

	creds_step(dc);

	/* Create the outgoing credentials */
	cred_out->timestamp.time = dc->sequence + 1;
	cred_out->challenge = dc->srv_chal;

	creds_reseed(dc);

	return creds_server_check(dc, &received_cred->challenge);
}

/****************************************************************************
 Create a client credential struct.
****************************************************************************/

void creds_client_init(struct dcinfo *dc,
			DOM_CHAL *clnt_chal,
			DOM_CHAL *srv_chal,
			const unsigned char mach_pw[16],
			DOM_CHAL *init_chal_out)
{
	dc->sequence = time(NULL);

	DEBUG(10,("creds_client_init: client chal : %s\n", credstr(clnt_chal->data) ));
	DEBUG(10,("creds_client_init: server chal : %s\n", credstr(srv_chal->data) ));
	dump_data_pw("creds_client_init: machine pass", (const unsigned char *)mach_pw, 16);

	/* Just in case this isn't already there */
	memcpy(dc->mach_pw, mach_pw, 16);

	/* Generate the session key. */
	cred_create_session_key(clnt_chal,		/* Stored client challenge. */
				srv_chal,		/* Stored server challenge. */
				dc->mach_pw,		  /* input machine password. */
				dc->sess_key);		  /* output session key. */

	dump_data_pw("creds_client_init: session key", dc->sess_key, 16);

	/* Generate the next client and server creds. */
	cred_hash2(dc->clnt_chal.data,	/* output */
			clnt_chal->data,		/* input */
			dc->sess_key);			/* input */

	cred_hash2(dc->srv_chal.data,		/* output */
			srv_chal->data,			/* input */
			dc->sess_key);			/* input */

	/* Seed is the client cred. */
	memcpy(dc->seed_chal.data, dc->clnt_chal.data, 8);

	DEBUG(10,("creds_client_init: clnt : %s\n", credstr(dc->clnt_chal.data) ));
	DEBUG(10,("creds_client_init: server : %s\n", credstr(dc->srv_chal.data) ));
	DEBUG(10,("creds_client_init: seed : %s\n", credstr(dc->seed_chal.data) ));

	memcpy(init_chal_out->data, dc->clnt_chal.data, 8);
}

/****************************************************************************
 Check a credential returned by the server.
****************************************************************************/

BOOL creds_client_check(const struct dcinfo *dc, const DOM_CHAL *rcv_srv_chal_in)
{
	if (memcmp(dc->srv_chal.data, rcv_srv_chal_in->data, 8)) {
		DEBUG(5,("creds_client_check: challenge : %s\n", credstr(rcv_srv_chal_in->data)));
		DEBUG(5,("calculated: %s\n", credstr(dc->srv_chal.data)));
		DEBUG(0,("creds_client_check: credentials check failed.\n"));
		return False;
	}
	DEBUG(10,("creds_client_check: credentials check OK.\n"));
	return True;
}

/****************************************************************************
  Step the client credentials to the next element in the chain, updating the
  current client and server credentials and the seed
  produce the next authenticator in the sequence ready to send to
  the server
****************************************************************************/

void creds_client_step(struct dcinfo *dc, DOM_CRED *next_cred_out)
{
        dc->sequence += 2;
	creds_step(dc);
	creds_reseed(dc);

	next_cred_out->challenge = dc->clnt_chal;
	next_cred_out->timestamp.time = dc->sequence;
}
