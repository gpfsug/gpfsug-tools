/*
 * GPFS support for rsync.
 * Written by Peter Somogyi
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, visit the http://fsf.org website.
 */ 


#include "rsync.h"
#include "ifuncs.h"

#include "lib/bst.h"
#include "zlib.h"

#define GPFS_ATTR_SIZE_MAX 0x100000
#define GPFS_ATTR_SIZE_INIT 0x2
#define GPFS_NDX_NOATTR -1

#define DEBUG_MIN_REPORT_LEVEL 3

struct rsync_gpfs_attr {
	int size;
	void *buf;
};

struct rsync_gpfs_attr2 {
	unsigned long crc32;
	int next_similar; /* index of the next attr having the same crc32 */

	unsigned char md5_digest[MD5_DIGEST_LEN];
};

/* Pointer list for GPFS attribute memory blocks */
static item_list gpfs_attr_list = EMPTY_ITEM_LIST;
static item_list gpfs_attr2_list = EMPTY_ITEM_LIST;
static struct bst_tree *gpfs_bst = NULL;

extern int gpfs_attrs_ndx; /* compat.c */
extern int dry_run; /* options.c */
extern int use_gpfs_attr_cache; /* options.c */

/*
 * which data should be transmitted in gpfs attributes:
 * - don't save file placement
 * - ignore pool
 */
static int gpfs_attr_flags = 3;

/*
 * GPFS ioctl specific values
 */
#define IOCTL_FATTR_ID (53)
static char *gpfs_devname = "/dev/ss0";
enum gpfs_func_id {gpfs_get_all_attrs = 27, gpfs_put_all_attrs = 28};

/* hope that gaps are the same... */
struct xattrs_params {
	int flags;
	void *buf;
	int size;
	int *size2;
};

static int gpfs_do_ioctl(int fd, enum gpfs_func_id func_id,
	struct xattrs_params *params, int *reason)
{
	int	gpfs_fd = open(gpfs_devname, O_RDONLY);
	long	args[5];
	int	rc;

	if (gpfs_fd == -1) {
		rprintf(FERROR, "gpfs_fgetattrs: open failed (%d)\n", errno);
		exit_cleanup(RERR_FILEIO);
	}

	args[0] = (long)fd;
	args[1] = (long)func_id;
	args[2] = (long)params;
	args[3] = (long)reason;

	rc = ioctl(gpfs_fd, IOCTL_FATTR_ID, &args);

	/* don't touch errno from now ! */

	close(gpfs_fd);

	return rc;
}

static int gpfs_fgetattrs(int fd, int flags, void *buffer, int bufferSize, int *attrSize)
{
	struct xattrs_params	params;

	params.flags = flags;
	params.buf = buffer;
	params.size = bufferSize;
	params.size2 = attrSize;

	return gpfs_do_ioctl(fd, gpfs_get_all_attrs, &params, NULL);
}

static int gpfs_fputattrs(int fd, int flags, void *buffer)
{
	struct xattrs_params	params;

	params.flags = flags;
	params.buf = buffer;

	return gpfs_do_ioctl(fd, gpfs_put_all_attrs, &params, NULL);
}


static void gpfs_report_bst(void)
{
	rprintf(FINFO, "gpfs bst: depth=%d, nodes=%d, hit=%d\n",
		gpfs_bst->depth, gpfs_bst->nodes, gpfs_bst->hit);
}

void gpfs_free_attr(struct rsync_gpfs_attr *a)
{
	if (!a)
		return;
	if (a->buf) {
		free(a->buf);
		a->buf = NULL;
	}
}

void gpfs_free_sxp(stat_x *sxp)
{
	if (!sxp->gpfs_attr)
		return;
	gpfs_free_attr(sxp->gpfs_attr);
	free(sxp->gpfs_attr);
	sxp->gpfs_attr = NULL;
}

void gpfs_free_list(void)
{
	int	i = gpfs_attr_list.count;
	int	rc;

	struct rsync_gpfs_attr *a = gpfs_attr_list.items;

	while(i) {
		gpfs_free_attr(a++);
		i--;
	}

	if (gpfs_bst) {

		rc = bst_free(gpfs_bst);
		if (rc) {
			rprintf(FERROR, "bst_free rc=%d\n", rc);
			exit_cleanup(RERR_CRASHED);
		}
	}
}

