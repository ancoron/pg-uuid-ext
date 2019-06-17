/*
 * Copyright 2019 Ancoron Luciferis
 *
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

#define UUID_TIME_OFFSET            INT64CONST(122192928000000000)

#define UUID_VARIANT_NCS            (0x00)
#define UUID_VARIANT_RFC4122        (0x01)
#define UUID_VARIANT_GUID           (0x02)
#define UUID_VARIANT_FUTURE         (0x03)

#define UUID_VERSION(uuid)              ((uuid->data[6] >> 4) & 0x0F)
#define UUID_VARIANT_IS_NCS(uuid)       (((uuid->data[8]) & 0x80) == 0x00)
#define UUID_VARIANT_IS_RFC4122(uuid)   (((uuid->data[8]) & 0xC0) == 0x80)
#define UUID_VARIANT_IS_GUID(uuid)      (((uuid->data[8]) & 0xE0) == 0xC0)
#define UUID_VARIANT_IS_FUTURE(uuid)    (((uuid->data[8]) & 0xE0) == 0xE0)

PG_MODULE_MAGIC;

static uint8 uuid_version_internal(const pg_uuid_t *uuid);
static uint8 uuid_variant_internal(const pg_uuid_t *uuid);
static bool uuid_is_rfc_v1_internal(const pg_uuid_t *uuid);
static int64 uuid_v1_timestamp0(const pg_uuid_t *uuid);
static TimestampTz uuid_v1_timestamp1(const pg_uuid_t *uuid);
static int uuid_ts_cmp0(const pg_uuid_t *a, const pg_uuid_t *b);
static int uuid_ts_only_cmp0(const pg_uuid_t *a, const TimestampTz b);

PG_FUNCTION_INFO_V1(uuid_v1_timestamp);
PG_FUNCTION_INFO_V1(uuid_version);
PG_FUNCTION_INFO_V1(uuid_variant);
PG_FUNCTION_INFO_V1(uuid_v1_node);

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

static uint8
uuid_version_internal(const pg_uuid_t *uuid)
{
    return UUID_VERSION(uuid);
}

static uint8
uuid_variant_internal(const pg_uuid_t *uuid)
{
    if (UUID_VARIANT_IS_RFC4122(uuid))
        return UUID_VARIANT_RFC4122;

    if (UUID_VARIANT_IS_GUID(uuid))
        return UUID_VARIANT_GUID;

    if (UUID_VARIANT_IS_NCS(uuid))
        return UUID_VARIANT_NCS;

    return UUID_VARIANT_FUTURE;
}

static bool
uuid_is_rfc_v1_internal(const pg_uuid_t *uuid)
{
    return 1 == UUID_VERSION(uuid) && UUID_VARIANT_IS_RFC4122(uuid);
}

static int64 uuid_v1_timestamp0(const pg_uuid_t *uuid)
{
    int64 timestamp = 0L;

    /* extract shuffled 60 bits to get the UUID timestamp */
    timestamp |= ( ((int64) uuid->data[6] & 0x0F) << 56 );
    timestamp |= ( ((int64) uuid->data[7]) << 48 );
    timestamp |= ( ((int64) uuid->data[4]) << 40 );
    timestamp |= ( ((int64) uuid->data[5]) << 32 );
    timestamp |= ( ((int64) uuid->data[0]) << 24 );
    timestamp |= ( ((int64) uuid->data[1]) << 16 );
    timestamp |= ( ((int64) uuid->data[2]) << 8 );
    timestamp |=   ((int64) uuid->data[3]);

    return timestamp;
}

static TimestampTz uuid_v1_timestamp1(const pg_uuid_t *uuid)
{
    TimestampTz timestamp = uuid_v1_timestamp0(uuid);

    /* from 100 ns precision to PostgreSQL epoch */
    timestamp -= UUID_TIME_OFFSET;
    timestamp /= 10;
    timestamp -= ((POSTGRES_EPOCH_JDATE - UNIX_EPOCH_JDATE) * SECS_PER_DAY * USECS_PER_SEC);

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
    TimestampTz         timestamp = 0L;
	pg_uuid_t           *uuid = PG_GETARG_UUID_P(0);

    if (PG_ARGISNULL(0))
        PG_RETURN_NULL();

    /* version and variant check */
    if (!uuid_is_rfc_v1_internal(uuid))
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

    PG_RETURN_TIMESTAMP(uuid_version_internal(uuid));
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

    PG_RETURN_TIMESTAMP(uuid_variant_internal(uuid));
}

/*
 * uuid_v1_node
 *	extract the node of a version 1 UUID (returns NULL otherwise)
 *
 */
Datum
uuid_v1_node(PG_FUNCTION_ARGS)
{
	pg_uuid_t        *uuid = PG_GETARG_UUID_P(0);
	static const char hex_chars[] = "0123456789abcdef";
	char*             node = malloc(13);
	int               i;
	int               j = 0;

    if (PG_ARGISNULL(0))
        PG_RETURN_NULL();

    /* version and variant check */
    if (!uuid_is_rfc_v1_internal(uuid))
        PG_RETURN_NULL();

	for (i = 10; i < UUID_LEN; i++)
	{
		int			hi;
		int			lo;

		hi = uuid->data[i] >> 4;
		lo = uuid->data[i] & 0x0F;

		node[j++] = hex_chars[hi];
		node[j++] = hex_chars[lo];
	}

    node[j++] = '\0';

	PG_RETURN_TEXT_P(cstring_to_text(node));
}

static int
uuid_ts_cmp0(const pg_uuid_t *a, const pg_uuid_t *b)
{
    int64 diff;

    diff = uuid_version_internal(a) - uuid_version_internal(b);
    if (diff < 0)
        return -1;
    else if (diff > 0)
        return 1;

    diff = uuid_v1_timestamp0(a) - uuid_v1_timestamp0(b);
    if (diff < 0)
        return -1;
    else if (diff > 0)
        return 1;

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
    if (!uuid_is_rfc_v1_internal(a))
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

