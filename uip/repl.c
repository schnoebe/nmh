
/*
 * repl.c -- reply to a message
 *
 * This code is Copyright (c) 2002, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information.
 */

#include <h/mh.h>
#include <h/mime.h>
#include <h/utils.h>

#define REPL_SWITCHES \
    X("group", 0, GROUPSW) \
    X("nogroup", 0, NGROUPSW) \
    X("annotate", 0, ANNOSW) \
    X("noannotate", 0, NANNOSW) \
    X("cc all|to|cc|me", 0, CCSW) \
    X("nocc all|to|cc|me", 0, NCCSW) \
    X("draftfolder +folder", 0, DFOLDSW) \
    X("draftmessage msg", 0, DMSGSW) \
    X("nodraftfolder", 0, NDFLDSW) \
    X("editor editor", 0, EDITRSW) \
    X("noedit", 0, NEDITSW) \
    X("convertargs type argstring", 0, CONVERTARGSW) \
    X("fcc folder", 0, FCCSW) \
    X("filter filterfile", 0, FILTSW) \
    X("form formfile", 0, FORMSW) \
    X("format", 5, FRMTSW) \
    X("noformat", 7, NFRMTSW) \
    X("inplace", 0, INPLSW) \
    X("noinplace", 0, NINPLSW) \
    X("mime", 0, MIMESW) \
    X("nomime", 0, NMIMESW) \
    X("query", 0, QURYSW) \
    X("noquery", 0, NQURYSW) \
    X("whatnowproc program", 0, WHATSW) \
    X("nowhatnowproc", 0, NWHATSW) \
    X("width columns", 0, WIDTHSW) \
    X("version", 0, VERSIONSW) \
    X("help", 0, HELPSW) \
    X("file file", 4, FILESW) /* interface from msh */ \
    X("build", 5, BILDSW) /* interface from mhe */ \
    X("atfile", 0, ATFILESW) \
    X("noatfile", 0, NOATFILESW) \
    X("fmtproc program", 0, FMTPROCSW) \
    X("nofmtproc", 0, NFMTPROCSW) \

#define X(sw, minchars, id) id,
DEFINE_SWITCH_ENUM(REPL);
#undef X

#define X(sw, minchars, id) { sw, minchars, id },
DEFINE_SWITCH_ARRAY(REPL, switches);
#undef X

#define CC_SWITCHES \
    X("to", 0, CTOSW) \
    X("cc", 0, CCCSW) \
    X("me", 0, CMESW) \
    X("all", 0, CALSW) \

#define X(sw, minchars, id) id,
DEFINE_SWITCH_ENUM(CC);
#undef X

#define X(sw, minchars, id) { sw, minchars, id },
DEFINE_SWITCH_ARRAY(CC, ccswitches);
#undef X

#define DISPO_SWITCHES \
    X("quit", 0, NOSW) \
    X("replace", 0, YESW) \
    X("list", 0, LISTDSW) \
    X("refile +folder", 0, REFILSW) \
    X("new", 0, NEWSW) \

#define X(sw, minchars, id) id,
DEFINE_SWITCH_ENUM(DISPO);
#undef X

#define X(sw, minchars, id) { sw, minchars, id },
DEFINE_SWITCH_ARRAY(DISPO, aqrnl);
#undef X

static struct swit aqrl[] = {
    { "quit", 0, NOSW },
    { "replace", 0, YESW },
    { "list", 0, LISTDSW },
    { "refile +folder", 0, REFILSW },
    { NULL, 0, 0 }
};

short ccto = -1;		/* global for replsbr */
short cccc = -1;
short ccme = -1;
short querysw = 0;

static short outputlinelen = OUTPUTLINELEN;
static short groupreply = 0;		/* Is this a group reply?        */

static int mime = 0;			/* include original as MIME part */
static char *form   = NULL;		/* form (components) file        */
static char *filter = NULL;		/* message filter file           */
static char *fcc    = NULL;		/* folders to add to Fcc: header */


/*
 * prototypes
 */
