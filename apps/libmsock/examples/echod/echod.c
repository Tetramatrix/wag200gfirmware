/*
**  htget - echo servers. prints out whatever a client writes.
**
**  Limitations and Comments:
**  an example of msock library.
**
**  Development History:
**      who                  when           why
**      ma_muquit@fccc.edu   Jul-10-1999    first cut
*/

#include <msock.h>

int main (int argc,char **argv) 
{
    int
        connected,
        rc,
        port,
        client_port,
        sock_fd;
    
    char
        szclient_host[64],
        szclient_ip[64],
        szbuf[BUFSIZ],
        szhost[64],
        szpage[1024];

    if (argc < 2)
    {
        (void) fprintf(stderr,"usage: %s port\n",argv[0]);
        return(1);
    }
    port=atoi(argv[1]);

    /* open socket and start listening */
    sock_fd=ServerSocket((u_short) port,5);
    if (sock_fd == -1)
    {
        (void) fprintf(stderr,"Failed to open socket\n");
        return (1);
    }

    /* we will be here only when a client connects */

    /* find out the name/ip of the client */
    rc=getPeerInfo(sock_fd,szclient_host,szclient_ip,&client_port);
    if (rc == 0)
    {
        (void) fprintf(stderr,"================================\n");
        (void) fprintf(stderr,"Client hostname %s\n",szclient_host);
        (void) fprintf(stderr,"Client IP: %s\n",szclient_ip);
        (void) fprintf(stderr,"Client port: %d\n",client_port);
        (void) fprintf(stderr,"================================\n");
    }

    connected=1;
    /* server the client */
    while (connected)
    {
        rc=sockGets(sock_fd,szbuf,sizeof(szbuf));
        if (rc < 0)
            break;
        if (rc)
        {
            if (strcmp(szbuf,"quit") == 0)
            {
                (void) fprintf(stderr,"closing connection with %s ...",
                               szclient_host);
                break;
            }
            (void) fprintf(stdout,"%s\n",szbuf);
            (void) fflush(stdout);
        }
    }
    

    (void) fprintf(stderr," <closed>\n\n");
    close(sock_fd);
    return(0);
}
