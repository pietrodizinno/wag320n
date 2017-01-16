#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "commands.h"
#include "main.h"
#include "libcase.h"
#include "filename.h"

char *convertDir(char *dirname,usershare *p_usershare,char *current_share)
{
	char *p1=NULL,*p2=NULL,*p3=NULL,*p4=NULL;
	char dir1[MAXCMD + 1];
	char cwd[MAX_FILE_LENGTH];
      
	if(!dirname||!dirname[0]||!p_usershare)
		return NULL;
	getcwd(cwd, sizeof(cwd) - 1);
	if(strcmp(cwd,"/")==0&&dirname[0]=='.')
		strcpy(dir1,dirname+1);
	else
		strcpy(dir1,dirname);	
	p1=strtok(dir1,"/");
	if(!p1)  
		return NULL;
	else{
		p3=isShare(p1,p_usershare);//p1: Share Name, p3: Share's Directory
		if(!p3)
			return NULL;
		else{
			p2=strtok(NULL,"");//p2: Subdirectory
			if(!p2){
				if(current_share)
					strcpy(current_share,p1);
				return p3;
			}    
			else{
				if(p3[strlen(p3)-1]!='/')
					p4=malloc(strlen(p2)+strlen(p3)+2);
				else
					p4=malloc(strlen(p2)+strlen(p3)+1);
				if(!p4){
					free(p3);
					return NULL;                             
				}      
				else{
					if(p3[strlen(p3)-1]!='/')
						sprintf(p4,"%s/%s",p3,p2);
					else
						sprintf(p4,"%s%s",p3,p2);
					free(p3);
					if(current_share)
						strcpy(current_share,p1);
					return p4;
				}       
			}    
		}	
	}
	if(current_share)
		current_share[0]=0;
	return NULL;
}
 
char *isShare(char *p1,usershare *shares)
{
    char *shareDir;
    usershare *current;   
	
    if(!p1||!shares)
         return NULL;
    current=shares;
    while(current){
		if (strequal (p1, current->share)) {
             shareDir=malloc(strlen(current->dir)+1);
             if(!shareDir)
                 return NULL;
              strcpy(shareDir,current->dir);
              return shareDir;
         }
	 current=current->next;
     }
     return NULL;
}             
  
int getUserShare(char *usr,usershare **shares)
{
 	sh_list *shlist=NULL; 
	u_list *u=NULL;
 	usershare *newitem,*tail=NULL;
 	int find=0;
 	
	for(shlist=shinfo; shlist; shlist=shlist->pNext){
		find=0; 

		u=shlist->pUser;
		while(u){
			if(!strcmp(usr,u->name)){   
				find=1;
				break;
			} 	 
			u=u->pNext;
		}
		if(find){      
			newitem=(usershare *)malloc(sizeof(usershare));  	
			if(!newitem)
				goto err;
			newitem->next=NULL;
			if(!tail)
				*shares=tail=newitem;
			else{
				tail->next=newitem;
				tail=tail->next;
			}
			newitem->share=malloc(strlen(shlist->sh.name)+1);
			if(!(newitem->share))
				goto err;
			newitem->dir = malloc (strlen (shlist->sh.dir) + 50);
			if(!(newitem->dir))
      	             goto err;	
			strcpy(newitem->share,shlist->sh.name);
			sprintf(newitem->dir,"/usb_%d%s",shlist->sh.hard_index,shlist->sh.dir);
		}        
	}
	return 0;
err:
     freeUserShare(*shares);
     return 1;
}

int getShareDir(char *sharename,char *dir)
{
	usershare *current=NULL;
	int len=0;
   	
	if(!share||!dir)
		return -1;
	current=share;
	while(current){ 
		if (strequal (current->share, sharename)) {
			strcpy(dir,current->dir);
			len=strlen(dir)-1;
			if(dir[len]=='/'&&len!=0)
				dir[len]=0;
			return 0;
		}	        
		current=current->next;
	}
	return 0;        	
}	 
 
void freeUserShare(usershare *shares)
{
	usershare *current;
	
	current=shares;
	while(current){
		if(current->share)
			free(current->share);
		if(current->dir)
			free(current->dir);   
		current=current->next;
	}
	return;
}		      
	
