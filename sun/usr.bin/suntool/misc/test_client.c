#ifndef lint
static	char sccsid[] = "@(#)test_client.c 1.3 87/05/04";
#endif

#include <errno.h>
#include "selection_impl.h"
#include <selection_attributes.h>
#include <sys/param.h>

extern int            errno;

static int            running;   /* done's flag, to kick out of select loop */

static int            sock_fd,
		      sock_mask;	/* fake notifier data	 */
static Notify_value (*sock_reader) ();
static char          *sock_client;

static void           answer_function(),
		      answer_request();

static Seln_result    answer_yield();

static void           process_keyboard(),
		      do_acquire(),
		      do_yield(),
		      do_clear(),
		      do_function(),
		      do_inquire(),
		      do_request(),
		      do_quit(),
		      do_dump(),
		      do_halt(),
		      do_help(),
		      print_reply();


static char         *seln_handle;

#ifdef STANDALONE
main(argc, argv)
#else
test_client_main(argc, argv)
#endif STANDALONE
    int              argc;
    char           **argv;
{
    int              inmask;

    if ((seln_handle =
	    seln_create(answer_function, answer_request,
    			answer_yield, (char *) NULL)) == (char *) NULL) {
	printf("Couldn't initialize holder\n");
	exit(1);
    } else {
	printf("Holder initialized\n");
    }
    running = TRUE;
    while (running) {
	inmask = (1 << fileno(stdin)) | sock_mask;
	select(NOFILE, &inmask, 0, 0, 0);
	if (errno == EINTR) {
	    continue;
	}
	if (inmask & sock_mask) {
	    (void) sock_reader(sock_client, sock_fd);
	}
	if (inmask & (1 << fileno(stdin))) {
	    process_keyboard();
	}
    }
    seln_destroy(seln_handle);
    exit(0);
}

/*
 *	Fake a notifier interface
 */
Notify_func
notify_set_input_func(handle, func, sock)
    char       *handle;
    Notify_func	func;
    int         sock;
{
    sock_client = handle;
    sock_reader = func;
    sock_fd = sock;
    sock_mask |= 1 << sock;
    return (Notify_func) NULL;
}

static void
process_keyboard()
{
    char    buffer[256];

    gets(buffer);
    switch (*buffer) {
      case 'a':
      case 'A':
	do_acquire(buffer + 1);
	break;
      case 'c':
      case 'C':
	do_clear();
	break;
      case 'f':
      case 'F':
	do_function(buffer + 1);
	break;
      case 'i':
      case 'I':
	do_inquire(buffer + 1);
	break;
      case 'r':
      case 'R':
	do_request(buffer + 1);
	break;
      case 'q':
      case 'Q':
	do_quit();
	break;
      case 'y':
      case 'Y':
	do_yield(buffer + 1);
	break;
      case 'd':
      case 'D':
	do_dump();
	break;
      case 'H':
	do_halt();
	break;
      default:
	do_help();
    }
}

static void
do_acquire(line)
    char        *line;
{
    Seln_rank    asked, given;

    asked = (Seln_rank) (*line - '0');
    if (ord(asked) < ord(SELN_CARET) || ord(asked) > ord(SELN_SHELF)) {
	asked = SELN_UNSPECIFIED;
    }
    given = seln_acquire(seln_handle, asked);
    printf("Acquire selection # %d got selection # %d\n",
	   asked, given);
}

static void
do_clear()
{
   seln_clear_functions();
}

static void
do_function(line)
    char          *line;
{
    Seln_function  func;
    int            state;
    Seln_result    result;

    if (*line >= '0' && *line <= '9') {
	func = (Seln_function)(*line++ - '0');
    } else {
	printf("Which function key changed? (0 - 9)\n");
	return;
    }
    switch (*line) {
      case 'D':
      case 'd':
      case '1':
	state = 1;
	break;

      case 'U':
      case 'u':
      case '0':
	state = 0;
	break;

      default:
	printf("Which way did function key change? (1 / 0 or d / u)\n");
	return;
    }
    result = seln_inform(func, state);
    printf("Inform service that function key %d went %s gave result = %d\n",
	   func, (state ? "down" : "up"), result);

}

