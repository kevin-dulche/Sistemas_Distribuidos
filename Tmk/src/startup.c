/*****************************************************************************
 *                                                                           *
 *  Copyright (c) 1991-1996						     *
 *  by ParallelTools, L.L.C. (PTOOLS), Houston, Texas			     *
 *                                                                           *
 *  This software is furnished under a license and may be used and copied    *
 *  only in accordance with the terms of such license and with the	     *
 *  inclusion of the above copyright notice.  This software or any other     *
 *  copies thereof may not be provided or otherwise made available to any    *
 *  other person.  No title to or ownership of the software is hereby	     *
 *  transferred.                                                             *
 *									     *
 *  The recipient of this software (RECIPIENT) acknowledges and agrees that  *
 *  the software contains information and trade secrets that are	     *
 *  confidential and proprietary to PTOOLS.  RECIPIENT agrees to take all    *
 *  reasonable steps to safeguard the software, and to prevent its	     *
 *  disclosure.								     * 
 *                                                                           *
 *  The information in this software is subject to change without notice     *
 *  and should not be construed as a commitment by PTOOLS.		     *
 *                                                                           *
 *  This software is furnished AS IS, without warranty of any kind, either   *
 *  express or implied (including, but not limited to, any implied warranty  *
 *  of merchantability or fitness), with regard to the software.  PTOOLS     *
 *  assumes no responsibility for the use or reliability of its software.    *
 *  PTOOLS shall not be liable for any special,	incidental, or		     *
 *  consequential damages, or any damages whatsoever due to causes beyond    *
 *  the reasonable control of PTOOLS, loss of use, data or profits, or from  *
 *  loss or destruction of materials provided to PTOOLS by RECIPIENT.	     *
 *									     *
 *  PTOOLS's liability for damages arising out of or in connection with the  *
 *  use or performance of this software, whether in an action of contract    *
 *  or tort including negligence, shall be limited to the purchase price,    *
 *  or the total amount paid by RECIPIENT, whichever is less.		     *
 *                                                                           *
 *****************************************************************************/

/*
 * $Id: startup.c,v 10.19.1.19.0.6 1999/08/03 04:49:49 alc Exp $
 *
 * Description:    
 *	initialization and cleanup routines
 *
 * External Functions:
 *			Tmk_err,
 *			Tmk_errexit,
 *			Tmk_exit,
 *			Tmk_perrexit,
 *			Tmk_startup
 *
 * External Variables:
 *			Tmk_hostlist
 *			Tmk_proc_id,
 *			Tmk_nprocs
 *
 * Facility:	TreadMarks Distributed Shared Memory System
 * History:
 *	15-Apr-1993	Alan L. Cox	Created
 *	 1-Aug-1993	Alan L. Cox	Added reliable message protocol
 *	 7-Apr-1994	Alan L. Cox	Added X window support for slaves
 *	16-May-1994	Alan L. Cox	Modified Tmk_*err* to use vprintf
 *	 1-Aug-1994	Alan L. Cox	Dynamic port/SAP allocation
 *					 (changes provided by Cristiana Amza)
 *	Version 0.9.1
 *
 *	13-Jan-1995	Alan L. Cox	Additional error reporting
 *					 in start_process
 *	18-Jan-1995	Rob Fowler	Added license expiration code
 *	28-Jan-1995	Alan L. Cox	Modified license expiration code and
 *					 error reporting
 *	Version 0.9.2
 *
 *	18-May-1995	Alan L. Cox	Corrected process creation using rsh
 *	22-May-1995	Alan L. Cox	Added debugger support
 *
 *	Version 0.9.4
 *
 *	15-Jun-1995	Rob Fowler	Added IBM Load Leveler support
 *	 1-Sep-1995	Alan L. Cox	Corrected process limit check under -h
 *
 *	Version 0.9.5
 *
 *	10-Nov-1995	Alan L. Cox	Corrected the change of 1-Sep-1995
 *
 *	Version 0.9.6
 *
 *	20-Jan-1996	Alan L. Cox	Modified (getcwd) to adhere to ANSI C
 *	27-Jan-1996	Alan L. Cox	Replaced sigblock and sigsetmask
 *					 with sigprocmask
 *	Version 0.10
 */
