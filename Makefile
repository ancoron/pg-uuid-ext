#
# Makefile for extension 'uuid_ext'
#
# See: https://www.postgresql.org/docs/current/extend-extensions.html

MODULE_big = uuid_ext
OBJS = uuid_ext.o

# Define name of the extension
EXTENSION = uuid_ext

# Which SQL to execution at "CREATE EXTENSION ..."
DATA = uuid_ext--0.1.sql uuid_ext--0.1--0.2.sql uuid_ext--0.2--0.3.sql

# pg_regress settings
REGRESS_PORT := 5432
REGRESS = $(EXTENSION)
REGRESS_OPTS = --load-extension=$(EXTENSION) --port=$(REGRESS_PORT)

# Use PGXS for installation
# See: https://www.postgresql.org/docs/current/extend-pgxs.html
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