static void
do_inquire(line)
    char          *line;
{
    Seln_holder    holder;
    Seln_rank      rank;

    rank = (Seln_rank) (*line - '0');
    if (ord(rank) < ord(SELN_CARET) || ord(rank) > ord(SELN_SHELF)) {
	rank = SELN_UNSPECIFIED;
    }    printf("Inquire selection %d ", rank);
    holder = seln_inquire(rank);
    printf("gave result = %X\n", holder);
}

static void
do_request(line)
    char               *line;
{
    unsigned            flags[SELN_BUFSIZE / (sizeof (unsigned))];
    unsigned           *flag_ptr = flags;
    register char       c;
    register int        i = 0;
    Seln_holder         holder;
    Seln_rank           rank = SELN_PRIMARY;
    Seln_request       *result;

    while ((c = *line++) != '\0') {
	switch (c) {
	  case '1':
	    rank = SELN_CARET;
	    i--;
	    continue;
	  case '2':
	    rank = SELN_PRIMARY;
	    i--;
	    continue;
	  case '3':
	    rank = SELN_SECONDARY;
	    i--;
	    continue;
	  case '4':
	    rank = SELN_SHELF;
	    i--;
	    continue;
	  case 'l':
	  case 'L':
	    *flag_ptr++ = (unsigned)(SELN_REQ_LINE_POS);
	    *flag_ptr++;
	    break;
	  case 'c':
	  case 'C':
	    *flag_ptr++ = (unsigned)(SELN_REQ_CONTENTS_ASCII);
	    *flag_ptr++ = 0;
	    break;
	  case 'd':
	  case 'D':
	    *flag_ptr++ = (unsigned)(SELN_REQ_DESELECT);
	    *flag_ptr++;
	    break;
	  case 'p':
	  case 'P':
	    *flag_ptr++ = (unsigned)(SELN_REQ_COMMIT_PEND_DELETE);
	    *flag_ptr++;
	    break;
	  case 's':
	  case 'S':
	    *flag_ptr++ = (unsigned)(SELN_REQ_BYTESIZE);
	    *flag_ptr++;
	    break;
	  default:
	    printf("Unknown request flag '%c' ignored.\n", c);
	    break;
	}
	if (++i > 3) {
	    printf("Too many request flags\n");
	    return;
	}
    }
    holder = seln_inquire(rank);
    switch (i) {
      case 0:
	printf("No request to send\n");
	return;
      case 1:
	result = seln_ask(&holder, flags[0], flags[1], 0);
	break;
      case 2:
	result = seln_ask(&holder, flags[0], flags[1],
					     flags[2], flags[3], 0);
	break;
      case 3:
	result = seln_ask(&holder, rank, flags[0], flags[1],
					     flags[2], flags[3],
					     flags[4], flags[5], 0);
	break;
      case 4:
	result = seln_ask(&holder, rank, flags[0], flags[1],
					     flags[2], flags[3],
					     flags[4], flags[5],
					     flags[6], flags[7], 0);
	break;
      case 5:
	result = seln_ask(&holder, rank, flags[0], flags[1],
					     flags[2], flags[3],
					     flags[4], flags[5],
					     flags[6], flags[7],
					     flags[8], flags[9], 0);
	break;
      case 6:
	result = seln_ask(&holder, rank, flags[0], flags[1],
					     flags[2], flags[3],
					     flags[4], flags[5],
					     flags[6], flags[7],
					     flags[8], flags[9],
					     flags[10], flags[11], 0);
	break;
    }
    printf("Request returned %x:\n", result);
    if (result != NULL) {
	print_reply(result);
    }
}

static void
do_quit()
{
    running = FALSE;
}

static void
do_yield(line)
    char       *line;
{
    Seln_rank	rank;
    Seln_result result;

    rank = (Seln_rank) (*line - '0');
    if (ord(rank) < ord(SELN_CARET) || ord(rank) > ord(SELN_SHELF)) {
	printf("Done with which selection (1 - 4)?\n");
	return;
    }
    result = seln_done(seln_handle, rank);
    printf("Yield selection %d gave result = %d\n", rank, result);
}

