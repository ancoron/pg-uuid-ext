/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>

#include "postgres.h"

#include "access/hash.h"
#include "datatype/timestamp.h"
#include "lib/stringinfo.h"
#include "lib/hyperloglog.h"
#include "port/pg_bswap.h"
#include "utils/builtins.h"
#include "utils/guc.h"
#include "utils/sortsupport.h"
#include "utils/timestamp.h"
#include "utils/uuid.h"
#include "uuid_ext.h"

PG_MODULE_MAGIC;

/* sortsupport for uuid */
typedef struct
{
	int64 input_count; /* number of non-null values seen */
	bool estimating; /* true if estimating cardinality */

	hyperLogLogState abbr_card; /* cardinality estimator */
} uuid_ts_sortsupport_state;

static int64 uuid_v1_timestamp0(const pg_uuid_t *uuid);
static TimestampTz uuid_v1_timestamp1(const pg_uuid_t *uuid);
static int uuid_ts_cmp0(const pg_uuid_t *a, const pg_uuid_t *b);
static int uuid_ts_only_cmp0(const pg_uuid_t *a, const TimestampTz b);

static int uuid_ts_cmp_abbrev(Datum x, Datum y, SortSupport ssup);
static bool uuid_ts_abbrev_abort(int memtupcount, SortSupport ssup);
static Datum uuid_ts_abbrev_convert(Datum original, SortSupport ssup);
static int uuid_ts_sort_cmp(Datum x, Datum y, SortSupport ssup);

PG_FUNCTION_INFO_V1(uuid_v1_timestamp);
PG_FUNCTION_INFO_V1(uuid_version);
PG_FUNCTION_INFO_V1(uuid_variant);
PG_FUNCTION_INFO_V1(uuid_v1_node);
PG_FUNCTION_INFO_V1(generate_uuid_v1_at);

PG_FUNCTION_INFO_V1(uuid_timestamp_sortsupport);

PG_FUNCTION_INFO_V1(uuid_timestamp_cmp);
PG_FUNCTION_INFO_V1(uuid_timestamp_eq);
PG_FUNCTION_INFO_V1(uuid_timestamp_ne);
PG_FUNCTION_INFO_V1(uuid_timestamp_lt);
PG_FUNCTION_INFO_V1(uuid_timestamp_le);
PG_FUNCTION_INFO_V1(uuid_timestamp_gt);
PG_FUNCTION_INFO_V1(uuid_timestamp_ge);

PG_FUNCTION_INFO_V1(uuid_timestamp_only_cmp);
PG_FUNCTION_INFO_V1(uuid_timestamp_only_eq);
PG_FUNCTION_INFO_V1(uuid_timestamp_only_ne);
PG_FUNCTION_INFO_V1(uuid_timestamp_only_lt);
PG_FUNCTION_INFO_V1(uuid_timestamp_only_le);
PG_FUNCTION_INFO_V1(uuid_timestamp_only_gt);
PG_FUNCTION_INFO_V1(uuid_timestamp_only_ge);

/*
 * External functions first.
 */

uint8
uuid_get_version(const pg_uuid_t *uuid)
{
	return UUID_VERSION(uuid);
}

uint8
uuid_get_variant(const pg_uuid_t *uuid)
{
	if (UUID_VARIANT_IS_RFC4122(uuid))
		return UUID_VARIANT_RFC4122;

	if (UUID_VARIANT_IS_GUID(uuid))
		return UUID_VARIANT_GUID;

	if (UUID_VARIANT_IS_NCS(uuid))
		return UUID_VARIANT_NCS;

	return UUID_VARIANT_FUTURE;
}

bool
uuid_is_rfc_v1(const pg_uuid_t *uuid)
{
	return 1 == UUID_VERSION(uuid) && UUID_VARIANT_IS_RFC4122(uuid);
}

/*
 * Extract and un-shuffle the 60 bits (version 1) UUID timestamp.
 */