#include "Tmk.h"

#include <net/if.h>

#include <netinet/in.h>

#if   ! defined(DAYS_VALID)
#	define	DAYS_VALID	10*365	/* ~10 years */
#else
#	define	SCRAMBLE(value, byte)	 ((value >> 8) ^ table_[(value ^ byte) & 255])

static	const	unsigned int	table_[256] = {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
	0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
	0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
	0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
	0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
	0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
	0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
	0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
	0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
	0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
	0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
	0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
	0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
	0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
	0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
	0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
	0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
	0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
	0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
	0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};
#endif

#include <stdarg.h>

sigset_t	ALRM_and_IO_mask;

unsigned short	proc_vector_time_[NPROCS];

unsigned short	inverse_time_[NPROCS];

struct	Tmk	tmk_ = { /*proc_id=*/0, /*nprocs=*/0, /*page_size=*/0, /*npages=*/NPAGES };

unsigned	Tmk_spinmask;

int		Tmk_debug;

char		Tmk_hostlist[NPROCS][MAXHOSTNAMELEN];

#if defined(PTHREADS)
pthread_mutex_t	Tmk_monitor_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

/*
 *
 */
void
Tmk_err(
	const char     *format,
	...)
{
	va_list		args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);

	fflush(stderr);
}

/*
 *
 */
void
Tmk_errexit(
	const char     *format,
	...)
{
	va_list 	args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);

	exit(-1);
}

/*
 *
 */
#if defined(__hpux)
int	h_errno;
#endif
#if ! (defined(__bsdi) || defined(__FreeBSD__) || defined(__sgi))
char   *h_errlist[] = {
	"Resolver Error 0 (no error)",
	"Unknown host",				/* 1 HOST_NOT_FOUND */
	"Host name lookup failure",		/* 2 TRY_AGAIN */
	"Unknown server error",			/* 3 NO_RECOVERY */
	"No address associated with name",	/* 4 NO_ADDRESS */
	"Service unavailable",	/* AIX 4.1 */	/* 5 SERVICE_UNAVAILABLE */
	NULL
};

/*
 * h_errno is thread safe under AIX 4.1 and DEC UNIX 3.2C, but not Solaris 2.5
 */
void	herror(const char *string)
{
	fprintf(stderr, "%s: %s\n", string, h_errlist[h_errno]);
}
#endif

/*
 * This function is called if gethostbyname fails.
 */
void
Tmk_herrexit(
	const char     *format,
	...)
{
	va_list 	args;
	char		string[257];

	va_start(args, format);
	vsprintf(string, format, args);
	va_end(args);

	herror(string);

	exit(-1);
}

/*
 *
 */
void
Tmk_perrexit(
	const char     *format,
	...)
{
	va_list 	args;
	char		string[257];

	va_start(args, format);
	vsprintf(string, format, args);
	va_end(args);
	
	perror(string);

	exit(-1);
}

/*
 *
 */
void
Tmk_errno_check(
	const char     *format,
	...)
{
	if (errno == ENOBUFS)
		return;
	else {
		va_list 	args;
		char		string[257];

		va_start(args, format);
		vsprintf(string, format, args);
		va_end(args);
	
		perror(string);

		exit(-1);
	}
}

#if defined(__GNUC__)
/*
 * This function is called by the `assert' macro provided with gcc.
 */
void	__eprintf(string, expression, line, filename)
	const char     *string;
	const char     *expression;
	unsigned	line;
	const char     *filename;
{
	fprintf(stderr, string, expression, line, filename);
	fflush(stderr);
	abort();
}
#endif

/*
 * Points to the string that is bound to the environment variable DEBUGGER.
 */
static  char   *debugger;

/*
 * Points to the string that is bound to the environment variable DISPLAY.
 */
