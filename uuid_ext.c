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
#include "fmgr.h"
#include "port.h"
#include "datatype/timestamp.h"
#include "lib/stringinfo.h"
#include "utils/builtins.h"
#include "utils/uuid.h"
#include "utils/timestamp.h"

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

static uint8 uuid_version_internal(pg_uuid_t *uuid);
static uint8 uuid_variant_internal(pg_uuid_t *uuid);
static bool uuid_is_rfc_v1_internal(pg_uuid_t *uuid);

PG_FUNCTION_INFO_V1(uuid_v1_timestamp);
PG_FUNCTION_INFO_V1(uuid_version);
PG_FUNCTION_INFO_V1(uuid_variant);
PG_FUNCTION_INFO_V1(uuid_v1_node);

static uint8
uuid_version_internal(pg_uuid_t *uuid)
{
    return UUID_VERSION(uuid);
}

static uint8
uuid_variant_internal(pg_uuid_t *uuid)
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
uuid_is_rfc_v1_internal(pg_uuid_t *uuid)
{
    return 1 == UUID_VERSION(uuid) && UUID_VARIANT_IS_RFC4122(uuid);
}

/*
 * uuid_to_timestamp
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

    /* extract shuffled 60 bits to get the UUID timestamp */
    timestamp |= ( ((int64) uuid->data[6] & 0x0F) << 56 );
    timestamp |= ( ((int64) uuid->data[7]) << 48 );
    timestamp |= ( ((int64) uuid->data[4]) << 40 );
    timestamp |= ( ((int64) uuid->data[5]) << 32 );
    timestamp |= ( ((int64) uuid->data[0]) << 24 );
    timestamp |= ( ((int64) uuid->data[1]) << 16 );
    timestamp |= ( ((int64) uuid->data[2]) << 8 );
    timestamp |= ( (int64) uuid->data[3] );

    /* from 100 ns precision to PostgreSQL epoch */
    timestamp -= UUID_TIME_OFFSET;
    timestamp /= 10;
    timestamp -= ((POSTGRES_EPOCH_JDATE - UNIX_EPOCH_JDATE) * SECS_PER_DAY * USECS_PER_SEC);

    /* Recheck in case roundoff produces something just out of range */
    if (!IS_VALID_TIMESTAMP(timestamp))
        ereport(ERROR,
                (errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
                 errmsg("timestamp out of range: \"%ld\"", timestamp)));

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
