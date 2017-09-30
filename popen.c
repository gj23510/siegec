#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#define MAXLINE 1024
 	 
 	int main()
 	{
 	    char result_buf[MAXLINE], command[MAXLINE];
 	    int rc = 0;
 	    FILE *fp;
 	 
 	  //  snprintf(command, sizeof(command), "ls ./ | wc -l");
 	    snprintf(command, sizeof(command), "./siegec demo1.siege");

 	    fp = popen(command, "r");
 	    if(NULL == fp)
 	    {
 	        perror("popen failed！");
 	        exit(1);
 	    }
 	    while(fgets(result_buf, sizeof(result_buf), fp) != NULL)
 	    {	        
 	     if('\n' == result_buf[strlen(result_buf)-1])
 	        {
 	            result_buf[strlen(result_buf)-1] = '\0';
 	        }
 	        printf("command【%s】 output【%s】\r\n", command, result_buf);
 	    }
 	 
 	    rc = pclose(fp);
 	    if(-1 == rc)
 	    {
 	        perror("closed failed to fp");
 	        exit(1);
 	    }
 	    else
 	    {
 	        printf("command【%s】thild process 【%d】return【%d】\r\n", command, rc, WEXITSTATUS(rc));
 	    }
 	 
 	    return 0;
 	}
