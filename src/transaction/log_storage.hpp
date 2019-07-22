/*
 * Copyright (C) 2008 Search Solution Corporation. All rights reserved by Search Solution.
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

//
// Define storage of logging system
//

#ifndef _LOG_STORAGE_HPP_
#define _LOG_STORAGE_HPP_

#include "cubstream.hpp"
#include "file_io.h"
#include "log_lsa.hpp"
#include "release_string.h"
#include "storage_common.h"
#include "system.h"
#include "transaction_global.hpp"

#include <cstdint>

const LOG_PAGEID LOGPB_HEADER_PAGE_ID = -9;     /* The first log page in the infinite log sequence. It is always kept
						 * on the active portion of the log. Log records are not stored on this
						 * page. This page is backed up in all archive logs */

const size_t LOGPB_IO_NPAGES = 4;
const size_t LOGPB_BUFFER_NPAGES_LOWER = 128;

/*
 * LOG PAGE
 */

typedef struct log_hdrpage LOG_HDRPAGE;
struct log_hdrpage
{
  LOG_PAGEID logical_pageid;	/* Logical pageid in infinite log */
  PGLENGTH offset;		/* Offset of first log record in this page. This may be useful when previous log page
				 * is corrupted and an archive of that page does not exist. Instead of losing the whole
				 * log because of such bad page, we could salvage the log starting at the offset
				 * address, that is, at the next log record */
  short dummy1;			/* Dummy field for 8byte align */
  int checksum;			/* checksum - currently CRC32 is used to check log page consistency. */
};

/* WARNING:
 * Don't use sizeof(LOG_PAGE) or of any structure that contains it
 * Use macro LOG_PAGESIZE instead.
 * It is also bad idea to allocate a variable for LOG_PAGE on the stack.
 */

typedef struct log_page LOG_PAGE;
struct log_page
{
  /* The log page */
  LOG_HDRPAGE hdr;
  char area[1];
};

const size_t MAXLOGNAME = (30 - 12);

// vacuum blocks
using VACUUM_LOG_BLOCKID = std::int64_t;

/*
 * This structure encapsulates various information and metrics related
 * to each backup level.
 * Estimates and heuristics are not currently used but are placeholder
 * for the future to avoid changing the physical representation again.
 */
typedef struct log_hdr_bkup_level_info LOG_HDR_BKUP_LEVEL_INFO;
struct log_hdr_bkup_level_info
{
  INT64 bkup_attime;		/* Timestamp when this backup lsa taken */
  INT64 io_baseln_time;		/* time (secs.) to write a single page */
  INT64 io_bkuptime;		/* total time to write the backup */
  int ndirty_pages_post_bkup;	/* number of pages written since the lsa for this backup level. */
  int io_numpages;		/* total number of pages in last backup */
};

/*
 * LOG HEADER INFORMATION
 */
typedef struct log_header LOG_HEADER;
struct log_header
{
  /* Log header information */
  char magic[CUBRID_MAGIC_MAX_LENGTH];	/* Magic value for file/magic Unix utility */
  /* Here exists 3 bytes */
  INT32 dummy;			/* for 8byte align */
  INT64 db_creation;		/* Database creation time. For safety reasons, this value is set on all volumes and the
				 * log. The value is generated by the log manager */
  char db_release[REL_MAX_RELEASE_LENGTH];	/* CUBRID Release */
  /* Here exists 1 byte */
  float db_compatibility;	/* Compatibility of the database against the current release of CUBRID */
  PGLENGTH db_iopagesize;	/* Size of pages in the database. For safety reasons this value is recorded in the log
				 * to make sure that the database is always run with the same page size */
  PGLENGTH db_logpagesize;	/* Size of log pages in the database. */
  bool is_shutdown;		/* Was the log shutdown ? */
  /* Here exists 3 bytes */
  TRANID next_trid;		/* Next Transaction identifier */
  MVCCID mvcc_next_id;		/* Next MVCC ID */
  int avg_ntrans;		/* Number of average transactions */
  int avg_nlocks;		/* Average number of object locks */
  DKNPAGES npages;		/* Number of pages in the active log portion. Does not include the log header page. */
  INT8 db_charset;
  bool was_copied;		/* set to true for copied database; should be reset on first server start */
  INT8 dummy3;			/* Dummy fields for 8byte align */
  INT8 dummy4;
  LOG_PAGEID fpageid;		/* Logical pageid at physical location 1 in active log */
  LOG_LSA append_lsa;		/* Current append location */
  LOG_LSA chkpt_lsa;		/* Lowest log sequence address to start the recovery process */
  LOG_PAGEID nxarv_pageid;	/* Next logical page to archive */
  LOG_PHY_PAGEID nxarv_phy_pageid;	/* Physical location of logical page to archive */
  int nxarv_num;		/* Next log archive number */
  int last_arv_num_for_syscrashes;	/* Last log archive needed for system crashes */
  int last_deleted_arv_num;	/* Last deleted archive number */
  LOG_LSA bkup_level0_lsa;	/* Lsa of backup level 0 */
  LOG_LSA bkup_level1_lsa;	/* Lsa of backup level 1 */
  LOG_LSA bkup_level2_lsa;	/* Lsa of backup level 2 */
  char prefix_name[MAXLOGNAME];	/* Log prefix name */
  bool has_logging_been_skipped;	/* Has logging been skipped ? */
  /* Here exists 5 bytes */
  VACUUM_LOG_BLOCKID vacuum_last_blockid;	/* Last processed blockid needed for vacuum. */
  int perm_status_obsolete;
  /* Here exists 4 bytes */
  LOG_HDR_BKUP_LEVEL_INFO bkinfo[FILEIO_BACKUP_UNDEFINED_LEVEL];
  /* backup specific info for future growth */