static void
do_dump()
{
#ifdef TESTING
    seln_dump(SELN_UNSPECIFIED);
#else
    printf("Sorry, no dump available (recompile with -DTESTING to get it)>\n");
#endif
}

static void
do_halt()
{
    printf("Halt Service Commands aren't supported yet.\n");
}

static void
do_help()
{
    printf("An\tAcquire: become holder of selection n\n");
    printf("C\tClear: set all function keys up\n");
    printf("Fnd\tFunction: Key n went down ('d') or up('u')\n");
    printf("In\tInquire: find the holder of selection n\n");
    printf("Rn\tRequest: send a request to the holder of selection n\n");
    printf("Q\tQuit: exit the tester\n");
    printf("Yn\tYield: release selection n\n");
    printf("Dn\tDump: ask the service to dump its state\n");
    printf("H\tHalt: ask the service to halt\n");
    printf("\nLower case is accepted, except for the Halt command\n");
    printf("A selection number may be omitted, to indicate Unspecified.\n");
}


static void
answer_function(client_data, args)
/* ARGSUSED */
    char               *client_data;
{
    printf("Client callback function called with op Do_function\n");
}

static void
answer_request(client_data, args)
    /* ARGSUSED */
    char               *client_data;
    Seln_request       *args;
{
    unsigned            requests[SELN_BUFSIZE / (sizeof (unsigned))];
    unsigned           *destp, *srcp, item;
    int                 size;
    char               *seln = "This sentence\nstarts on line 69.";

    printf("Client callback function called with op Request\n");
    bcopy(args->data, (char *) requests, (int) args->buf_size);
    srcp = requests;
    destp = (unsigned *) args->data;
    size = 0;
    do {
	switch (item = *srcp++) {
	  case (unsigned)(SELN_REQ_CONTENTS_ASCII):
	    *destp++ = item;
	    bcopy(seln, (char *) destp, strlen(seln));
	    destp += (strlen(seln) + sizeof (unsigned) -1) / sizeof (unsigned);
	    size += strlen(seln) + 2 * sizeof (unsigned) -1;
	    size -= size % sizeof (unsigned);
	    break;
	  case (unsigned)(SELN_REQ_LINE_POS):
	    *destp++ = item;
	    *destp++ = 69;
	    size += 2 * sizeof (unsigned);
	    break;
	  case (unsigned)(SELN_REQ_BYTESIZE):
	    *destp++ = item;
	    *destp++ = strlen(seln);
	    size += 2 * sizeof (unsigned);
	    break;
	  case 0:
	    *destp++ = item;
	    break;
	  default:
	    *destp++ = (unsigned)(SELN_REQ_UNKNOWN);
	    *destp++ = item;
	    size += 2 * sizeof (unsigned);
	    break;
	}
	srcp++;		/*  skip the placeholder  */
    } while (item != 0);
    args->buf_size = size;
}

static Seln_result
answer_yield(client_data, args)
/* ARGSUSED */
    char               *client_data;
{
    return SELN_SUCCESS;
}

static void
print_reply(buffer)
    Seln_request       *buffer;
{
    unsigned           *srcp, item;
    int                 size;
    char                string_buf[2048];

    srcp = (unsigned *) buffer->data;
    size = 0;
    while (item = *srcp++) {
	switch (item) {
	  case SELN_REQ_LINE_POS:
	    printf("Line # %d\n", *srcp++);
	    break;
	  case SELN_REQ_CONTENTS_ASCII:
	    if (size == 0) {
		size = strlen((char *) srcp) + sizeof (unsigned) -1;
		size -= size % sizeof (unsigned);
	    }
	    if (size >= 2048)
		size = 2047;
	    bcopy((char *) srcp, string_buf, size);
	    string_buf[size] = '\0';
	    printf("Contents: \"%s\"\n", string_buf);
	    srcp += size;
	    break;
	  case SELN_REQ_BYTESIZE:
	    size = *srcp++;
	    printf("Byte size = %d\n", size);
	    break;
	  case SELN_REQ_UNKNOWN:
	    printf("Request %d rejected: unrecognized\n", *srcp++);
	    break;
	}
    }

}
