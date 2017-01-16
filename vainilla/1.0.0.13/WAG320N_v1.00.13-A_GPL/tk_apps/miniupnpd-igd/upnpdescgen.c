/*******************************************************
 *              WAG200GV2 igd_upnpd.  
 *      This file generate xml.
 *      CopyRight 2007 @ Sercomm By Oliver.Hao.
 *******************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "upnpdescgen.h"
#include "upnpdescgen.h"

char *genServiceDesc_xml(int *len, char *xml_path)
{
	char * str;
	int tmplen = 0;
	struct stat status;
	FILE *fp_xml;
	int in;

	fp_xml = fopen(xml_path,"r");
	if(fp_xml == NULL)
	{
		printf("%s[%d] : open service xml fail\n",__FUNCTION__,__LINE__);
		return NULL;
	}
	stat(xml_path,&status);
	*len = status.st_size;
	str = (char *)malloc(*len+1);
	while((in = fgetc(fp_xml)) != EOF)
		str[tmplen++] = (unsigned char )in;

	str[*len] = 0;
	fclose(fp_xml);
	return str;
}

void gen_root_xml(char *mod_path, char *root_path, struct XMLElt *root_desc)
{
	FILE *fp_mod = fopen(mod_path,"r");
	FILE *fp_root = fopen(root_path,"w");

	if((fp_mod == NULL) || (fp_root == NULL))
	{
		printf("open file error\n");
		return ;
	}

	while(1)
	{
		int i,j;
		unsigned char in,out;
		char name[20];
		char *value = NULL;
		
		if((i = fgetc(fp_mod)) == EOF)
		{
			printf("read root xml mod file OK\n");
			break ;
		}

		in = (unsigned char)i;

		if(in != '@')
		{
			out = in;
			fputc(out,fp_root);
			continue ;
		}

		for(j = 0; (j <= 19) && ((in = fgetc(fp_mod)) != '#'); j++)
			name[j] = in;
		name[j] = 0;

#ifdef DEBUG
		printf("%s[%d] : read %s from root mod file\n",__FUNCTION__,__LINE__,name);
#endif
		j = -1;
		while(root_desc[++j].eltname != NULL)
		{
			if(strcmp(root_desc[j].eltname,name) == 0)
			{
				value = (char *)root_desc[j].data;
				fwrite(value,strlen(value),1,fp_root);
#ifdef DEBUG
		printf("%s[%d] : find %s , value = %s\n",__FUNCTION__,__LINE__,name,value);
#endif	
				break;
			}
		}
	}

	fclose(fp_mod);
	fclose(fp_root);
}