  int ha_server_state;
  int ha_file_status;
  LOG_LSA eof_lsa;

  LOG_LSA smallest_lsa_at_last_chkpt;

  LOG_LSA mvcc_op_log_lsa;	/* Used to link log entries for mvcc operations. Vacuum will then process these entries */
  MVCCID last_block_oldest_mvccid;	/* Used to find the oldest MVCCID in a block of log data. */
  MVCCID last_block_newest_mvccid;	/* Used to find the newest MVCCID in a block of log data. */

  INT64 ha_promotion_time;
  INT64 db_restore_time;
  bool mark_will_del;
  cubstream::stream_position m_ack_stream_position;

  log_header ()
    : magic {'0'}
    , dummy (0)
    , db_creation (0)
    , db_release {'0'}
    , db_compatibility (0.0f)
    , db_iopagesize (0)
    , db_logpagesize (0)
    , is_shutdown (false)
    , next_trid (LOG_SYSTEM_TRANID + 1)
    , mvcc_next_id (MVCCID_NULL)
    , avg_ntrans (0)
    , avg_nlocks (0)
    , npages (0)
    , db_charset (0)
    , was_copied (false)
    , dummy3 (0)
    , dummy4 (0)
    , fpageid (0)
    , append_lsa (NULL_LSA)
    , chkpt_lsa (NULL_LSA)
    , nxarv_pageid (0)
    , nxarv_phy_pageid (0)
    , nxarv_num (0)
    , last_arv_num_for_syscrashes (0)
    , last_deleted_arv_num (0)
    , bkup_level0_lsa (NULL_LSA)
    , bkup_level1_lsa (NULL_LSA)
    , bkup_level2_lsa (NULL_LSA)
    , prefix_name {'0'}
    , has_logging_been_skipped (false)
    , vacuum_last_blockid (0)
    , perm_status_obsolete (0)
    , bkinfo {{0, 0, 0, 0, 0}}
  , ha_server_state (0)
  , ha_file_status (0)
  , eof_lsa (NULL_LSA)
  , smallest_lsa_at_last_chkpt (NULL_LSA)
  , mvcc_op_log_lsa (NULL_LSA)
  , last_block_oldest_mvccid (MVCCID_NULL)
  , last_block_newest_mvccid (MVCCID_NULL)
  , ha_promotion_time (0)
  , db_restore_time (0)
  , mark_will_del (false)
  , m_ack_stream_position (0)
  {
    //
  }
};



typedef struct log_arv_header LOG_ARV_HEADER;
struct log_arv_header
{
  /* Log archive header information */
  char magic[CUBRID_MAGIC_MAX_LENGTH];	/* Magic value for file/magic Unix utility */
  INT32 dummy;			/* for 8byte align */
  INT64 db_creation;		/* Database creation time. For safety reasons, this value is set on all volumes and the
				 * log. The value is generated by the log manager */
  TRANID next_trid;		/* Next Transaction identifier */
  DKNPAGES npages;		/* Number of pages in the archive log */
  LOG_PAGEID fpageid;		/* Logical pageid at physical location 1 in archive log */
  int arv_num;			/* The archive number */
  INT32 dummy2;			/* Dummy field for 8byte align */

  log_arv_header ()
    : magic {'0'}
    , dummy (0)
    , db_creation (0)
    , next_trid (0)
    , npages (0)
    , fpageid (0)
    , arv_num (0)
    , dummy2 (0)
  {
  }
};

#endif // !_LOG_STORAGE_HPP_