static int64
uuid_v1_timestamp0(const pg_uuid_t *uuid)
{
	/* UUID timestamp is encoded in network byte order */
	int64 timestamp = pg_ntoh64(*(int64 *) uuid->data);

	timestamp = (
			((timestamp << 48) & 0x0FFF000000000000) |
			((timestamp << 16) & 0x0000FFFF00000000) |
			((timestamp >> 32) & 0x00000000FFFFFFFF));

	return timestamp;
}

/*
 * Extract the timestamp from a version 1 UUID.
 */
static TimestampTz
uuid_v1_timestamp1(const pg_uuid_t *uuid)
{
	/* from 100 ns precision to PostgreSQL epoch */
	TimestampTz timestamp = uuid_v1_timestamp0(uuid) / 10 - PG_UUID_OFFSET;

	return timestamp;
}

/*
 * uuid_v1_timestamp
 *	extract the timestamp of a version 1 UUID
 *
 */
Datum
uuid_v1_timestamp(PG_FUNCTION_ARGS)
{
	TimestampTz timestamp = 0L;
	pg_uuid_t *uuid = PG_GETARG_UUID_P(0);

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	/* version and variant check */
	if (!uuid_is_rfc_v1(uuid))
		PG_RETURN_NULL();

	timestamp = uuid_v1_timestamp1(uuid);

	/* Recheck in case roundoff produces something just out of range */
	if (!IS_VALID_TIMESTAMP(timestamp))
		ereport(ERROR,
			(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
			errmsg("timestamp out of range")));

	PG_RETURN_TIMESTAMP(timestamp);
}

/*
 * uuid_version
 *	extract the version of a UUID
 *
 */
Datum
uuid_version(PG_FUNCTION_ARGS)
{
	pg_uuid_t *uuid = PG_GETARG_UUID_P(0);

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	PG_RETURN_TIMESTAMP(uuid_get_version(uuid));
}

/*
 * uuid_version
 *	extract the version of a UUID
 *
 */
Datum
uuid_variant(PG_FUNCTION_ARGS)
{
	pg_uuid_t *uuid = PG_GETARG_UUID_P(0);

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	PG_RETURN_TIMESTAMP(uuid_get_variant(uuid));
}

/*
 * uuid_v1_node
 *	extract the node of a version 1 UUID (returns NULL otherwise)
 *
 */
Datum
uuid_v1_node(PG_FUNCTION_ARGS)
{
	pg_uuid_t *uuid = PG_GETARG_UUID_P(0);
	static const char hex_chars[] = "0123456789abcdef";
	char* node = palloc(13);
	int i;
	int j = 0;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	/* version and variant check */
	if (!uuid_is_rfc_v1(uuid))
		PG_RETURN_NULL();

	for (i = 10; i < UUID_LEN; i++)
	{
		int hi;
		int lo;

		hi = uuid->data[i] >> 4;
		lo = uuid->data[i] & 0x0F;

		node[j++] = hex_chars[hi];
		node[j++] = hex_chars[lo];
	}

	node[j++] = '\0';

	PG_RETURN_TEXT_P(cstring_to_text(node));
}

Datum
generate_uuid_v1_at(PG_FUNCTION_ARGS)
{
	int64 timestamp = (int64) PG_GETARG_TIMESTAMPTZ(0);

	pg_uuid_t *uuid;
	uuid = (pg_uuid_t*) palloc0(UUID_LEN);
	timestamp = (timestamp + PG_UUID_OFFSET) * 10;

	/* shuffle timestamp bytes into low, mid, high and set version */
	timestamp = (
			((timestamp >> 48) & 0x0000000000000FFF) |
			((timestamp >> 16) & 0x00000000FFFF0000) |
			((timestamp << 32) & 0xFFFFFFFF00000000) | 0x1000);

	/* convert to network byte order and copy into byte array */
	timestamp = pg_hton64(timestamp);
	memcpy(&(uuid->data)[0], &timestamp, 8);

	/* set RFC variant bits */
	uuid->data[8] = 0x80;

	PG_RETURN_UUID_P(uuid);
}

