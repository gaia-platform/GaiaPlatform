#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

#
# Helper script to build up DDL files  by concatenating
# them together.  Generates:
# ${DDL_OUTPUT_PRFIX}_db.ddl
#
# Note that cmake 3.18 has a 'cat' command but for now
# we do it ourselves because we are on 3.17
#
function (gaia_cat IN_FILE OUT_FILE)
  file(READ ${IN_FILE} CONTENTS)
  file(APPEND ${OUT_FILE} "${CONTENTS}")
endfunction()

file(WRITE "${OUTPUT_DDL_PREFIX}_db.ddl" "")

# Include rules engine type definitions.
gaia_cat(${EVENT_LOG_DDL} ${OUTPUT_DDL_PREFIX}_db.ddl)

# Now add the user's schema.
gaia_cat(${GAIA_USER_SCHEMA} ${OUTPUT_DDL_PREFIX}_db.ddl)
