
# Extended functions for UUID's

This extension for PostgreSQL provides some useful functions that aim at
speeding up various tasks especially with version 1 UUID's implemented in
compliance to [RFC 4122][1].

## uuid_version

The function `uuid_version(uuid)` returns the version field of the UUID as
specified in [RFC 4122][3], e.g.:

```sql
SELECT uuid_version('c9aec822-6992-5c93-b34a-33cc0e952b5e');
 uuid_version
--------------
            5
(1 row)
```

## uuid_variant

The function `uuid_variant(uuid)` returns the UUID variant as specified in
[RFC 4122][4] with the following values:

- `0` = NCS backward compatibility
- `1` = RFC 4122
- `2` = Microsoft Corporation backwards compatibility
- `3` = Reserved for future definition

Example for a rather ancient [CLSID][5]:

```sql
SELECT uuid_variant('{000C1090-0000-0000-C000-000000000046}');
 uuid_variant
--------------
            2
(1 row)
```

## uuid_v1_timestamp

The function `uuid_v1_timestamp(uuid)` extracts the timestamp of a version 1
UUID that was generated according to [RFC 4122][1] into an instance of the
PostgreSQL type [`timestamp with time zone`][2], e.g.:

```sql
SET timezone TO 'Asia/Tokyo';
SELECT uuid_v1_timestamp('b647e96b-862d-11e9-ae2b-db6f0f573554');
       uuid_v1_timestamp
-------------------------------
 2019-06-04 03:30:50.132721+09
(1 row)
```

The function ensures that only a UUID which actually contains a readable
timestamp value is actually parsed. In case a value other than an [RFC 4122][1]
version 1 UUID is provided, the function will simply return `NULL`, e.g. for
a version 4 (random) UUID:

```sql
SELECT uuid_v1_timestamp('22859369-3a4f-49ef-8264-1aaf0a953299') IS NULL AS is_null;
 is_null
---------
 t
(1 row)
```

## uuid_v1_node

The function `uuid_v1_node(uuid)` returns the [node][7] of a version 1 UUID,
e.g.:

```sql
SELECT uuid_v1_node('b647e96b-862d-11e9-ae2b-db6f0f573554');
 uuid_v1_node 
--------------
 db6f0f573554
(1 row)
```


# Build

Straight forward but please ensure that you have the necessary PostgreSQL
development headers in-place as well as [PGXS][6] (which should be made
available with installing the development package).

```
make
```

# Executing Tests

Some basic tests are included by making use of `pg_regress` which can be run with:

```
make installcheck
```

You might need to create a role with super-user privileges with the same name as
your local user or you re-use an existing one, e.g.:

```
sudo -u postgres make installcheck
```

If your _default_ PostgreSQL installation doesn't listen on standard port 5432,
you can adapt it by specifying `REGRESS_PORT` variable, e.g.:

```
sudo -u postgres make REGRESS_PORT=5433 installcheck
```


# Installation

This also requires [PGXS][6] as it figures out where to find the installation:

```
sudo make install
```

If you want to install it into a non-default PostgreSQL installation, just
specify the path to the respective `pg_config` binary, e.g.:

```
sudo make PG_CONFIG=/usr/lib/postgresql/10/bin/pg_config install
```


[1]: https://tools.ietf.org/html/rfc4122
[2]: https://www.postgresql.org/docs/current/datatype-datetime.html
[3]: https://tools.ietf.org/html/rfc4122#section-4.1.3
[4]: https://tools.ietf.org/html/rfc4122#section-4.1.1
[5]: https://docs.microsoft.com/en-us/windows/desktop/com/clsid-key-hklm
[6]: https://www.postgresql.org/docs/current/extend-pgxs.html
[7]: https://tools.ietf.org/html/rfc4122#section-4.1.6
