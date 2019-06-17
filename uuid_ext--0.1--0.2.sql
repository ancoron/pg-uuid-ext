/*
 * Copyright 2019 Ancoron Luciferis <ancoron.luciferis@gmail.com>.
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

-- complain if script is sourced in psql, rather than via ALTER EXTENSION
\echo Use "ALTER EXTENSION uuid_ext UPDATE TO '0.2'" to load this file. \quit

-- equal
CREATE FUNCTION uuid_timestamp_eq(uuid, uuid)
RETURNS bool
AS 'MODULE_PATHNAME', 'uuid_timestamp_eq'
LANGUAGE C
IMMUTABLE
LEAKPROOF
STRICT
PARALLEL SAFE;

COMMENT ON FUNCTION uuid_timestamp_eq(uuid, uuid) IS 'equal to';

CREATE OPERATOR =* (
    LEFTARG = uuid,
    RIGHTARG = uuid,
    PROCEDURE = uuid_timestamp_eq,
    COMMUTATOR = '=*',
    NEGATOR = '<>*',
    RESTRICT = eqsel,
    JOIN = eqjoinsel,
    MERGES
);

CREATE FUNCTION uuid_timestamp_only_eq(uuid, timestamp with time zone)
RETURNS bool
AS 'MODULE_PATHNAME', 'uuid_timestamp_only_eq'
LANGUAGE C
IMMUTABLE
LEAKPROOF
STRICT
PARALLEL SAFE;

COMMENT ON FUNCTION uuid_timestamp_only_eq(uuid, timestamp with time zone) IS 'equal to';

CREATE OPERATOR =~ (
    LEFTARG = uuid,
    RIGHTARG = timestamp with time zone,
    PROCEDURE = uuid_timestamp_only_eq,
    COMMUTATOR = '=~',
    NEGATOR = '<>~',
    RESTRICT = eqsel,
    JOIN = eqjoinsel,
    MERGES
);

-- not equal
CREATE FUNCTION uuid_timestamp_ne(uuid, uuid)
RETURNS bool
AS 'MODULE_PATHNAME', 'uuid_timestamp_ne'
LANGUAGE C
IMMUTABLE
LEAKPROOF
STRICT
PARALLEL SAFE;

COMMENT ON FUNCTION uuid_timestamp_ne(uuid, uuid) IS 'not equal to';

CREATE OPERATOR <>* (
	LEFTARG = uuid,
    RIGHTARG = uuid,
    PROCEDURE = uuid_timestamp_ne,
    COMMUTATOR = '<>*',
    NEGATOR = '=*',
    RESTRICT = neqsel,
    JOIN = neqjoinsel
);

CREATE FUNCTION uuid_timestamp_only_ne(uuid, timestamp with time zone)
RETURNS bool
AS 'MODULE_PATHNAME', 'uuid_timestamp_only_ne'
LANGUAGE C
IMMUTABLE
LEAKPROOF
STRICT
PARALLEL SAFE;

COMMENT ON FUNCTION uuid_timestamp_only_ne(uuid, timestamp with time zone) IS 'not equal to';

CREATE OPERATOR <>~ (
	LEFTARG = uuid,
    RIGHTARG = timestamp with time zone,
    PROCEDURE = uuid_timestamp_only_ne,
    COMMUTATOR = '<>~',
    NEGATOR = '=~',
    RESTRICT = neqsel,
    JOIN = neqjoinsel
);

-- lower than
CREATE FUNCTION uuid_timestamp_lt(uuid, uuid)
RETURNS bool
AS 'MODULE_PATHNAME', 'uuid_timestamp_lt'
LANGUAGE C
IMMUTABLE
LEAKPROOF
STRICT
PARALLEL SAFE;

COMMENT ON FUNCTION uuid_timestamp_lt(uuid, uuid) IS 'lower than';

CREATE OPERATOR <* (
	LEFTARG = uuid,
    RIGHTARG = uuid,
    PROCEDURE = uuid_timestamp_lt,
	COMMUTATOR = '>*',
    NEGATOR = '>=*',
	RESTRICT = scalarltsel,
    JOIN = scalarltjoinsel
);

CREATE FUNCTION uuid_timestamp_only_lt(uuid, timestamp with time zone)
RETURNS bool
AS 'MODULE_PATHNAME', 'uuid_timestamp_only_lt'
LANGUAGE C
IMMUTABLE
LEAKPROOF
STRICT
PARALLEL SAFE;

COMMENT ON FUNCTION uuid_timestamp_only_lt(uuid, timestamp with time zone) IS 'lower than';

CREATE OPERATOR <~ (
	LEFTARG = uuid,
    RIGHTARG = timestamp with time zone,
    PROCEDURE = uuid_timestamp_only_lt,
	COMMUTATOR = '>~',
    NEGATOR = '>=~',
	RESTRICT = scalarltsel,
    JOIN = scalarltjoinsel
);

-- greater than
CREATE FUNCTION uuid_timestamp_gt(uuid, uuid)
RETURNS bool
AS 'MODULE_PATHNAME', 'uuid_timestamp_gt'
LANGUAGE C
IMMUTABLE
LEAKPROOF
STRICT
PARALLEL SAFE;

COMMENT ON FUNCTION uuid_timestamp_gt(uuid, uuid) IS 'greater than';

CREATE OPERATOR >* (
	LEFTARG = uuid,
    RIGHTARG = uuid,
    PROCEDURE = uuid_timestamp_gt,
	COMMUTATOR = '<*',
    NEGATOR = '<=*',
	RESTRICT = scalargtsel,
    JOIN = scalargtjoinsel
);

CREATE FUNCTION uuid_timestamp_only_gt(uuid, timestamp with time zone)
RETURNS bool
AS 'MODULE_PATHNAME', 'uuid_timestamp_only_gt'
LANGUAGE C
IMMUTABLE
LEAKPROOF
STRICT
PARALLEL SAFE;

COMMENT ON FUNCTION uuid_timestamp_only_gt(uuid, timestamp with time zone) IS 'greater than';

CREATE OPERATOR >~ (
	LEFTARG = uuid,
    RIGHTARG = timestamp with time zone,
    PROCEDURE = uuid_timestamp_only_gt,
	COMMUTATOR = '<~',
    NEGATOR = '<=~',
	RESTRICT = scalargtsel,
    JOIN = scalargtjoinsel
);

-- lower than or equal
CREATE FUNCTION uuid_timestamp_le(uuid, uuid)
RETURNS bool
AS 'MODULE_PATHNAME', 'uuid_timestamp_le'
LANGUAGE C
IMMUTABLE
LEAKPROOF
STRICT
PARALLEL SAFE;

COMMENT ON FUNCTION uuid_timestamp_le(uuid, uuid) IS 'lower than or equal to';

CREATE OPERATOR <=* (
	LEFTARG = uuid,
    RIGHTARG = uuid,
    PROCEDURE = uuid_timestamp_le,
	COMMUTATOR = '>=*',
    NEGATOR = '>*',
	RESTRICT = scalarltsel,
    JOIN = scalarltjoinsel
);

CREATE FUNCTION uuid_timestamp_only_le(uuid, timestamp with time zone)
RETURNS bool
AS 'MODULE_PATHNAME', 'uuid_timestamp_only_le'
LANGUAGE C
IMMUTABLE
LEAKPROOF
STRICT
PARALLEL SAFE;

COMMENT ON FUNCTION uuid_timestamp_only_le(uuid, timestamp with time zone) IS 'lower than or equal to';

CREATE OPERATOR <=~ (
	LEFTARG = uuid,
    RIGHTARG = timestamp with time zone,
    PROCEDURE = uuid_timestamp_only_le,
	COMMUTATOR = '>=~',
    NEGATOR = '>~',
	RESTRICT = scalarltsel,
    JOIN = scalarltjoinsel
);

-- greater than or equal
CREATE FUNCTION uuid_timestamp_ge(uuid, uuid)
RETURNS bool
AS 'MODULE_PATHNAME', 'uuid_timestamp_ge'
LANGUAGE C
IMMUTABLE
LEAKPROOF
STRICT
PARALLEL SAFE;

COMMENT ON FUNCTION uuid_timestamp_ge(uuid, uuid) IS 'greater than or equal to';

CREATE OPERATOR >=* (
	LEFTARG = uuid,
    RIGHTARG = uuid,
    PROCEDURE = uuid_timestamp_ge,
	COMMUTATOR = '<=*',
    NEGATOR = '<*',
	RESTRICT = scalargtsel,
    JOIN = scalargtjoinsel
);

CREATE FUNCTION uuid_timestamp_only_ge(uuid, timestamp with time zone)
RETURNS bool
AS 'MODULE_PATHNAME', 'uuid_timestamp_only_ge'
LANGUAGE C
IMMUTABLE
LEAKPROOF
STRICT
PARALLEL SAFE;

COMMENT ON FUNCTION uuid_timestamp_only_ge(uuid, timestamp with time zone) IS 'greater than or equal to';

CREATE OPERATOR >=~ (
	LEFTARG = uuid,
    RIGHTARG = timestamp with time zone,
    PROCEDURE = uuid_timestamp_only_ge,
	COMMUTATOR = '<=~',
    NEGATOR = '<~',
	RESTRICT = scalargtsel,
    JOIN = scalargtjoinsel
);

-- generic comparison function
CREATE FUNCTION uuid_timestamp_cmp(uuid, uuid)
RETURNS int4
AS 'MODULE_PATHNAME', 'uuid_timestamp_cmp'
LANGUAGE C
IMMUTABLE
LEAKPROOF
STRICT
PARALLEL SAFE;

COMMENT ON FUNCTION uuid_timestamp_cmp(uuid, uuid) IS 'UUID v1 comparison function for timestamps';

CREATE FUNCTION uuid_timestamp_only_cmp(uuid, timestamp with time zone)
RETURNS int4
AS 'MODULE_PATHNAME', 'uuid_timestamp_only_cmp'
LANGUAGE C
IMMUTABLE
LEAKPROOF
STRICT
PARALLEL SAFE;

COMMENT ON FUNCTION uuid_timestamp_only_cmp(uuid, timestamp with time zone) IS 'UUID v1 comparison function for timestamps';

CREATE OPERATOR CLASS uuid_timestamp_ops FOR TYPE uuid
    USING btree AS
        OPERATOR        1       <*,
        OPERATOR        1       <~ (uuid, timestamp with time zone),
        OPERATOR        2       <=*,
        OPERATOR        2       <=~ (uuid, timestamp with time zone),
        OPERATOR        3       =*,
        OPERATOR        3       =~ (uuid, timestamp with time zone),
        OPERATOR        4       >=*,
        OPERATOR        4       >=~ (uuid, timestamp with time zone),
        OPERATOR        5       >*,
        OPERATOR        5       >~ (uuid, timestamp with time zone),
        FUNCTION        1       uuid_timestamp_cmp(uuid, uuid),
        FUNCTION        1       uuid_timestamp_only_cmp(uuid, timestamp with time zone)
;