static	char   *display;

#if defined(LoadL)
/*
 * Is this an IBM LoadLeveler job?
 */
static	int	ll_job = 0;

#include <llapi.h>

static	struct JM_JOB_INFO job_request;

#endif

/*
 * Does the user have a .netrc file?
 */
static	int	netrc_found = 0;

#if !defined(MPL)
/*
 * Allow 30 seconds for the remote process to contact the manager.
 */
static	struct	timeval	timeout = { 30, 0 };

/*
 * Code that doesn't work with the ATM network isn't compiled under Ultrix.
 */
static	void	start_process(i, argc, argv)
	int	i;
	int	argc;
	char   *argv[];
{
	char   *ptr;
	char	pathname[MAXPATHLEN];
	char	string[MAXPATHLEN];
	char   *sptr = string;
	char	arguments[MAXPATHLEN];
	int	j;
	int	flag = 0;
	char   *hostname = Tmk_hostlist[i];
	int	fd;
	int	fd2p;
	FILE   *fp2;
	fd_set	fds;
	int	maxfdp1;
	sigset_t
		mask;

	sigio_mutex(SIG_BLOCK, &ALRM_and_IO_mask, &mask);

	if ((ptr = getenv("PWD")) == 0)
		if ((ptr = getcwd(pathname, MAXPATHLEN)) == 0)
			Tmk_errexit("<getwd>start_process: %s\n", ptr);

	sprintf(string, "cd %s ; ", ptr);

	if (display)
		sprintf(string, "%sxterm -display %s -name %s -title %s -e ", string, display, hostname, hostname);

	if (debugger) {

		sprintf(string, "%s%s %s", string, debugger, argv[0]);

		sptr = arguments;
		sptr[0] = 0;
	}
	for (j = 0; j < argc; j++) {

		if (0 == strcmp(argv[j], "--"))
			flag = 1;

		sprintf(sptr, "%s%s ", sptr, argv[j]);
	}
	if (flag)
		sprintf(sptr, "%s-i%d -m%ld", sptr, i, page_array_[0].vadr);
	else
		sprintf(sptr, "%s-- -i%d -m%ld", sptr, i, page_array_[0].vadr);
#if defined(LoadL)
	if (ll_job)
		for (j = 0; j < Tmk_nprocs; j++)
			sprintf(sptr, "%s -h%s", sptr, Tmk_hostlist[j]);
#endif
	for (j = 0; j < i; j++)
		sprintf(sptr, "%s -p%d", sptr, Tmk_port_[j][i]);

	if (debugger)
		printf("Arguments: %s\n", sptr);

	if (Tmk_debug)
		Tmk_err("%s\n", string);
#if defined(LoadL)
	if (ll_job) {

		char	command[MAXPATHLEN];

		sprintf(command, "/bin/sh -c \"%s\"", string);

		if ((fd2p = ll_start_host(hostname, command)) < 0)
			Tmk_ll_errexit(fd2p);

		fp2 = fdopen(fd2p, "r");
	}
	else	/*  Danger!   Dangling "else".  */
#endif
	if (netrc_found) {

		if ((fd = rexec(&hostname, getservbyname("exec", "tcp")->s_port, NULL, NULL, string, &fd2p)) < 0)
			Tmk_errexit("Tmk_startup: process creation failed on %s.\n", hostname);

		fp2 = fdopen(fd2p, "r");
	}
	else {
		char	command[MAXPATHLEN];
#if ! defined(__hpux)
		sprintf(command, "rsh %s -n \"%s\"", hostname, string);
#else
		sprintf(command, "remsh %s -n \"%s\"", hostname, string);
#endif
		if ((fp2 = popen(command, "r")) == NULL)
			Tmk_perrexit("popen");

		fd2p = fileno(fp2);
	}
	FD_ZERO(&fds);

	FD_SET(fd2p, &fds);

	for (maxfdp1 = MAX(fd2p + 1, rep_fd_[i] + 1);;) {

		fd_set	fds_tmp, fds_exc;
#if defined(__linux)
		struct	timeval	timeout_tmp;

		timeout_tmp = timeout;
#endif
		fds_tmp = fds;

		FD_SET(rep_fd_[i], &fds_tmp);

		FD_ZERO(&fds_exc);

		FD_SET(rep_fd_[i], &fds_exc);
#if defined(__linux)
		if ((flag = select(maxfdp1, (fd_set_t)&fds_tmp, NULL, (fd_set_t)&fds_exc, &timeout_tmp)) < 0)
#else
		if ((flag = select(maxfdp1, (fd_set_t)&fds_tmp, NULL, (fd_set_t)&fds_exc, &timeout)) < 0)
#endif
			Tmk_perrexit("select");

		if (flag == 0)
			Tmk_errexit("Tmk_startup: process creation failed on %s.\n", hostname);

		if (FD_ISSET(fd2p, &fds_tmp))
			if (string == fgets(string, sizeof(string), fp2))
				Tmk_err("from %s: %s", hostname, string);
			else
				FD_CLR(fd2p, &fds);
		else {
			assert(FD_ISSET(rep_fd_[i], &fds_tmp) || FD_ISSET(rep_fd_[i], &fds_exc));

			break;
		}
	}
#if  defined(LoadL)
	if (netrc_found && ! ll_job)
#else
	if (netrc_found)
#endif
		close(fd);
#if ! defined(__linux)
	fclose(fp2);
#endif
	sigio_mutex(SIG_SETMASK, &mask, NULL);

	Tmk_err("[ Tmk: %s started ]\n", hostname);
}

