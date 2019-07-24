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
 *
 *      uuid_ext.h
 */

#ifndef UUID_EXT_H
#define UUID_EXT_H

/*
 * The time offset between the UUID timestamp and the PostgreSQL epoch in
 * microsecond precision.
 *
 * This constant is the result of the following expression:
 * `122192928000000000 / 10 + ((POSTGRES_EPOCH_JDATE - UNIX_EPOCH_JDATE) * SECS_PER_DAY * USECS_PER_SEC)`
 */
#define PG_UUID_OFFSET              INT64CONST(13165977600000000)

#define UUID_VARIANT_NCS            (0x00)
#define UUID_VARIANT_RFC4122        (0x01)
#define UUID_VARIANT_GUID           (0x02)
#define UUID_VARIANT_FUTURE         (0x03)

#define UUID_VERSION(uuid)              ((uuid->data[6] >> 4) & 0x0F)
#define UUID_VARIANT_IS_NCS(uuid)       (((uuid->data[8]) & 0x80) == 0x00)
#define UUID_VARIANT_IS_RFC4122(uuid)   (((uuid->data[8]) & 0xC0) == 0x80)
#define UUID_VARIANT_IS_GUID(uuid)      (((uuid->data[8]) & 0xE0) == 0xC0)
#define UUID_VARIANT_IS_FUTURE(uuid)    (((uuid->data[8]) & 0xE0) == 0xE0)

extern uint8 uuid_get_version(const pg_uuid_t *uuid);
extern uint8 uuid_get_variant(const pg_uuid_t *uuid);
extern bool uuid_is_rfc_v1(const pg_uuid_t *uuid);

#endif /* UUID_EXT_H */