static int
uuid_ts_cmp0(const pg_uuid_t *a, const pg_uuid_t *b)
{
	int64 diff;

	uint8 version = uuid_get_version(a);
	diff = version - uuid_get_version(b);
	if (diff < 0)
		return -1;
	else if (diff > 0)
		return 1;

	if (version == 1)
	{
		diff = uuid_v1_timestamp0(a) - uuid_v1_timestamp0(b);
		if (diff < 0)
			return -1;
		else if (diff > 0)
			return 1;
	}

	/* timestamp equals so just compare on memory */
	diff = memcmp(a, b, UUID_LEN);
	if (diff < 0)
		return -1;
	else if (diff > 0)
		return 1;
	return 0;
}

Datum
uuid_timestamp_cmp(PG_FUNCTION_ARGS)
{
	pg_uuid_t *a = PG_GETARG_UUID_P(0);
	pg_uuid_t *b = PG_GETARG_UUID_P(1);

	PG_RETURN_INT32(uuid_ts_cmp0(a, b));
}

Datum
uuid_timestamp_eq(PG_FUNCTION_ARGS)
{
	pg_uuid_t *a = PG_GETARG_UUID_P(0);
	pg_uuid_t *b = PG_GETARG_UUID_P(1);

	PG_RETURN_BOOL(memcmp(a, b, UUID_LEN) == 0);
}

Datum
uuid_timestamp_ne(PG_FUNCTION_ARGS)
{
	pg_uuid_t *a = PG_GETARG_UUID_P(0);
	pg_uuid_t *b = PG_GETARG_UUID_P(1);

	PG_RETURN_BOOL(memcmp(a, b, UUID_LEN) != 0);
}

Datum
uuid_timestamp_lt(PG_FUNCTION_ARGS)
{
	pg_uuid_t *a = PG_GETARG_UUID_P(0);
	pg_uuid_t *b = PG_GETARG_UUID_P(1);

	PG_RETURN_BOOL(uuid_ts_cmp0(a, b) < 0);
}

Datum
uuid_timestamp_le(PG_FUNCTION_ARGS)
{
	pg_uuid_t *a = PG_GETARG_UUID_P(0);
	pg_uuid_t *b = PG_GETARG_UUID_P(1);

	PG_RETURN_BOOL(uuid_ts_cmp0(a, b) <= 0);
}

Datum
uuid_timestamp_gt(PG_FUNCTION_ARGS)
{
	pg_uuid_t *a = PG_GETARG_UUID_P(0);
	pg_uuid_t *b = PG_GETARG_UUID_P(1);

	PG_RETURN_BOOL(uuid_ts_cmp0(a, b) > 0);
}

Datum
uuid_timestamp_ge(PG_FUNCTION_ARGS)
{
	pg_uuid_t *a = PG_GETARG_UUID_P(0);
	pg_uuid_t *b = PG_GETARG_UUID_P(1);

	PG_RETURN_BOOL(uuid_ts_cmp0(a, b) >= 0);
}

static int
uuid_ts_only_cmp0(const pg_uuid_t *a, const TimestampTz b)
{
	if (!uuid_is_rfc_v1(a))
		return -1;

	return timestamptz_cmp_internal(uuid_v1_timestamp1(a), b);
}

Datum
uuid_timestamp_only_cmp(PG_FUNCTION_ARGS)
{
	pg_uuid_t *a = PG_GETARG_UUID_P(0);
	TimestampTz b = PG_GETARG_TIMESTAMPTZ(1);

	PG_RETURN_INT32(uuid_ts_only_cmp0(a, b));
}

Datum
uuid_timestamp_only_eq(PG_FUNCTION_ARGS)
{
	pg_uuid_t *a = PG_GETARG_UUID_P(0);
	TimestampTz b = PG_GETARG_TIMESTAMPTZ(1);

	PG_RETURN_BOOL(uuid_ts_only_cmp0(a, b) == 0);
}