int folderIsShare(char *folder,usershare *shares)
{
    usershare *current;
    char nas_sh_name[FSH_MaxShareLen+6];
	char cwd[MAX_FILE_LENGTH]={0};
    
    if(!folder||!folder[0])
        return 0;
    if(strstr(folder,"/.")||strstr(folder,"../"))
		return 1;    
    getcwd(cwd, sizeof(cwd) - 1);    
    current=shares;
    while(current){
		strcpy(nas_sh_name, current->share);
        if(strequal(folder,nas_sh_name)&&strstr(pwd,"/usb_")!=pwd&&!strcmp(cwd,"/"))
            return 1;
        sprintf(nas_sh_name,"/%s",current->share);
        if(strequal(folder,nas_sh_name))
            return 1;
        sprintf(nas_sh_name,"/%s/",current->share);
        if(strequal(folder,nas_sh_name))
            return 1;
        sprintf(nas_sh_name,"./%s",current->share);
        if(strequal(folder,nas_sh_name)&&strstr(pwd,"/usb_")!=pwd&&!strcmp(cwd,"/"))
            return 1;
        sprintf(nas_sh_name,"./%s/",current->share);
        if(strequal(folder,nas_sh_name)&&strstr(pwd,"/usb_")!=pwd&&!strcmp(cwd,"/"))
            return 1;
		sprintf(nas_sh_name,"%s/",current->share);
        if(strequal(folder,nas_sh_name)&&strstr(pwd,"/usb_")!=pwd&&!strcmp(cwd,"/"))
            return 1;
        current=current->next;
    }
    return 0;       
}	

void getUpDir(char *dir)
{
	char *p1;
	int len=0;
  	
	if(!dir)
		return;
	len=strlen(dir);
	if(len>1){
		len--;
		while(*(dir+len)=='/'){
			*(dir+len)='\0';
			len--;	
		}	     
	}
	p1=strrchr(dir,'/');
	if(!p1)
		return;
	if(p1==dir){
		strcpy(dir,"/");     	
		return;
	}
	*p1=0;
	return;  
}

int checkFolder(char *filename,usershare *shares,int *readonly, int *convert, int flag)
{
	char file[MAXCMD + 1],*p1=NULL;
	char cwd[MAX_FILE_LENGTH];
	usershare *current=NULL;
	sh_list *shlist=NULL; 
	u_list *u=NULL;
	int len;
   
	if(!filename||!shares)
		return 1;
	getcwd(cwd, sizeof(cwd) - 1);
	if(strncmp(filename,"..",2)==0)
		return 1;
	if(strcmp(cwd,"/")==0&&filename[0]=='.')
		filename++; 
	if(strcmp(cwd,"/")==0&&*filename=='\0')
		*filename='/';
	if(!strcmp(filename,"/")){
		if(flag)
			return 2;  	
		if(readonly)
			*readonly=0;	    
		return 0;
	}
	len=strlen(filename)-1;
	if(strstr(filename,"../")==filename||strstr(filename,"/../")||(filename[len]=='.' && filename[len-1]=='.' && filename[len-2]=='/')) 
		return 1;
	strcpy(file,filename);          
	p1=strtok(file,"/");
	if(!p1)
		return 1;
	if (!strcmp (cwd, "/") && strstr (pwd, "/usb_") != pwd) {
		current=shares;   
		while(current){
			if (strequal (p1, current->share)) {
				if(flag){
					if(folderIsShare(filename,shares))
						return 2;
				}
				if(readonly){
					for(shlist=shinfo;shlist;shlist=shlist->pNext){
						if(!strequal(shlist->sh.name,p1))
							continue;   
						u=shlist->pUser;
						while(u){
							if(!strcmp(user,u->name)){      	
								*readonly=(u->nAccess==7)?1:0;
								return 0;
							}   
							u=u->pNext;
						}
					}	
					*readonly=0;
					return 0;
				}	 
				return 0;
			}//if(!strcmp(p1,current->share)){   
			current=current->next;
        }
        if(flag){
			return 2;
		}
        else{
			sh_list *pCurrent=NULL;
	        
	  		pCurrent=shinfo; 
			while(pCurrent){  
				if(strequal(pCurrent->sh.name, p1))	{
					return 2;
				}
				pCurrent=pCurrent->pNext;
			}        
			return 1;
		}
	}
	else{
		if(filename[0]=='/'){
			current=shares;   
			while(current){
				if(strequal(p1,current->share)){
					if(flag){
						if(folderIsShare(filename,shares))
							return 2;
					}  
					if(readonly){
						for(shlist=shinfo;shlist;shlist=shlist->pNext){
							if(!strequal(shlist->sh.name,p1))
								continue;   
							u=shlist->pUser;
							while(u){
								if(!strcmp(user,u->name)){      	
									*readonly=(u->nAccess==7)?1:0;
									return 0;
								}   
								u=u->pNext;
							}
						}	
						*readonly=0;
						return 0;
					}  
					return 0; 
				} //if(!strequal(p1,current->share))     
				current=current->next;
			}
			if(flag)
				return 2;
			else{
				sh_list *pCurrent=NULL;
	        
				pCurrent=shinfo; 
				while(pCurrent){  
					if(!strcmp(pCurrent->sh.name, p1))	
						return 2;
					pCurrent=pCurrent->pNext;
				}        
				return 1; 
			}
		}
		else{
			if(convert) 
				*convert=0;      
			if(readonly){
				for(shlist=shinfo;shlist;shlist=shlist->pNext){
					if(!strequal(shlist->sh.name,currentshare))
						continue;   
					u=shlist->pUser;
					while(u){
						if(!strcmp(user,u->name)){      	
							*readonly=(u->nAccess==7)?1:0;
							return 0;
						}   
						u=u->pNext;
					}
				}	
				*readonly=0;
				return 0;
			}
			return 0;	           	
		}                 
	} 
	return 1;    
}					


