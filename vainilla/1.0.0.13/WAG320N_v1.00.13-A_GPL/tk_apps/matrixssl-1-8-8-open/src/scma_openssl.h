/*
 Copyright - 2005 SerComm Corporation.

 This program is free software; you can distribute it and/or modify it
 under the terms of the GNU General Public License (Version 2) as
 published by the Free Software Foundation.

 This program is distributed in the hope it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 for more details.

 You should have received a copy of the GNU General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
*/

#ifndef HEADER_SSL_H 
#define HEADER_SSL_H 

#include "sslSocket.h"


#define VERSION 0.01

#define SSL_MODE_ENABLE_PARTIAL_WRITE       0x00000001L
/* Make it possible to retry SSL_write() with changed buffer location
 * (buffer contents must stay the same!); this is not the default to avoid
 * the misconception that non-blocking SSL_write() behaves like
 * non-blocking write(): */
#define SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER 0x00000002L
/* Never bother the application with retries if the transport
 * is blocking: */
#define SSL_MODE_AUTO_RETRY 0x00000004L
/* Don't attempt to automatically build certificate chain */
#define SSL_MODE_NO_AUTO_CHAIN 0x00000008L

struct bio_st;
struct ssl_st;
struct ssl_ctx_st;
struct ssl_method_st;

typedef struct bio_st BIO;
typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;
typedef struct ssl_method_st SSL_METHOD;

SSL_METHOD *SSLv23_client_method();

void OpenSSL_add_all_algorithms(void);

SSL_CTX *SSL_CTX_new(SSL_METHOD *meth);
void	SSL_CTX_free(SSL_CTX *);

int SSL_CTX_load_verify_locations(SSL_CTX *ctx, const char *CAfile,
	const char *CApath);
BIO *BIO_new_ssl_connect(SSL_CTX *ctx);
int BIO_get_ssl(BIO *b,SSL **sslp);
void SSL_set_mode(const SSL *ssl, int mode);
int BIO_set_conn_hostname(BIO *b,char *name); //mush be ip:port
int BIO_do_connect(BIO *b);

int	BIO_write(BIO *b, const void *data, int len);
int	BIO_read(BIO *b, void *data, int len);
int	BIO_free(BIO *a);
int SSL_library_init(void );
#endif