Datum
uuid_timestamp_only_ne(PG_FUNCTION_ARGS)
{
	pg_uuid_t *a = PG_GETARG_UUID_P(0);
	TimestampTz b = PG_GETARG_TIMESTAMPTZ(1);

	PG_RETURN_BOOL(uuid_ts_only_cmp0(a, b) != 0);
}

Datum
uuid_timestamp_only_lt(PG_FUNCTION_ARGS)
{
	pg_uuid_t *a = PG_GETARG_UUID_P(0);
	TimestampTz b = PG_GETARG_TIMESTAMPTZ(1);

	PG_RETURN_BOOL(uuid_ts_only_cmp0(a, b) < 0);
}

Datum
uuid_timestamp_only_le(PG_FUNCTION_ARGS)
{
	pg_uuid_t *a = PG_GETARG_UUID_P(0);
	TimestampTz b = PG_GETARG_TIMESTAMPTZ(1);

	PG_RETURN_BOOL(uuid_ts_only_cmp0(a, b) <= 0);
}

Datum
uuid_timestamp_only_gt(PG_FUNCTION_ARGS)
{
	pg_uuid_t *a = PG_GETARG_UUID_P(0);
	TimestampTz b = PG_GETARG_TIMESTAMPTZ(1);

	PG_RETURN_BOOL(uuid_ts_only_cmp0(a, b) > 0);
}

Datum
uuid_timestamp_only_ge(PG_FUNCTION_ARGS)
{
	pg_uuid_t *a = PG_GETARG_UUID_P(0);
	TimestampTz b = PG_GETARG_TIMESTAMPTZ(1);

	PG_RETURN_BOOL(uuid_ts_only_cmp0(a, b) >= 0);
}


/*
 * Parts of below code have been shamelessly copied (and modified) from:
 *	  src/backend/utils/adt/uuid.c
 *
 * ...and are:
 * Copyright (c) 2007-2019, PostgreSQL Global Development Group
 */

/*
 * Sort support strategy routine
 */
Datum
uuid_timestamp_sortsupport(PG_FUNCTION_ARGS)
{
	SortSupport ssup = (SortSupport) PG_GETARG_POINTER(0);

	ssup->comparator = uuid_ts_sort_cmp;
	ssup->ssup_extra = NULL;

	if (ssup->abbreviate)
	{
		uuid_ts_sortsupport_state *uss;
		MemoryContext oldcontext;

		oldcontext = MemoryContextSwitchTo(ssup->ssup_cxt);

		uss = palloc(sizeof(uuid_ts_sortsupport_state));
		uss->input_count = 0;
		uss->estimating = true;
		initHyperLogLog(&uss->abbr_card, 10);

		ssup->ssup_extra = uss;

		ssup->comparator = uuid_ts_cmp_abbrev;
		ssup->abbrev_converter = uuid_ts_abbrev_convert;
		ssup->abbrev_abort = uuid_ts_abbrev_abort;
		ssup->abbrev_full_comparator = uuid_ts_sort_cmp;

		MemoryContextSwitchTo(oldcontext);
	}

	PG_RETURN_VOID();
}

/*
 * SortSupport comparison func
 */
static int
uuid_ts_sort_cmp(Datum x, Datum y, SortSupport ssup)
{
	pg_uuid_t *arg1 = DatumGetUUIDP(x);
	pg_uuid_t *arg2 = DatumGetUUIDP(y);

	return uuid_ts_cmp0(arg1, arg2);
}

/*
 * Conversion routine for sortsupport.
 *
 * Converts original uuid representation to abbreviated key representation.
 *
 * Our encoding strategy is simple: if the UUID is an RFC 4122 version 1 then
 * extract the 60-bit timestamp. Otherwise, pack the first `sizeof(Datum)`
 * bytes of uuid data into a Datum (on little-endian machines, the bytes are
 * stored in reverse order), and treat it as an unsigned integer.
 */