static void docc (char *, int);
static void add_convert_header (const char *, char *, char *, char *);


int
main (int argc, char **argv)
{
    int	i, isdf = 0;
    int anot = 0, inplace = 1;
    int nedit = 0, nwhat = 0;
    int atfile = 0;
    int fmtproc = -1;
    char *cp, *cwd, *dp, *maildir, *file = NULL;
    char *folder = NULL, *msg = NULL, *dfolder = NULL;
    char *dmsg = NULL, *ed = NULL, drft[BUFSIZ], buf[BUFSIZ];
    char **argp, **arguments;
    svector_t convert_types = svector_create (10);
    svector_t convert_args = svector_create (10);
    size_t n;
    struct msgs *mp = NULL;
    struct stat st;
    FILE *in;

    int buildsw = 0;

    if (nmh_init(argv[0], 1)) { return 1; }

    arguments = getarguments (invo_name, argc, argv, 1);
    argp = arguments;

    while ((cp = *argp++)) {
	if (*cp == '-') {
	    switch (smatch (++cp, switches)) {
		case AMBIGSW: 
		    ambigsw (cp, switches);
		    done (1);
		case UNKWNSW: 
		    adios (NULL, "-%s unknown", cp);

		case HELPSW: 
		    snprintf (buf, sizeof(buf), "%s: [+folder] [msg] [switches]",
			invo_name);
		    print_help (buf, switches, 1);
		    done (0);
		case VERSIONSW:
		    print_version(invo_name);
		    done (0);

		case GROUPSW:
		    groupreply++;
		    continue;
		case NGROUPSW:
		    groupreply = 0;
		    continue;

		case ANNOSW: 
		    anot++;
		    continue;
		case NANNOSW: 
		    anot = 0;
		    continue;

		case CCSW: 
		    if (!(cp = *argp++) || *cp == '-')
			adios (NULL, "missing argument to %s", argp[-2]);
		    docc (cp, 1);
		    continue;
		case NCCSW: 
		    if (!(cp = *argp++) || *cp == '-')
			adios (NULL, "missing argument to %s", argp[-2]);
		    docc (cp, 0);
		    continue;

		case EDITRSW: 
		    if (!(ed = *argp++) || *ed == '-')
			adios (NULL, "missing argument to %s", argp[-2]);
		    nedit = 0;
		    continue;
		case NEDITSW:
		    nedit++;
		    continue;
		    
		case CONVERTARGSW: {
                    char *type;
                    size_t i;

		    if (!(type = *argp++)) {
			adios (NULL, "missing type argument to %s", argp[-2]);
                    }
		    if (!(cp = *argp++)) {
			adios (NULL, "missing argstring argument to %s",
			       argp[-3]);
                    }

                    for (i = 0; i < svector_size (convert_types); ++i) {
                        if (! strcmp (svector_at (convert_types, i), type)) {
                            /* Already saw this type, so just update
                               its args. */
                            svector_strs (convert_args)[i] = cp;
                            break;
                        }
                    }

                    if (i == svector_size (convert_types)) {
                        svector_push_back (convert_types, type);
                        svector_push_back (convert_args, cp);
                    }
		    continue;
                }

		case WHATSW: 
		    if (!(whatnowproc = *argp++) || *whatnowproc == '-')
			adios (NULL, "missing argument to %s", argp[-2]);
		    nwhat = 0;
		    continue;
		case BILDSW: 
		    buildsw++;	/* fall... */
		case NWHATSW: 
		    nwhat++;
		    continue;

		case FCCSW: 
		    if (!(cp = *argp++) || *cp == '-')
			adios (NULL, "missing argument to %s", argp[-2]);
		    dp = NULL;
		    if (*cp == '@')
			cp = dp = path (cp + 1, TSUBCWF);
		    if (fcc)
			fcc = add (", ", fcc);
		    fcc = add (cp, fcc);
		    if (dp)
			free (dp);
		    continue;

		case FILESW: 
		    if (file)
			adios (NULL, "only one file at a time!");
		    if (!(cp = *argp++) || *cp == '-')
			adios (NULL, "missing argument to %s", argp[-2]);
		    file = path (cp, TFILE);
		    continue;
		case FILTSW:
		    if (!(cp = *argp++) || *cp == '-')
			adios (NULL, "missing argument to %s", argp[-2]);
		    filter = getcpy (etcpath (cp));
		    mime = 0;
		    continue;
		case FORMSW: 
		    if (!(form = *argp++) || *form == '-')
			adios (NULL, "missing argument to %s", argp[-2]);
		    continue;

		case FRMTSW: 
		    filter = getcpy (etcpath (mhlreply));
		    mime = 0;
		    continue;
		case NFRMTSW: 
		    filter = NULL;
		    continue;

		case INPLSW: 
		    inplace++;
		    continue;
		case NINPLSW: 
		    inplace = 0;
		    continue;

		case MIMESW:
		    mime++;
		    filter = NULL;
		    continue;
		case NMIMESW:
		    mime = 0;
		    continue;

		case QURYSW: 
		    querysw++;
		    continue;
		case NQURYSW: 
		    querysw = 0;
		    continue;

		case WIDTHSW: 
		    if (!(cp = *argp++) || *cp == '-')
			adios (NULL, "missing argument to %s", argp[-2]);
		    if ((outputlinelen = atoi (cp)) < 10)
			adios (NULL, "impossible width %d", outputlinelen);
		    continue;

		case DFOLDSW: 
		    if (dfolder)
			adios (NULL, "only one draft folder at a time!");
		    if (!(cp = *argp++) || *cp == '-')
			adios (NULL, "missing argument to %s", argp[-2]);
		    dfolder = path (*cp == '+' || *cp == '@' ? cp + 1 : cp,
				    *cp != '@' ? TFOLDER : TSUBCWF);
		    continue;
		case DMSGSW:
		    if (dmsg)
			adios (NULL, "only one draft message at a time!");
		    if (!(dmsg = *argp++) || *dmsg == '-')
			adios (NULL, "missing argument to %s", argp[-2]);
		    continue;
		case NDFLDSW: 
		    dfolder = NULL;
		    isdf = NOTOK;
		    continue;

		case ATFILESW:
		    atfile++;
		    continue;
		case NOATFILESW:
		    atfile = 0;
		    continue;

		case FMTPROCSW:
		    if (!(formatproc = *argp++) || *formatproc == '-')
			adios (NULL, "missing argument to %s", argp[-2]);
		    fmtproc = 1;
		    continue;
		case NFMTPROCSW:
		    fmtproc = 0;
		    continue;
	    }
	}
	if (*cp == '+' || *cp == '@') {
	    if (folder)
		adios (NULL, "only one folder at a time!");
	    else
		folder = pluspath (cp);
	} else {
	    if (msg)
		adios (NULL, "only one message at a time!");
	    else
		msg = cp;
	}
    }

    if (ccto == -1) 
	ccto = groupreply;
    if (cccc == -1)
	cccc = groupreply;
    if (ccme == -1)
	ccme = groupreply;

    cwd = getcpy (pwd ());

    if (!context_find ("path"))
	free (path ("./", TFOLDER));
    if (file && (msg || folder))
	adios (NULL, "can't mix files and folders/msgs");

try_it_again:

    strncpy (drft, buildsw ? m_maildir ("reply")
			  : m_draft (dfolder, NULL, NOUSE, &isdf), sizeof(drft));

    /* Check if a draft exists */
    if (!buildsw && stat (drft, &st) != NOTOK) {
	printf ("Draft \"%s\" exists (%ld bytes).", drft, (long) st.st_size);
	for (i = LISTDSW; i != YESW;) {
	    if (!(argp = getans ("\nDisposition? ", isdf ? aqrnl : aqrl)))
		done (1);
	    switch (i = smatch (*argp, isdf ? aqrnl : aqrl)) {
		case NOSW: 
		    done (0);
		case NEWSW: 
		    dmsg = NULL;
		    goto try_it_again;
		case YESW: 
		    break;
		case LISTDSW: 
		    showfile (++argp, drft);
		    break;
		case REFILSW: 
		    if (refile (++argp, drft) == 0)
			i = YESW;
		    break;
		default: 
		    advise (NULL, "say what?");
		    break;
	    }
	}
    }

    if (file) {
	/*
	 * We are replying to a file.
	 */
	anot = 0;	/* we don't want to annotate a file */
    } else {
	/*
	 * We are replying to a message.
	 */
	if (!msg)
	    msg = "cur";
	if (!folder)
	    folder = getfolder (1);
	maildir = m_maildir (folder);

	if (chdir (maildir) == NOTOK)
	    adios (maildir, "unable to change directory to");

	/* read folder and create message structure */
	if (!(mp = folder_read (folder, 1)))
	    adios (NULL, "unable to read folder %s", folder);

	/* check for empty folder */
	if (mp->nummsg == 0)
	    adios (NULL, "no messages in %s", folder);

	/* parse the message range/sequence/name and set SELECTED */
	if (!m_convert (mp, msg))
	    done (1);
	seq_setprev (mp);	/* set the previous-sequence */

	if (mp->numsel > 1)
	    adios (NULL, "only one message at a time!");

	context_replace (pfolder, folder);	/* update current folder   */
	seq_setcur (mp, mp->lowsel);	/* update current message  */
	seq_save (mp);			/* synchronize sequences   */
	context_save ();			/* save the context file   */
    }

    msg = file ? file : getcpy (m_name (mp->lowsel));

    if ((in = fopen (msg, "r")) == NULL)
	adios (msg, "unable to open");

    /* find form (components) file */
    if (!form) {
	if (groupreply)
	    form = etcpath (replgroupcomps);
	else
	    form = etcpath (replcomps);
    }

    replout (in, msg, drft, mp, outputlinelen, mime, form, filter,
    	     fcc, fmtproc);
    fclose (in);

    {
	char *filename = concat (mp->foldpath, "/", msg, NULL);

        for (n = 0; n < svector_size (convert_types); ++n) {
            add_convert_header (svector_at (convert_types, n),
                                svector_at (convert_args, n),
                                filename, drft);
        }
	free (filename);
    }

    if (nwhat)
	done (0);
    what_now (ed, nedit, NOUSE, drft, msg, 0, mp, anot ? "Replied" : NULL,
	    inplace, cwd, atfile);

    svector_free (convert_args);
    svector_free (convert_types);

    done (1);
    return 1;
}