/* return: 0 when equal */
static int gpfs_attr_cmp(const struct rsync_gpfs_attr *a1,
	const struct rsync_gpfs_attr *a2)
{
	if (a1 == a2)
		return 0;
	if (!a1 || !a2)
		return -1;
	/* now a1 and a2 are nonzero */

	if (a1->buf == a2->buf)
		return 0;
	if (!a1->buf || !a2->buf)
		return -1;
	/* now both buffers are nonzero */

	if (a1->size != a2->size)
		return -1;

	return memcmp(a1->buf, a2->buf, a1->size);
}

static struct rsync_gpfs_attr *gpfs_get_attr_elem(int index)
{
	struct rsync_gpfs_attr *a = (struct rsync_gpfs_attr *)gpfs_attr_list.items;
	return a + index;
}

static struct rsync_gpfs_attr2 *gpfs_get_attr2_elem(int index)
{
	struct rsync_gpfs_attr2 *a2 = (struct rsync_gpfs_attr2 *)gpfs_attr2_list.items;
	return a2 + index;
}

static int gpfs_receive_attr_int(int f)
{
	int ndx = read_varint(f) - 1;
	struct rsync_gpfs_attr	*a;


	if (ndx == GPFS_NDX_NOATTR)
		return ndx; /* having no GPFS attribute */

	if (ndx < 0 || ndx > (int)gpfs_attr_list.count) {
		rprintf(FERROR_XFER, "gpfs_receive_attr: GPFS attr "
			"index %d is out of [-1,  %lu]\n", ndx, gpfs_attr_list.count);
		exit_cleanup(RERR_STREAMIO);
	}

	if (ndx != 0) {
		/* cached */
		return ndx - 1;
	}

	a = EXPAND_ITEM_LIST(&gpfs_attr_list, struct rsync_gpfs_attr, 1000);
	if (!a) {
		rprintf(FERROR, "failed to allocate gpfs attribute "
			"item (at count: %d)\n", (int)gpfs_attr_list.count);
		exit_cleanup(RERR_MALLOC);
	}

	/* read  */
	a->size = read_varint(f);


	if (a->size > GPFS_ATTR_SIZE_MAX) {
		rprintf(FERROR_XFER, "gpfs attribute is too big (%d), "
			"possible protocol error\n", a->size);
		exit_cleanup(RERR_STREAMIO);
	}

	a->buf = malloc(a->size);
	if (!a->buf) {
		rprintf(FERROR, "failed to allocate gpfs attribute buffer (%d)\n",
			a->size);
		exit_cleanup(RERR_MALLOC);
	}
	read_buf(f, a->buf, a->size);

	return (gpfs_attr_list.count - 1);
}

void gpfs_receive_attr(struct file_struct *file, int f)
{
	F_GPFS_ATTR(file) = gpfs_receive_attr_int(f);
}

static void gpfs_calc_attr2(struct rsync_gpfs_attr *a, struct rsync_gpfs_attr2 *a2)
{
	get_md5(a2->md5_digest, (uchar *)a->buf, (int)a->size);
	a2->crc32 = crc32(0, (unsigned char *)a->buf, a->size);
	a2->next_similar = -1;
}

static void gpfs_add_attr(struct rsync_gpfs_attr *b, struct rsync_gpfs_attr2 *b2)
{
	struct rsync_gpfs_attr *a;
	struct rsync_gpfs_attr2 *a2;

	if (!b)
		return;

	a = EXPAND_ITEM_LIST(&gpfs_attr_list, struct rsync_gpfs_attr, 1000);
	if (!a) {
		rprintf(FERROR, "failed to allocate gpfs attribute "
			"item (at count: %d)\n", (int)gpfs_attr_list.count);
		exit_cleanup(RERR_MALLOC);
	}

	a->buf = malloc(b->size);
	if (!a->buf) {
		rprintf(FERROR, "failed to allocate gpfs attribute "
			"size %u (at count: %d)\n", b->size,
			(int)gpfs_attr_list.count);
		exit_cleanup(RERR_MALLOC);
	}

	memcpy(a->buf, b->buf, b->size);
	a->size = b->size;

	/* calculate the second part */
	a2 = EXPAND_ITEM_LIST(&gpfs_attr2_list, struct rsync_gpfs_attr2, 1000);
	if (!a2) {
		rprintf(FERROR, "failed to allocate gpfs attribute2 "
			"item (at count: %d)\n", (int)gpfs_attr2_list.count);
		exit_cleanup(RERR_MALLOC);
	}

	memcpy(a2, b2, sizeof(struct rsync_gpfs_attr2));
}