/*
 * Obtain the list of network interfaces attached to this host,
 * including their associated IP addresses.  Obtain the IP address of 
 * Tmk_hostlist[Tmk_proc_id] and search for a match among the network
 * interfaces.  Exit if a match isn't found.  Otherwise, return.
 */
static	void	test_hostlist( void )
{
	char		buf[1024];

	struct	ifconf	ifconf;
	struct	ifreq  *ifreq;

	struct hostent *hp;

	int		dummy_s;

	if ((dummy_s = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
		Tmk_perrexit("<socket>Tmk_startup");

	ifconf.ifc_buf = buf;
	ifconf.ifc_len = sizeof(buf);

	if (0 > ioctl(dummy_s, SIOCGIFCONF, (char *)&ifconf))
		Tmk_perrexit("<ioctl>Tmk_startup");

	if (0 > close(dummy_s))
		Tmk_perrexit("<close>Tmk_startup");

	if ((hp = gethostbyname(Tmk_hostlist[Tmk_proc_id])) == NULL)
		Tmk_herrexit("Tmk_startup: %s", Tmk_hostlist[Tmk_proc_id]);

	assert(hp->h_addrtype == AF_INET);

	ifreq = ifconf.ifc_req;

	while (ifreq < (struct ifreq *)(ifconf.ifc_buf + ifconf.ifc_len)) {

		if (ifreq->ifr_addr.sa_family == AF_INET)
			if (0 == memcmp(&((struct sockaddr_in *)&ifreq->ifr_addr)->sin_addr, hp->h_addr, hp->h_length))
				return;

#if defined(_AIX) || defined(__bsdi) || defined(__FreeBSD__)
	        ifreq = (struct ifreq *)((caddr_t) ifreq + sizeof(ifreq->ifr_name) + ifreq->ifr_addr.sa_len);
#else
		ifreq++;
#endif
	}
	Tmk_errexit("Tmk_startup: this host isn't %s.  Please change your host list.\n", Tmk_hostlist[Tmk_proc_id]);
}
#endif /* !defined(MPL) */

/*
 * Used by the exit handler
 */
static	void	int_handler(int signo)
{
}

/*
 * Called by atexit (or on_exit)
 */
#if defined(__sun) && ! defined(__SVR4)
static	void	exit_handler(status, arg)
	int	status;
	caddr_t	arg;
#else
static	void	exit_handler( void )
#endif
{
	if (display && Tmk_proc_id) {

		struct	sigaction sa;
		struct	sigaction sa_old;

		Tmk_err("[ Tmk: to exit type ^C (SIGINT) ]\n");

		sa.sa_flags = 0;
		sa.sa_handler = int_handler;

		sigemptyset(&sa.sa_mask);

		sigaction(SIGINT, &sa, &sa_old);

		sigsuspend(&ALRM_and_IO_mask);

		sigaction(SIGINT, &sa_old, NULL);
	}
}

/*
 *
 */
void
Tmk_startup(
	int		argc,
	char   * const *argv)
{
	char   *cp = 0;
extern	char   *optarg;
extern	int	optind;
	char	filename[MAXPATHLEN];
	time_t	timestamp = TIMESTAMP;
	time_t	past;
	time_t	future;
	struct	timeval	present;
	int	c, i = 0, j = 0;
extern	int	end;
#if  defined(LoadL)
static	char   *ll_bad_option = "Tmk_startup: the \"%s\" option is not allowed under LoadLeveler.\n";
#endif
	FILE   *fp;
#if  defined(PTHREADS)
	pthread_mutex_lock(&Tmk_monitor_lock);
#endif
#if defined(__sun) && ! defined(__SVR4)
	if (on_exit(exit_handler, NULL))
		Tmk_err("Unable to register handler with \"on_exit\"\n");
#else
	if (0 > atexit(exit_handler))
		perror("Unable to register handler with \"atexit\"");
#endif
	while ((c = getopt(argc, argv, "D:df:h:I:i:m:n:P:p:rSstvX:x")) != -1)
		switch (c) {
		case '?':
			exit(-1);
	        case 'D':
			Tmk_err("Warning: \"-D\" is obsolete.\n");
			continue;
		case 'd':
			if ((debugger = getenv("DEBUGGER")) == 0)
				Tmk_errexit("Tmk_startup: DEBUGGER=(null)\n");
			goto get_display;
		case 'f':
			cp = optarg;
			continue;
		case 'h':
			strcpy(Tmk_hostlist[i], optarg); i++;
			continue;
	        case 'I':
			Tmk_err("Warning: \"-I\" is obsolete.\n");
			continue;
		case 'i':
			Tmk_proc_id = atoi(optarg);
			continue;
		case 'm':
			page_array_[0].vadr = (caddr_t) atol(optarg);
			continue;
		case 'n':
			if ((Tmk_nprocs = atoi(optarg)) > NPROCS)
				Tmk_errexit("Tmk_startup: -n%d exceeds the %d-process limit.\n", Tmk_nprocs, NPROCS);
			else if (Tmk_nprocs == 0)
				Tmk_errexit("Tmk_startup: \"%s\" is an invalid argument for -n.\n", optarg);
			continue;
		case 'P':
			if ((Tmk_npages = atoi(optarg)) > NPAGES)
				Tmk_errexit("Tmk_startup: -P%d exceeds the %d-page limit.\n", Tmk_npages, NPAGES);
			continue;
		case 'p':
			Tmk_port_[j][Tmk_proc_id] = atoi(optarg); j++;
			continue;
		case 'r':
			netrc_found = 1;
			continue;
		case 'S':
			tmk_stat_flag = 2;
			continue;
		case 's':
			tmk_stat_flag = 1;
			continue;
		case 't':
			Tmk_debug = 1;
			continue;
		case 'v':
			Tmk_page_init_to_valid = 1;
			continue;
		case 'x':
		get_display:
			if ((display = getenv("DISPLAY")) == 0)
				Tmk_errexit("Tmk_startup: DISPLAY=(null)\n");
			continue;
		case 'X':
			if ((page_shift = atoi(optarg)) > 3)
				Tmk_errexit("Tmk_startup: -X%d exceeds the maximum page size.\n", page_shift);
			continue;
		}

	if (optind < argc)
		Tmk_errexit("Tmk_startup: \"%s\" is an invalid argument.\n", argv[optind]);
#if defined(LoadL)
	if (Tmk_proc_id == 0 && getenv("LOADL_ACTIVE")) {

		if (debugger)
			Tmk_errexit(ll_bad_option, "-d");

		if (display)
			Tmk_errexit(ll_bad_option, "-x");

		if ((c = ll_get_hostlist(&job_request)) < 0)
			Tmk_ll_errexit(c);

		if (Tmk_nprocs)
			Tmk_errexit(ll_bad_option, "-n");

		Tmk_nprocs = job_request.jm_min_num_nodes;

		if (Tmk_nprocs > NPROCS) 
			Tmk_errexit("Tmk_startup: the LoadLeveler allocation, %d, exceeds the %d-process limit.\n", Tmk_nprocs, NPROCS);
	    
		if (i)
			Tmk_errexit(ll_bad_option, "-h");

		for ( ; i < Tmk_nprocs; i++) {

			struct in_addr	addr;
			struct hostent *host;

			addr.s_addr = inet_addr(job_request.jm_min_node_info[i].jm_node_address);

			if ((host = gethostbyaddr((char *)&addr, sizeof(addr), AF_INET)) == 0)
				Tmk_perrexit("Tmk_startup: gethostbyaddr");

			strcpy(Tmk_hostlist[i], host->h_name);
		}
		ll_job = 1;
	}
#elif defined(MPL)
	if (i > 0)
		Tmk_errexit("Tmk_startup: the \"-h\" option is not allowed under MPL.\n");

	if (0 > mpc_environ(&i, (int *)&Tmk_proc_id))
		Tmk_MPLerrexit("<mpc_environ>Tmk_startup");
#endif
	if (i == 0) {

		/*
		 * Neither LoadLeveler nor "-h" were used.  (Under LoadLeveler slaves
		 * always receive the "-h" option from the master.)
		 */
		if (cp == 0)
			sprintf(cp = filename, "%s/.Tmkrc", getenv("HOME"));

		if ((fp = fopen(cp, "r")) == 0)
			Tmk_perrexit(cp);

		if (0 < Tmk_nprocs)
			j = Tmk_nprocs;
		else
			j = NPROCS + 1;

		for (i = 0; i < j; i++)
			switch (fscanf(fp, "%s\n", Tmk_hostlist[i])) {
			case 0:
				Tmk_errexit("Tmk_startup: %s is improperly formatted.\n", cp);
			case EOF:
				goto escape;
			default:
				continue;
			}
	escape:
		fclose(fp);
	}
	else {
		if (cp)
#if defined(LoadL)
			if (ll_job)
				Tmk_errexit(ll_bad_option, "-f");
			else	/*  Danger!   Dangling "else".  */
#endif
			Tmk_errexit("Tmk_startup: the \"-f\" and \"-h\" options are incompatible\n");

		cp = "command line";
	}
	if (0 > gettimeofday(&present, NULL))
		Tmk_perrexit("Tmk_startup<gettimeofday>");

	if (i > NPROCS)
		Tmk_errexit("Tmk_startup: %s exceeds the %d-process limit.\n", cp, NPROCS);
	else if (i < Tmk_nprocs)
		Tmk_errexit("Tmk_startup: %s contains less than %d host(s).\n", cp, Tmk_nprocs);
	else if (Tmk_nprocs == 0)
		Tmk_nprocs = i;
	else if (i > Tmk_nprocs) {

		Tmk_err("Tmk_startup: %s contains %d host(s), using the first %d.\n", cp, i, Tmk_nprocs);

		i = Tmk_nprocs;
	}
#if ! defined(MPL)
	test_hostlist();
#endif
#if	defined(DAYS_VALID) && DAYS_VALID < 61
	if ((cp = getenv("TMK_DEMO_KEY")) != NULL) {

		unsigned int	key_0_[4],
				key_1_[4];

		unsigned int	value_0,
				value_1;

		sscanf(cp, "%d.%d.%d.%d.%d.%d.%d.%d",
		       &key_0_[0], &key_0_[1], &key_0_[2], &key_0_[3],
		       &key_1_[0], &key_1_[1], &key_1_[2], &key_1_[3]);

		value_0 =
		value_1 = ~0;

		for (j = 0; j < sizeof(key_0_)/sizeof(key_0_[0]); j++) {
			value_0 = SCRAMBLE(value_0, key_0_[j]);
			value_1 = SCRAMBLE(value_1, key_1_[j]);
		}

		past   = value_0;
		future = value_1;

		if ((past + DAYS_VALID*24*3600 != future) || (past > future))
			Tmk_errexit("Tmk_startup: TMK_DEMO_KEY=\"%s\" is invalid.\n", cp);
	}
	else
		Tmk_errexit("Tmk_startup: TMK_DEMO_KEY=(null)\n");
#else
	past   = TIMESTAMP;
	future = TIMESTAMP + DAYS_VALID*24*3600;
#endif
	if ((past > present.tv_sec) ||
	    (present.tv_sec >= future))
		Tmk_nprocs = 1;

	/*
	 * This abomination is necessary to handle Tmk_nprocs == 32.
	 */
	Tmk_spinmask = ((1 << (Tmk_nprocs - 1)) << 1) - 1;

	sigemptyset(&ALRM_and_IO_mask);
	sigaddset(&ALRM_and_IO_mask, SIGALRM);
	sigaddset(&ALRM_and_IO_mask, SIGIO);

	Tmk_page_initialize();

	Tmk_interval_initialize();

	Tmk_diff_initialize();

	Tmk_repo_initialize();

	Tmk_segv_initialize();

	Tmk_distribute_initialize();

	Tmk_barrier_initialize();

	Tmk_lock_initialize();

	Tmk_sched_initialize();
#if !defined(MPL)
	Tmk_tout_initialize();
#endif
	Tmk_sigio_initialize();
#if !defined(MPL)
	if (Tmk_proc_id) {

		if (display == 0) {

			close(0);
			close(1);

			open("/dev/null", O_RDONLY, 0);
			open("/dev/null", O_WRONLY, 0);

			dup2(1, 2);
		}
	}
	else if (netrc_found == 0) {

		int	fd;

		sprintf(filename, "%s/.netrc", getenv("HOME"));

		if ((fd = open(filename, O_RDONLY, 0)) != -1) {

			close(fd);

			netrc_found = 1;
		}
	}
#endif
	Tmk_err("TreadMarks Version 1.0.3.3-BETA of %s"
#if	defined(DAYS_VALID) && DAYS_VALID < 61
		"Copyright (c) ParallelTools, L.L.C., 1991-1998\n"
		"\n"
		"\tUSE OF THIS DEMONSTRATION COPY OF TREADMARKS IS SUBJECT TO THE TERMS\n"
		"\tAND CONDITIONS OF THE LICENSE AGREEMENT INCLUDED WITH THIS SOFTWARE.\n"
		"\n",
#else
		"Copyright (c) ParallelTools, L.L.C., 1991-1998\n",
#endif
		ctime(&timestamp));

	if (i != Tmk_nprocs) {

		Tmk_err("\007\n"
			"\tYour license has expired.  This program will execute sequentially.\n"
			"\n"
			"\tPlease contact ParallelTools, L.L.C.\n"
			"\n");

		sleep(5);
	}
	for (i = 0; i < Tmk_nprocs; i++) {
		if (i == Tmk_proc_id)
			continue;

		Tmk_accept_initialize(i);
	}
	Tmk_connect_initialize();

#if ! defined(MPL)
	for (i = 0; i < Tmk_proc_id; i++) {

		Tmk_connect(i);

		Tmk_accept(i);
	}
	for (i += 1; i < Tmk_nprocs; i++) {

		if (Tmk_proc_id == 0)
			start_process(i, argc, argv);

		Tmk_accept(i);

		Tmk_connect(i);
	}
#endif
#if defined(PTHREADS)
	pthread_mutex_unlock(&Tmk_monitor_lock);
#endif	
}