void
docc (char *cp, int ccflag)
{
    switch (smatch (cp, ccswitches)) {
	case AMBIGSW: 
	    ambigsw (cp, ccswitches);
	    done (1);
	case UNKWNSW: 
	    adios (NULL, "-%scc %s unknown", ccflag ? "" : "no", cp);

	case CTOSW: 
	    ccto = ccflag;
	    break;

	case CCCSW: 
	    cccc = ccflag;
	    break;

	case CMESW: 
	    ccme = ccflag;
	    break;

	case CALSW: 
	    ccto = cccc = ccme = ccflag;
	    break;
    }
}

/*
 * Add pseudoheaders that will pass the convert arguments to
 * mhbuild.  They have the form:
 *   MHBUILD_FILE_PSEUDOHEADER-text/calendar: /home/user/Mail/inbox/7
 *   MHBUILD_ARGS_PSEUDOHEADER-text/calendar: reply -accept
 * The ARGS pseudoheader is optional, but we always add it when
 * -convertargs is used.
 */
void
add_convert_header (const char *convert_type, char *convert_arg,
                    char *filename, char *drft) {
    char *field_name;

    field_name = concat (MHBUILD_FILE_PSEUDOHEADER, convert_type, NULL);
    annotate (drft, field_name, filename, 1, 0, -2, 1);
    free (field_name);

    field_name = concat (MHBUILD_ARGS_PSEUDOHEADER, convert_type, NULL);
    annotate (drft, field_name, convert_arg, 1, 0, -2, 1);
    free (field_name);
}