/* MUST BE INVOKED ON THE SENDER ONLY */
static int gpfs_find_attr(struct rsync_gpfs_attr *a)
{
	int	rc;
	int	similar;
	struct rsync_gpfs_attr2	a2; /* calculated part */

	/* do the calculation only once,
	 * just at the beginning */
	gpfs_calc_attr2(a, &a2);

	/* create tree if it doesn't exist yet */
	if (!gpfs_bst) {
		gpfs_bst = bst_new_tree();
		if (!gpfs_bst) {
			rprintf(FERROR, "failed to allocate bst\n");
			exit_cleanup(RERR_MALLOC);
		}
	}

	/* search/insert into tree */
	rc = bst_insert(gpfs_bst, a2.crc32, gpfs_attr2_list.count, &similar);
	if (rc<0) {
		rprintf(FERROR, "gpfs bst_search error %d", rc);
		gpfs_report_bst();
		exit_cleanup((rc==-1) ? RERR_MALLOC: RERR_CRASHED);
	} else if (rc>0) { /* found attr having similar crc32 */
		struct rsync_gpfs_attr2 *c2 = NULL;

		/* try to find equal in the similar chain */
		while(similar>=0) {
			c2 = gpfs_get_attr2_elem(similar);

			if (memcmp(c2->md5_digest, a2.md5_digest, MD5_DIGEST_LEN)==0) {
				if (gpfs_attr_cmp(a, gpfs_get_attr_elem(similar))==0) {
					/* found */

#ifdef __TEST_RARE_CONDITION__
				static int gpfs_wrong = 0;
				gpfs_wrong = (gpfs_wrong + 1)%4;
				if (gpfs_wrong++ > 2)
#endif

					return similar;
				}
			}

			/* get the next one */
			similar = c2->next_similar;
		}


		/* now c2 must be the last similar one in the chain */
		if (!c2) {
			rprintf(FERROR, "gpfs bst chain inconsistency\n");
			gpfs_report_bst();
			exit_cleanup(RERR_CRASHED);
		}
		c2->next_similar = gpfs_attr2_list.count; /* will be added in the next step */
	} /* else couldn't find, and a2->next_similar is kept -1 */

	gpfs_add_attr(a, &a2);
	return -1;
}

void gpfs_cache_attr(struct file_struct *file, stat_x *sxp)
{
	int	match;
	struct rsync_gpfs_attr	*a = sxp->gpfs_attr;

	if (!use_gpfs_attr_cache || !a || !a->buf)
		return;

	match =  gpfs_find_attr(a);
	if (match >= 0) {
		F_GPFS_ATTR(file) = match;
	} else {
		/* already added by gpfs_find_attr */
		F_GPFS_ATTR(file) = gpfs_attr_list.count - 1;
	}
}

void gpfs_send_attr(stat_x *sxp, int f)
{
	struct rsync_gpfs_attr	*a = sxp->gpfs_attr;

	if (a && a->buf) {
		int ndx = (use_gpfs_attr_cache) ? gpfs_find_attr(a) : -1;


		/* write 1 if it was 0; means not in cache (yet), +1 */
		/* +1 is to compress the GPFS_NDX_NOATTR better */
		write_varint(f, ndx + 1 + 1);

		if (ndx < 0) {

			write_varint(f, a->size);
			write_buf(f, a->buf, a->size);
		}
	} else {
		/* we have no attribute for this file */

		/* +1 is just because of the trivial compression
		 * of the mostly awaited GPFS_NDX_NOATTR  */
		write_varint(f, GPFS_NDX_NOATTR + 1);
	}
}

static void gpfs_dump_attr(int ll, const char *fname, unsigned char *buf, int size)
{
	int	col;
	unsigned int b;

	rprintf(ll, "%s attr size: %d ptr: %lx\n", fname, size, (unsigned long)buf);

	while(size) {
		col = 16;
		while(col && size) {
			b = (unsigned int)(*(buf++));
			rprintf(ll, "%.2x ", b);

			col--;
			size--;
		}
		rprintf(ll, "\n");
	}
}

/*
 * return: 0 on success, -1 on error
 */
