SET timezone TO 'Zulu';
\x
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

SELECT
    uuid_timestamp_cmp('8385ded2-8dbb-11e9-ae2b-db6f0f573554', '8385ded2-8dbb-11e9-ae2b-db6f0f573554') AS eq,
    uuid_timestamp_cmp('8385ded2-8dbb-11e9-ae2b-db6f0f573554', '8385ded3-8dbb-11e9-ae2b-db6f0f573554') AS lt,
    uuid_timestamp_cmp('8385ded3-8dbb-11e9-ae2b-db6f0f573554', '8385ded2-8dbb-11e9-ae2b-db6f0f573554') AS gt,
    uuid_timestamp_cmp('8385ded2-8dbb-11e9-ae2b-db6f0f573554', '8385ded2-8dbb-11e9-ae2c-db6f0f573554') AS lt_clock,
    uuid_timestamp_cmp('8385ded2-8dbb-11e9-ae2c-db6f0f573554', '8385ded2-8dbb-11e9-ae2b-db6f0f573554') AS gt_clock,
    uuid_timestamp_cmp('8385ded2-8dbb-11e9-ae2b-db6f0f573554', '8385ded2-8dbb-11e9-ae2b-db6f0f573555') AS lt_node,
    uuid_timestamp_cmp('8385ded2-8dbb-11e9-ae2b-db6f0f573555', '8385ded2-8dbb-11e9-ae2b-db6f0f573554') AS gt_node
;

CREATE TABLE uuid_test (id uuid);
CREATE UNIQUE INDEX uidx_uuid_test_id ON uuid_test (id uuid_timestamp_ops);

INSERT INTO uuid_test (id) VALUES
('1004cd50-4241-11e9-b3ab-db6f0f573554'), -- 2019-03-09 07:58:02.056840
('05602550-8a8c-11e9-b3ab-db6f0f573554'), -- 2019-06-09 07:56:00.175240
('8385ded2-8dbb-11e9-ae2b-db6f0f573554'), -- 2019-06-13 09:13:31.650017
('ffc449f0-8c2f-11e9-aba7-e03f497ffcbf'), -- 2019-06-11 10:02:19.391640
('ffc449f0-8c2f-11e9-96b4-e03f49d7f7bb'), -- 2019-06-11 10:02:19.391640
('ffc449f0-8c2f-11e9-9bb8-e03f4977f7b7'), -- 2019-06-11 10:02:19.391640
('ffc449f0-8c2f-11e9-8f34-e03f49c7763b'), -- 2019-06-11 10:02:19.391640
('ffced5f0-8c2f-11e9-aba7-e03f497ffcbf'), -- 2019-06-11 10:02:19.460760
('ffd961f0-8c2f-11e9-96b4-e03f49d7f7bb'), -- 2019-06-11 10:02:19.529880
('ffe3edf0-8c2f-11e9-9bb8-e03f4977f7b7'), -- 2019-06-11 10:02:19.599000
('ffee79f0-8c2f-11e9-aba7-e03f497ffcbf'), -- 2019-06-11 10:02:19.668120
('fff905f0-8c2f-11e9-96b4-e03f49d7f7bb'), -- 2019-06-11 10:02:19.737240
('000391f0-8c30-11e9-aba7-e03f497ffcbf'), -- 2019-06-11 10:02:19.806360
('000e1df0-8c30-11e9-9bb8-e03f4977f7b7'), -- 2019-06-11 10:02:19.875480
('0018a9f0-8c30-11e9-96b4-e03f49d7f7bb'), -- 2019-06-11 10:02:19.944600
('002335f0-8c30-11e9-9bb8-e03f4977f7b7'), -- 2019-06-11 10:02:20.013720
('002dc1f0-8c30-11e9-aba7-e03f497ffcbf'), -- 2019-06-11 10:02:20.082840
('00384df0-8c30-11e9-96b4-e03f49d7f7bb'), -- 2019-06-11 10:02:20.151960
('0042d9f0-8c30-11e9-aba7-e03f497ffcbf'), -- 2019-06-11 10:02:20.221080
('004d65f0-8c30-11e9-9bb8-e03f4977f7b7')  -- 2019-06-11 10:02:20.290200
;

ANALYZE uuid_test;

SELECT
    count(*) FILTER (WHERE id <* '002335f0-8c30-11e9-9bb8-e03f4977f7b7') AS count_lt,
    count(*) FILTER (WHERE id <=* '002335f0-8c30-11e9-9bb8-e03f4977f7b7') AS count_le,
    count(*) FILTER (WHERE id >* '002335f0-8c30-11e9-9bb8-e03f4977f7b7') AS count_gt,
    count(*) FILTER (WHERE id >=* '002335f0-8c30-11e9-9bb8-e03f4977f7b7') AS count_ge
FROM uuid_test;

SELECT
    count(*) FILTER (WHERE id <~ '2019-06-11 10:02:20.013720') AS count_lt,
    count(*) FILTER (WHERE id <=~ '2019-06-11 10:02:20.013720') AS count_le,
    count(*) FILTER (WHERE id >~ '2019-06-11 10:02:20.013720') AS count_gt,
    count(*) FILTER (WHERE id >=~ '2019-06-11 10:02:20.013720') AS count_ge
FROM uuid_test;

SET enable_seqscan TO off;
\x
SET timezone TO 'Asia/Tokyo';
EXPLAIN (ANALYZE, TIMING OFF, SUMMARY OFF, COSTS OFF)
SELECT count(*) FROM uuid_test WHERE id <~ '2019-06-11 10:02:19Z';

EXPLAIN (ANALYZE, TIMING OFF, SUMMARY OFF, COSTS OFF)
SELECT * FROM uuid_test WHERE id = '000e1df0-8c30-11e9-9bb8-e03f4977f7b7';
