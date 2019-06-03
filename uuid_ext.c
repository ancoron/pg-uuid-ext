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

PG_FUNCTION_INFO_V1(uuid_to_timestamp);
PG_FUNCTION_INFO_V1(uuid_version);
PG_FUNCTION_INFO_V1(uuid_variant);

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
uuid_to_timestamp(PG_FUNCTION_ARGS)
{
    int64               msb = 0L;
    float8              seconds;
    TimestampTz         result;
	pg_uuid_t           *uuid = PG_GETARG_UUID_P(0);

    if (PG_ARGISNULL(0))
        PG_RETURN_NULL();

    /* version and variant check */
    if (!uuid_is_rfc_v1_internal(uuid))
        PG_RETURN_NULL();

    /* extract shuffled 60 bits to get the UUID timestamp */
    msb |= ( ((int64) uuid->data[6] & 0x0F) << 56 );
    msb |= ( ((int64) uuid->data[7]) << 48 );
    msb |= ( ((int64) uuid->data[4]) << 40 );
    msb |= ( ((int64) uuid->data[5]) << 32 );
    msb |= ( ((int64) uuid->data[0]) << 24 );
    msb |= ( ((int64) uuid->data[1]) << 16 );
    msb |= ( ((int64) uuid->data[2]) << 8 );
    msb |= ((int64) uuid->data[3]);

    /* from 100 ns precision to PostgreSQL epoch */
    seconds = (msb - UUID_TIME_OFFSET);
    seconds /= 10000000;
    seconds -= ((POSTGRES_EPOCH_JDATE - UNIX_EPOCH_JDATE) * SECS_PER_DAY);
    seconds = rint(seconds * USECS_PER_SEC);
    result = (int64) seconds;

    /* Recheck in case roundoff produces something just out of range */
    if (!IS_VALID_TIMESTAMP(result))
        ereport(ERROR,
                (errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
                 errmsg("timestamp out of range: \"%g\"", seconds)));

	PG_RETURN_TIMESTAMP(result);
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