int gpfs_get_attr(const char *fname, stat_x *sxp)
{
	int	rc, fd;
	struct rsync_gpfs_attr *a;

	fd = open(fname, O_RDONLY | O_DIRECT | O_NOATIME | O_NOFOLLOW
				| O_NONBLOCK | O_BINARY);
	if (fd==-1) {
		gpfs_free_sxp(sxp); /* makes sxp->gpfs_attr=NULL */
		rprintf(FWARNING, "gpfs_get_attr: failed to open file %s errno %d\n",
			fname, errno);
		return -1;
	}

	a = new(struct rsync_gpfs_attr);
	if (!a)
		out_of_memory("gpfs_get_attr#1");

	a->buf = malloc(GPFS_ATTR_SIZE_INIT);
	if (!a->buf)
		out_of_memory("gpfs_get_attr#2");
	a->size = GPFS_ATTR_SIZE_INIT;
	rc = gpfs_fgetattrs(fd, gpfs_attr_flags, a->buf, a->size, &a->size);
	if (rc==-1 && (errno==ENOSPC || errno==ERANGE)) {
		free(a->buf);
		a->buf = malloc(a->size);
		if (!a->buf) {
			rprintf(FERROR, "malloc of gpfs_fgetattrs failed (size: %d)\n",
				a->size);
			exit_cleanup(RERR_MALLOC);
		}
		rc = gpfs_fgetattrs(fd, gpfs_attr_flags, a->buf, a->size, &a->size);
		if (rc) {
			rprintf(FWARNING, "gpfs_fgetattrs failed (%d, %d, %d)\n",
				a->size, rc, errno);
			free(a->buf);
			a->buf = NULL;
		} else {
			/* a good case: do nothing here */
		}
	} else if (!rc) {
		if (a->size==0) {
			/* we don't have any attribute for this path */
			free(a->buf);
			a->buf = NULL;
		} else {
			/* don't waste memory */
			a->buf = realloc(a->buf, a->size);
			if (!a->buf) {
				rprintf(FERROR, "realloc of gpfs_fgetattrs failed (size: %d)\n",
					a->size);
				exit_cleanup(RERR_MALLOC);
			}
			/* a good case */
		}
	} else { /* other error (e.g. permission problem) */
		rprintf(FWARNING, "gpfs_fgetattrs failed (%s, %d, %d, %d)\n",
			fname, a->size, rc, errno);
		free(a->buf);
		a->buf = NULL;
	}

	if (!a->buf)
		a->size = 0;

	sxp->gpfs_attr = a;
	/* now each case has assigned sxp->gpfs_attr->buf */

	close(fd);
	return (rc ? -1 : 0);
}

/*
 * return: 0 on success, error otherwise
 */
int gpfs_set_attr(const char *fname, struct file_struct *file)
{
	int	rc;
	int	fd;
	int	ndx = F_GPFS_ATTR(file);


	fd = open(fname, O_RDONLY | O_DIRECT | O_NOATIME |  O_NOFOLLOW
				| O_NONBLOCK | O_BINARY);
	if (fd==-1) {
		rprintf(FWARNING, "gpfs_set_attr: failed to open file %s errno %d\n",
			fname, errno);
		return -1;
	}

	if (dry_run) {
		close(fd);
		return 0;
	}

	if (ndx == GPFS_NDX_NOATTR) {
		/* remove any attribute */
		rc = gpfs_fputattrs(fd, gpfs_attr_flags, NULL);
		if (rc) {
			rprintf(FWARNING, "gpfs_fputattrs failed (%s, %d, %d, NULL)\n",
				fname, rc, errno);
		}
	} else {
		struct rsync_gpfs_attr *a = gpfs_attr_list.items;
		a += ndx;

		if (a->buf==NULL) {
			rprintf(FERROR, "gpfs_set_attr internal error (%s, %d)\n",
				fname, ndx);
			rc = -1;
		} else {
			rc = gpfs_fputattrs(fd, gpfs_attr_flags, a->buf);
			if (rc) {
				rprintf(FWARNING, "gpfs_fputattrs failed (%s, %d, %d, %d)\n",
					fname, a ? a->size : -1, rc, errno);
			}
		}
	}

	close(fd);
	return rc;
}

int gpfs_attr_get_changed(const char *fname, struct file_struct *file, stat_x *sxp)
{
	struct rsync_gpfs_attr *a;
	int	has_changed;

	if (!sxp->gpfs_attr) {
		if (gpfs_get_attr(fname, sxp))
			return 0;
	}

	if (F_GPFS_ATTR(file)>=0) {
		a = gpfs_attr_list.items;
		a += F_GPFS_ATTR(file);
		has_changed = gpfs_attr_cmp(a, sxp->gpfs_attr);
	} else {
		has_changed = (sxp->gpfs_attr!=NULL &&
			sxp->gpfs_attr->size != 0);
	}


	return has_changed;
}

