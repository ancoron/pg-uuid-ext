SET timezone TO 'Zulu';

SELECT extract(epoch from ts), to_char(ts, 'YYYY-MM-DD HH24:MI:SS.US')
FROM uuid_v1_timestamp('edb4d8f0-1a80-11e8-98d9-e03f49f7f8f3') AS ts;

SELECT extract(epoch from ts), to_char(ts, 'YYYY-MM-DD HH24:MI:SS.US')
FROM uuid_v1_timestamp('4938f30e-8449-11e9-ae2b-e03f49467033') AS ts;

SELECT
    coalesce(to_char(uuid_v1_timestamp('87c771ce-bc95-3114-ae59-c0e26acf8e81'), 'YYYY-MM-DD HH24:MI:SS.US'), '<null>') AS ver_3,
    coalesce(to_char(uuid_v1_timestamp('22859369-3a4f-49ef-8264-1aaf0a953299'), 'YYYY-MM-DD HH24:MI:SS.US'), '<null>') AS ver_4,
    coalesce(to_char(uuid_v1_timestamp('c9aec822-6992-5c93-b34a-33cc0e952b5e'), 'YYYY-MM-DD HH24:MI:SS.US'), '<null>') AS ver_5,
    coalesce(to_char(uuid_v1_timestamp('00000000-0000-0000-0000-000000000000'), 'YYYY-MM-DD HH24:MI:SS.US'), '<null>') AS nil
;

SELECT
    uuid_version('00000000-0000-0000-0000-000000000000') AS ver_0,
    uuid_version('edb4d8f0-1a80-11e8-98d9-e03f49f7f8f3') AS ver_1,
    uuid_version('87c771ce-bc95-3114-ae59-c0e26acf8e81') AS ver_3,
    uuid_version('314dffb7-03ce-442c-b1dd-c1a9b3919282') AS ver_4,
    uuid_version('c9aec822-6992-5c93-b34a-33cc0e952b5e') AS ver_5
;

SELECT
    uuid_variant('00000000-0000-0000-0000-000000000000') AS var_0,
    uuid_variant('edb4d8f0-1a80-11e8-98d9-e03f49f7f8f3') AS var_1,
    uuid_variant('{000C1090-0000-0000-C000-000000000046}') AS var_2,
    uuid_variant('C9AEC82269925C93E34A33CC0E952B5E') AS var_3
;

SELECT
    coalesce(uuid_v1_node('b647e96b-862d-11e9-ae2b-db6f0f573554'), '<null>') AS ver_1,
    coalesce(uuid_v1_node('87c771ce-bc95-3114-ae59-c0e26acf8e81'), '<null>') AS ver_3,
    coalesce(uuid_v1_node('22859369-3a4f-49ef-8264-1aaf0a953299'), '<null>') AS ver_4,
    coalesce(uuid_v1_node('c9aec822-6992-5c93-b34a-33cc0e952b5e'), '<null>') AS ver_5,
    coalesce(uuid_v1_node('00000000-0000-0000-0000-000000000000'), '<null>') AS nil
;
