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

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION uuid_ext" to load this file. \quit

CREATE FUNCTION uuid_to_timestamp(uuid) RETURNS timestamp with time zone
AS 'MODULE_PATHNAME', 'uuid_to_timestamp'
LANGUAGE C STRICT PARALLEL SAFE;

CREATE FUNCTION uuid_version(uuid) RETURNS integer
AS 'MODULE_PATHNAME', 'uuid_version'
LANGUAGE C STRICT PARALLEL SAFE;

CREATE FUNCTION uuid_variant(uuid) RETURNS integer
AS 'MODULE_PATHNAME', 'uuid_variant'
LANGUAGE C STRICT PARALLEL SAFE;