void de_dotdot (char *file)
{
	char *cp;
	char *cp2;
	int l;

	/* Collapse any multiple / sequences. */
	while ((cp = strstr (file, "//")) != (char *) 0) {
		for (cp2 = cp + 2; *cp2 == '/'; ++cp2)
			continue;
		(void) strcpy (cp + 1, cp2);
	}
	/* Remove leading ./ and any /./ sequences. */
	while (strncmp (file, "./", 2) == 0)
		(void) strcpy (file, file + 2);
	while ((cp = strstr (file, "/./")) != (char *) 0)
		(void) strcpy (cp, cp + 2);

	/* Alternate between removing leading ../ and removing xxx/../ */
	for (;;) {
		while (strncmp (file, "../", 3) == 0)
			(void) strcpy (file, file + 3);
		cp = strstr (file, "/../");
		if (cp == (char *) 0)
			break;
		for (cp2 = cp - 1; cp2 >= file && *cp2 != '/'; --cp2)
			continue;
		(void) strcpy (cp2 + 1, cp + 4);
	}

	/* Also elide any xxx/.. at the end. */
	while ((l = strlen (file)) > 3
	       && strcmp ((cp = file + l - 3), "/..") == 0) {
		for (cp2 = cp - 1; cp2 >= file && *cp2 != '/'; --cp2)
			continue;
		if (cp2 < file)
			break;
		*cp2 = '\0';
	}
}

int utf8_support = 0;

#include <iconv.h>

extern iconv_t libiconv_open (const char *tocode, const char *fromcode);
extern size_t libiconv (iconv_t cd, char **inbuf, size_t * inbytesleft, char **outbuf, size_t * outbytesleft);
extern int libiconv_close (iconv_t cd);
extern int code_page;

static size_t do_convert (const char *to_ces, const char *from_ces, char *inbuf, size_t inbytesleft, char *outbuf, size_t outbytesleft)
{
	iconv_t cd = libiconv_open (to_ces, from_ces);
	if (cd <= 0)
		return -1;
	size_t ret = libiconv (cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
	libiconv_close (cd);
	return ret;
}

void to_utf8 (char *file_name)
{
#ifdef DEBUG            
	bftpd_log("utf8_support: %d.\n", utf8_support);
	bftpd_log("code_page: %d.\n", code_page);
#endif 	
	if (utf8_support)
		return;
	if (file_name && strlen (file_name) < MAX_FILE_LENGTH) {
		char codepage_str[8] =
			{ 0 }, converted_text[3 * MAX_FILE_LENGTH + 1] = {
		0};
		int ret = 0;

		if (code_page == 437 || code_page == 850)
			ret = do_convert ("UTF-8", "CP1252", file_name,
					  strlen (file_name), converted_text,
					  sizeof (converted_text));
		else {
			sprintf (codepage_str, "CP%d", code_page);
			ret = do_convert ("UTF-8", codepage_str, file_name,
					  strlen (file_name), converted_text,
					  sizeof (converted_text));
		}
		if (!ret) {
			strcpy (file_name, converted_text);
		}
	}
}

void from_utf8 (char *file_name)
{
#ifdef DEBUG            
	bftpd_log("utf8_support: %d.\n", utf8_support);
	bftpd_log("code_page: %d.\n", code_page);
#endif 	
	if (utf8_support)
		return;
	if (file_name && strlen (file_name) < MAX_FILE_LENGTH) {
		char codepage_str[8] = { 0 }, converted_text[MAX_FILE_LENGTH + 1] = { 0};
		int ret = 0;

		if (code_page == 437 || code_page == 850)
			ret = do_convert ("CP1252", "UTF-8", file_name,
					  strlen (file_name), converted_text,
					  sizeof (converted_text));
		else {
			sprintf (codepage_str, "CP%d", code_page);
			ret = do_convert (codepage_str, "UTF-8", file_name,
					  strlen (file_name), converted_text,
					  sizeof (converted_text));
		}
		if (!ret)
			strcpy (file_name, converted_text);
	}
}


