
/*
 * mhfree.c -- routines to free the data structures used to
 *          -- represent MIME messages
 *
 * This code is Copyright (c) 2002, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information.
 */

#include <h/mh.h>
#include <h/mime.h>
#include <h/mhparse.h>

/* The list of top-level contents to display */
CT *cts = NULL;

/*
 * prototypes
 */
void free_header (CT);
void free_ctinfo (CT);
void free_encoding (CT, int);
void freects_done (int);

/*
 * static prototypes
 */
static void free_text (CT);
static void free_multi (CT);
static void free_partial (CT);
static void free_external (CT);
static void free_pmlist (PM *);


/*
 * Primary routine to free a MIME content structure
 */

void
free_content (CT ct)
{
    if (!ct)
	return;

    /*
     * free all the header fields
     */
    free_header (ct);

    if (ct->c_partno) {
	free (ct->c_partno);
	ct->c_partno = NULL;
    }

    if (ct->c_vrsn) {
	free (ct->c_vrsn);
	ct->c_vrsn = NULL;
    }

    if (ct->c_ctline) {
	free (ct->c_ctline);
	ct->c_ctline = NULL;
    }

    free_ctinfo (ct);

    /*
     * some of the content types have extra
     * parts which need to be freed.
     */
    switch (ct->c_type) {
	case CT_MULTIPART:
	    free_multi (ct);
	    break;

	case CT_MESSAGE:
	    switch (ct->c_subtype) {
		case MESSAGE_PARTIAL:
		    free_partial (ct);
		    break;

		case MESSAGE_EXTERNAL:
		    free_external (ct);
		    break;
	    }
	    break;

	case CT_TEXT:
	    free_text (ct);
	    break;
    }

    if (ct->c_showproc) {
	free (ct->c_showproc);
	ct->c_showproc = NULL;
    }
    if (ct->c_termproc) {
	free (ct->c_termproc);
	ct->c_termproc = NULL;
    }
    if (ct->c_storeproc) {
	free (ct->c_storeproc);
	ct->c_storeproc = NULL;
    }

    if (ct->c_celine) {
	free (ct->c_celine);
	ct->c_celine = NULL;
    }

    /* free structures for content encodings */
    free_encoding (ct, 1);

    if (ct->c_id) {
	free (ct->c_id);
	ct->c_id = NULL;
    }
    if (ct->c_descr) {
	free (ct->c_descr);
	ct->c_descr = NULL;
    }
    if (ct->c_dispo) {
	free (ct->c_dispo);
	ct->c_dispo = NULL;
    }
    if (ct->c_dispo_type) {
	free (ct->c_dispo_type);
	ct->c_dispo_type = NULL;
    }
    free_pmlist (&ct->c_dispo_first);

    if (ct->c_file) {
	if (ct->c_unlink)
	    (void) m_unlink (ct->c_file);
	free (ct->c_file);
	ct->c_file = NULL;
    }
    if (ct->c_fp) {
	fclose (ct->c_fp);
	ct->c_fp = NULL;
    }

    if (ct->c_storage) {
	free (ct->c_storage);
	ct->c_storage = NULL;
    }
    if (ct->c_folder) {
	free (ct->c_folder);
	ct->c_folder = NULL;
    }

    free (ct);
}


/*
 * Free the linked list of header fields
 * for this content.
 */

void
free_header (CT ct)
{
    HF hp1, hp2;

    hp1 = ct->c_first_hf;
    while (hp1) {
	hp2 = hp1->next;

	free (hp1->name);
	free (hp1->value);
	free (hp1);

	hp1 = hp2;
    }

    ct->c_first_hf = NULL;
    ct->c_last_hf  = NULL;
}


void
free_ctinfo (CT ct)
{
    CI ci;

    ci = &ct->c_ctinfo;
    if (ci->ci_type) {
	free (ci->ci_type);
	ci->ci_type = NULL;
    }
    if (ci->ci_subtype) {
	free (ci->ci_subtype);
	ci->ci_subtype = NULL;
    }
    free_pmlist(&ci->ci_first_pm);
    if (ci->ci_comment) {
	free (ci->ci_comment);
	ci->ci_comment = NULL;
    }
    if (ci->ci_magic) {
	free (ci->ci_magic);
	ci->ci_magic = NULL;
    }
}


static void
free_text (CT ct)
{
    struct text *t;

    if (!(t = (struct text *) ct->c_ctparams))
	return;

    free ((char *) t);
    ct->c_ctparams = NULL;
}


static void
free_multi (CT ct)
{
    struct multipart *m;
    struct part *part, *next;

    if (!(m = (struct multipart *) ct->c_ctparams))
	return;

    if (m->mp_start)
	free (m->mp_start);
    if (m->mp_stop)
	free (m->mp_stop);
    free (m->mp_content_before);
    free (m->mp_content_after);
	
    for (part = m->mp_parts; part; part = next) {
	next = part->mp_next;
	free_content (part->mp_part);
	free ((char *) part);
    }
    m->mp_parts = NULL;

    free ((char *) m);
    ct->c_ctparams = NULL;
}


static void
free_partial (CT ct)
{
    struct partial *p;

    if (!(p = (struct partial *) ct->c_ctparams))
	return;

    if (p->pm_partid)
	free (p->pm_partid);

    free ((char *) p);
    ct->c_ctparams = NULL;
}


static void
free_external (CT ct)
{
    struct exbody *e;

    if (!(e = (struct exbody *) ct->c_ctparams))
	return;

    free_content (e->eb_content);
    if (e->eb_body)
	free (e->eb_body);
    if (e->eb_url)
    	free (e->eb_url);

    free ((char *) e);
    ct->c_ctparams = NULL;
}


static void
free_pmlist (PM *p)
{
    PM pm = *p, pm2;

    while (pm != NULL) {
    	if (pm->pm_name)
	    free (pm->pm_name);
    	if (pm->pm_value)
	    free (pm->pm_value);
    	if (pm->pm_charset)
	    free (pm->pm_charset);
    	if (pm->pm_lang)
	    free (pm->pm_lang);
	pm2 = pm->pm_next;
	free(pm);
	pm = pm2;
    }

    if (*p)
	*p = NULL;
}


/*
 * Free data structures related to encoding/decoding
 * Content-Transfer-Encodings.
 */

void
free_encoding (CT ct, int toplevel)
{
    CE ce = &ct->c_cefile;

    if (ce->ce_fp) {
	fclose (ce->ce_fp);
	ce->ce_fp = NULL;
    }

    if (ce->ce_file) {
	if (ce->ce_unlink)
	    (void) m_unlink (ce->ce_file);
	free (ce->ce_file);
	ce->ce_file = NULL;
    }

    if (! toplevel) {
	ct->c_ceopenfnx = NULL;
    }
}


void
freects_done (int status)
{
    CT *ctp;

    for (ctp = cts; ctp && *ctp; ctp++)
	free_content (*ctp);

    free (cts);

    exit (status);
}
