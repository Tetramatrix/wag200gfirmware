--- /home/chi/Software/Wag200G/wag200g_v1.01.06_eu/code/apps/busybox.1.0/loginutils/passwd.c	2006-02-26 10:38:14.000000000 +0100
+++ passwd.c	2007-01-04 00:03:18.000000000 +0100
@@ -17,7 +17,7 @@
 static char crypt_passwd[128];
 
 static int create_backup(const char *backup, FILE * fp);
-static int new_password(const struct passwd *pw, int amroot, int algo);
+static int new_password(const struct passwd *pw, int amroot, int algo, char *cmdline_pass);
 static void set_filesize_limit(int blocks);
 
 
@@ -138,6 +138,7 @@
 	char *np;
 	char *name;
 	char *myname;
+        char *cmdline_password=NULL;
 	int flag;
 	int algo = 1;				/* -a - password algorithm */
 	int lflg = 0;				/* -l - lock account */
@@ -150,7 +151,7 @@
 #endif							/* CONFIG_FEATURE_SHADOWPASSWDS */
 	amroot = (getuid() == 0);
 	openlog("passwd", LOG_PID | LOG_CONS | LOG_NOWAIT, LOG_AUTH);
-	while ((flag = getopt(argc, argv, "a:dlu")) != EOF) {
+	while ((flag = getopt(argc, argv, "a:p:dlu")) != EOF) {
 		switch (flag) {
 		case 'a':
 			algo = get_algo(optarg);
@@ -164,6 +165,9 @@
 		case 'u':
 			uflg++;
 			break;
+                case 'p':
+			cmdline_password = (char *) bb_xstrdup(optarg);
+			break;
 		default:
 			bb_show_usage();
 		}
@@ -207,7 +211,7 @@
 			}
 		}
 		printf("Changing password for %s\n", name);
-		if (new_password(pw, amroot, algo)) {
+		if (new_password(pw, amroot, algo, cmdline_password)) {
 			bb_error_msg_and_die( "The password for %s is unchanged.\n", name);
 		}
 	} else if (lflg) {
@@ -316,7 +320,7 @@
 }
 
 
-static int new_password(const struct passwd *pw, int amroot, int algo)
+static int new_password(const struct passwd *pw, int amroot, int algo,char *cmdline_pass)
 {
 	char *clear;
 	char *cipher;
@@ -324,8 +328,10 @@
 	char orig[200];
 	char pass[200];
 	time_t start, now;
-
-	if (!amroot && crypt_passwd[0]) {
+        
+        if(cmdline_pass==NULL)
+        {
+	 if (!amroot && crypt_passwd[0]) {
 		if (!(clear = bb_askpass(0, "Old password:"))) {
 			/* return -1; */
 			return 1;
@@ -381,6 +387,11 @@
 	}
 	bzero(cp, strlen(cp));
 	bzero(orig, sizeof(orig));
+       } /* end if sizeof(cmdline_pass)<2*/
+ 
+        else safe_strncpy(pass,cmdline_pass,strlen(cmdline_pass)+1);
+     
+        printf("\nNew password before encryption:'%s'\n",pass);
 
 	if (algo == 1) {
 		cp = pw_encrypt(pass, "$1$");
