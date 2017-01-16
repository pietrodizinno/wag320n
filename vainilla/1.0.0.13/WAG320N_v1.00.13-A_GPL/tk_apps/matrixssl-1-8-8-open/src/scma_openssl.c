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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sslSocket.h"
#include "scma_openssl.h"
#include <syslog.h>
struct bio_st
{
    sslConn_t *cp;
    sslKeys_t *keys;
    int (*bwrite)(sslConn_t *cp, char *buf, int len, int *status);
    int (*bread)(sslConn_t *cp, char *buf, int len, int *status);
    char *ip;
    short port;
    struct ssl_st *ssl;
};

struct ssl_st
{
    ssl_t *ssl;
    struct ssl_ctx_st *ctx;
};


struct ssl_ctx_st
{
    int side;  //server or client
    struct ssl_method_st * ssl_method;
    sslConn_t *cp;
    sslKeys_t *keys;
    int loadca;
};

struct ssl_method_st
{
    int mode;
};

#define printf_ssl(fmt, args...)    printf("%s:%d [LIB_SSL]"fmt"\n", __FILE__, __LINE__, ## args)


void OpenSSL_add_all_algorithms(void)
{
    return;
}

int SSL_CTX_load_verify_locations(SSL_CTX *ctx, const char *CAfile,
	const char *CApath)
{
    char * certfile = NULL;
    char * keyfile = NULL;
    char * password = NULL;
    char * path = NULL;
    int r = 0;
    if(CAfile != NULL && CApath != NULL)
    {
        int cafilelen = strlen(CAfile);
        int capathlen = strlen(CApath);
        path = (char *)malloc(cafilelen + capathlen + 1);
        memset(path, 0, cafilelen + capathlen + 1);
        if(CApath[capathlen-1] == '/')
            strncpy(path, CApath, capathlen-1);
        else
            strcpy(path, CApath);
        strcat(path, CAfile);
        r = matrixSslReadKeys(&(ctx->keys), certfile, keyfile, password, path);
        free(path);
    }
    else if(CAfile == NULL && CApath == NULL)
    {
        return 0;
    }
    else if(CAfile != NULL && CApath == NULL)
    {
        r = matrixSslReadKeys(&(ctx->keys), certfile, keyfile, password, (char *)CAfile);
    }else
        return 0;
    ctx->loadca = 1;
    if(r< 0)
        return 0;
    return 1;
}

static SSL_METHOD sslv23_client = { 0 };

SSL_METHOD *SSLv23_client_method()
{
    return &sslv23_client;
}

SSL_CTX *SSL_CTX_new(SSL_METHOD *meth)
{
    SSL_CTX *ctx = NULL;
    openlog("SSL",0,LOG_SYSLOG);
    ctx = (SSL_CTX *)malloc(sizeof(SSL_CTX));
    if(ctx == NULL)
    {
        printf_ssl("SSL_CTX_new malloc fail.");
	    closelog();
        return NULL;
    }
    memset(ctx, 0, sizeof(SSL_CTX));
    ctx->ssl_method = (SSL_METHOD *)malloc(sizeof(SSL_METHOD));
    if(ctx->ssl_method == NULL)
    {
        free(ctx);
        printf_ssl("SSL_CTX_new method null.");
	    closelog();
        return NULL;
    }
    ctx->ssl_method->mode = meth->mode;
    printf_ssl("SSL_CTX_nes return 0x%x",ctx);
    closelog();
    return ctx;
}

void SSL_CTX_free(SSL_CTX *ctx)
{
    openlog("SSL",0,LOG_SYSLOG);

    if(ctx->cp != NULL)
    {
        printf_ssl("SSL_CTX_free ctx->cp != NULL");
        sslWriteClosureAlert(ctx->cp); //向客户端发送关闭警叿        socketShutdown(ctx->cp->fd);
        sslFreeConnection(&ctx->cp);
    }
    if(ctx->keys != NULL)
	{
        printf_ssl("SSL_CTX_free ctx->keys != NULL");
        matrixSslFreeKeys(ctx->keys);
	}
    matrixSslClose();
    if(ctx->ssl_method != NULL)
	{
        printf_ssl("SSL_CTX_free ctx->ssl_method != NULL");
        free(ctx->ssl_method);
	}
	
    free(ctx);
    printf_ssl("SSL_CTX_free ctx freed");

    closelog();

}

BIO *BIO_new_ssl_connect(SSL_CTX *ctx)
{
    BIO *ret = NULL;

    openlog("SSL",0,LOG_SYSLOG);

    ret = malloc(sizeof(BIO));
    memset(ret, 0 ,sizeof(BIO));
    if(ret == NULL)
	{
	    printf_ssl("BIO_new_ssl_connect malloc fail!");
	    closelog();
        return NULL;
	}
    ret->bread = sslRead;
    ret->bwrite = sslWrite;
    ret->cp = ctx->cp;
    ret->keys = ctx->keys;
    ret->ssl = (SSL *)malloc(sizeof(SSL));
    if(ret->ssl == NULL)
    {
        free(ret);
	    printf_ssl("BIO_new_ssl_connect ret->ssl == NULL!");
	    closelog();
        return NULL;
    }
    //cp content from ctx to ret->ssl
    ret->ssl->ctx = ctx;

    closelog();
    return(ret);
}