static Datum
uuid_ts_abbrev_convert(Datum original, SortSupport ssup)
{
	uuid_ts_sortsupport_state *uss = ssup->ssup_extra;
	pg_uuid_t *authoritative = DatumGetUUIDP(original);
	Datum res;

#if SIZEOF_DATUM == 8
	if (uuid_is_rfc_v1(authoritative))
	{
		int64 timestamp = uuid_v1_timestamp0(authoritative);
		memcpy(&res, &timestamp, sizeof(Datum));
	}
	else
	{
		memcpy(&res, authoritative->data, sizeof(Datum));
	}
#else       /* SIZEOF_DATUM != 8 */
	/*
	 * First 4 bytes are already the most significant bits.
	 */
	memcpy(&res, authoritative->data, sizeof(Datum));
#endif

	uss->input_count += 1;

	if (uss->estimating)
	{
		uint32 tmp;

#if SIZEOF_DATUM == 8
		tmp = (uint32) res ^ (uint32) ((uint64) res >> 32);
#else       /* SIZEOF_DATUM != 8 */
		tmp = (uint32) res;
#endif

		addHyperLogLog(&uss->abbr_card, DatumGetUInt32(hash_uint32(tmp)));
	}

	/*
	 * Byteswap on little-endian machines.
	 *
	 * This is needed so that uuid_ts_cmp_abbrev() (an unsigned integer 3-way
	 * comparator) works correctly on all platforms.  If we didn't do this,
	 * the comparator would have to call memcmp() with a pair of pointers to
	 * the first byte of each abbreviated key, which is slower.
	 */
	res = DatumBigEndianToNative(res);

	return res;
}

/*
 * Abbreviated key comparison func
 */
static int
uuid_ts_cmp_abbrev(Datum x, Datum y, SortSupport ssup)
{
	if (x > y)
		return 1;
	else if (x == y)
		return 0;
	else
		return -1;
}

/*
 * Callback for estimating effectiveness of abbreviated key optimization.
 *
 * We pay no attention to the cardinality of the non-abbreviated data, because
 * there is no equality fast-path within authoritative uuid comparator.
 */
static bool
uuid_ts_abbrev_abort(int memtupcount, SortSupport ssup)
{
	uuid_ts_sortsupport_state *uss = ssup->ssup_extra;
	double abbr_card;

	if (memtupcount < 10000 || uss->input_count < 10000 || !uss->estimating)
		return false;

	abbr_card = estimateHyperLogLog(&uss->abbr_card);

	/*
	 * If we have >100k distinct values, then even if we were sorting many
	 * billion rows we'd likely still break even, and the penalty of undoing
	 * that many rows of abbrevs would probably not be worth it.  Stop even
	 * counting at that point.
	 */
	if (abbr_card > 100000.0)
	{
#ifdef TRACE_SORT
		if (trace_sort)
			elog(LOG,
				"uuid_ts_abbrev: estimation ends at cardinality %f"
				" after " INT64_FORMAT " values (%d rows)",
				abbr_card, uss->input_count, memtupcount);
#endif
		uss->estimating = false;
		return false;
	}

	/*
	 * Target minimum cardinality is 1 per ~2k of non-null inputs.  0.5 row
	 * fudge factor allows us to abort earlier on genuinely pathological data
	 * where we've had exactly one abbreviated value in the first 2k
	 * (non-null) rows.
	 */
	if (abbr_card < uss->input_count / 2000.0 + 0.5)
	{
#ifdef TRACE_SORT
		if (trace_sort)
			elog(LOG,
				"uuid_ts_abbrev: aborting abbreviation at cardinality %f"
				" below threshold %f after " INT64_FORMAT " values (%d rows)",
				abbr_card, uss->input_count / 2000.0 + 0.5, uss->input_count,
				memtupcount);
#endif
		return true;
	}

#ifdef TRACE_SORT
	if (trace_sort)
		elog(LOG,
			"uuid_ts_abbrev: cardinality %f after " INT64_FORMAT
			" values (%d rows)", abbr_card, uss->input_count, memtupcount);
#endif

	return false;
}
