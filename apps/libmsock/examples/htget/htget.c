/*
**  htget - download a web page
**
**  Limitations and Comments:
**  an example of msock library.
**  usage: hget http://hostname:port/page
**
**    example:  htget http://www.fccc.edu/index.html
**              htget http://www.fccc.edu:8080/index.html
**
**  Development History:
**      who                  when           why
**      ma_muquit@fccc.edu   Jul-10-1999    first cut
*/

#include <msock.h>

/* proto */
static int parseURL(char *url,char *hostname,int *port,char *page);

int main (int argc,char **argv) 
{
    int
        rc,
        port,
        sock_fd;
    
    char
        szbuf[BUFSIZ],
        szhost[64],
        szpage[1024];

    if (argc < 2)
    {
        (void) fprintf(stderr,"usage: %s <URL>\n",argv[0]);
        (void) fprintf(stderr,"\nExample:\n");
        (void) fprintf(stderr," %s http://www.fccc.edu/index.html\n",argv[0]);
        (void) fprintf(stderr," %s http://www.fccc.edu:8080/index.html\n",
                       argv[0]);
        return(1);
    }
    rc=parseURL(argv[1],szhost,&port,szpage);
    if (rc != 0)
    {
        (void) fprintf(stderr,"Invalid URL: %s\n",argv[1]);
        return(1);
    }

    /* open a connection to the web server */
    sock_fd=ClientSocket(szhost,(u_short) port);
    if (sock_fd == -1)
    {
        (void) fprintf(stderr,"Failed to connect to %s\n",szhost);
        return (1);
    }

    /* we connected successfully, send HTTP GET command*/
    (void) sprintf(szbuf,"GET %s HTTP/1.0\r\n",szpage);
    (void) sockPuts(sock_fd,szbuf);
    (void) sockPuts(sock_fd,"User-Agent: hget1.1 by muquit\r\n");
     (void) sockPuts(sock_fd,"\r\n");


    /* cool! now read the result back from the server, and write it to stdout*/

    /* print HTTP Header came from server */
    while (sockGets(sock_fd,szbuf,sizeof(szbuf)-1))
    {
#ifdef DEBUG
        (void) fprintf(stderr,"%s\n",szbuf);
        (void) fflush(stderr);
#endif /* DEBUG */
    }

#ifdef DEBUG
    (void) fprintf(stderr,"\n");
#endif /* DEBUG */


    /* print the page */
    while ((rc=sockRead(sock_fd,szbuf,sizeof(szbuf))))
    {
        write(STDOUT_FILENO,szbuf,rc);
    }
    close(sock_fd);
    return(0);
}


/*
**  parseURL()
**  parse a URL of the form
**          http://host:port/thepage.html
**          http://host/thepage.html
**
**  Parameters:
**  char *url       the url to parse
**  char *hostname  the hostname (returns)
**  int  *port      the port (returns)
**  char *page      the page (returns)
**
**  Return Values:
**  0 on success
**  -1 on failure
**
**  Limitations and Comments:
**  - not much error checking. 
**  - hostname and page must have enough space preallocated by the caller.
**  - lots of possibilities of buffer overflow.
**
**  Development History:
**      who                  when           why
**      ma_muquit@fccc.edu   Jul-10-1999    first cut
*/

int parseURL(char *url,char *hostname,int *port,char *page)
{
    char 
        tmpbuf[32],
        *q,
        *r,
        *p,
        *ptmp;
    
    if (url == NULL || *url == '\0')
        return(-1);

    /* initialize */
    *port=80;
    *tmpbuf='\0';

    ptmp=strdup(url);
    if (!ptmp)
        return (-1);

    if (strlen(ptmp) < 7)
        return (-1);

    /* skip http:// */
    if ( (ptmp[0] == 'h' || ptmp[0] == 'H') &&
         (ptmp[1] == 't' || ptmp[1] == 'T') &&
         (ptmp[2] == 't' || ptmp[2] == 'T') &&
         (ptmp[3] == 'p' || ptmp[3] == 'P') &&
         (ptmp[4] == ':') &&
         (ptmp[5] == '/') &&
         (ptmp[6] == '/'))
    {
        p = ptmp+7;
    }
    else
    {
        return (-1);
    }

    q=hostname;
    r=tmpbuf;
    for (; (*p != '/') && (*p != '\0'); p++)
    {
        /* get host part out */
        if (*p != ':' && !isdigit(*p))
        {
            *q++ = *p;
        }

        /* get port out if any */
        if (isdigit(*p))
        {
            *r++ = *p;
        }
    }
    /* NULL terminate */
    *q='\0';
    *r='\0';
    if (*tmpbuf != '\0')
        *port=atoi(tmpbuf);

    /* the rest is the page */
    if (*p == '\0')
        return (-1);

    if (strcmp(p,"/") == 0)
    {
        (void) strcpy(page,"/index.html");
    }
    else
        (void) strcpy(page,p);

    return (0);
}