int BIO_get_ssl(BIO *b,SSL **sslp)
{
    openlog("SSL",0,LOG_SYSLOG);

    *sslp = b->ssl;
    if(*sslp != NULL)
	{
	    printf_ssl("BIO_get_ssl *sslp != NULL!");
	    closelog();
        return -1;
	}
    else
	{
	    printf_ssl("BIO_new_ssl_connect *sslp == NULL");
	    closelog();
        return 1; 
	}
}


/*
#define SSL_MODE_ENABLE_PARTIAL_WRITE       0x00000001L
#define SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER 0x00000002L
#define SSL_MODE_AUTO_RETRY 0x00000004L
#define SSL_MODE_NO_AUTO_CHAIN 0x00000008L
*/
void SSL_set_mode(const SSL *ssl, int mode)
{
    openlog("SSL",0,LOG_SYSLOG);

    printf("d\n");
    ssl->ctx->ssl_method->mode |= mode;
    printf("a\n");
    printf_ssl("SSL_set_mode");
    closelog();

}

int BIO_set_conn_hostname(BIO *b,char *name)
{
    char *p;
    char *ip;
    char *port;

    openlog("SSL",0,LOG_SYSLOG);

    if(name == NULL)
	{  
		printf_ssl("BIO_set_conn_hostname name == NULL");
	    closelog();
        return -1;
	}
    p = strchr(name, ':');
    if(p == NULL)
	{
		printf_ssl("BIO_set_conn_hostname strchr :  == NULL");
	    closelog();
        return -1;
	}
    ip = (char *)malloc(p - name + 1);
    if(ip == NULL)
	{
		printf_ssl("BIO_set_conn_hostname malloc  NULL  1111");
	    closelog();
        return -1;
	}
    memset(ip, 0, p - name + 1);
    strncpy(ip, name, p-name);
    port  = (char *)malloc(name + strlen(name) - p -1 + 1);
    if(port == NULL)
    {
		printf_ssl("BIO_set_conn_hostname malloc  NULL  2222");
        free(ip);
	    closelog();
        return -1;
    }
    memset(port, 0, name + strlen(name) - p -1 + 1);
    strncpy(port , p + 1, name + strlen(name) - p -1);
    b->ip = ip;
    b->port = atoi(port);

	printf_ssl("BIO_set_conn_hostname finish");

    closelog();
    return 1;
}

static int myValidator(sslCertInfo_t *t, void *arg)
{
    return 1;
}

int BIO_do_connect(BIO *b)
{
    char *ip;
    short port;
    int err;
    int ret;
    SOCKET  fd = socketConnect(b->ip, b->port, &err);

    openlog("SSL",0,LOG_SYSLOG);

    if(fd < 0)
	{	
		printf_ssl("BIO_do_connect fd < 0");
	    closelog();
        return -1;
	}
    setSocketBlock(fd);   //将套接字设置为阻塞模庿
    if(b->ssl->ctx->loadca == 1)
    {
		printf_ssl("BIO_do_connect (b->ssl->ctx->loadca == 1)");
        closelog();
        ret = sslConnect(&(b->cp), fd, b->keys, NULL, 0x0000, NULL);
    }
    else
    {
		printf_ssl("BIO_do_connect (b->ssl->ctx->loadca == 1) else");
        closelog();
        ret = sslConnect(&(b->cp), fd, b->keys, NULL, 0x0000, myValidator);
    }
    if(ret < 0)
    {
		printf_ssl("BIO_do_connect (b->ssl->ctx->loadca == 1) else");
        socketShutdown(fd);
	    closelog();
        return -1;
    }

	printf_ssl("BIO_do_connect finish");
    closelog();
    return 1;
}

int BIO_read(BIO *b, void *out, int outl)
{
    int i;
    int status = 0;
    openlog("SSL",0,LOG_SYSLOG);

    i = b->bread(b->cp, (char *)out, outl, &status);
	
	printf_ssl("BIO_read finish");
    closelog();

    return(i);
}

int BIO_write(BIO *b, const void *in, int inl)
{
    int i;
    int status = 0;
    openlog("SSL",0,LOG_SYSLOG);

    i = (b->bwrite)(b->cp, (char *)in, inl, &status);
	
	printf_ssl("BIO_write finish");
    closelog();

    return(i);
}

int BIO_free(BIO *a)
{
    openlog("SSL",0,LOG_SYSLOG);

    if(a->ssl != NULL)
	{
		printf_ssl("BIO_free (a->ssl != NULL) ");

        free(a->ssl);
	}

	printf_ssl("BIO_free finish");

    free(a);
    closelog();

}

int SSL_library_init(void )
{ 
	return 1;/*Is useless for matrixSSL*/
}


