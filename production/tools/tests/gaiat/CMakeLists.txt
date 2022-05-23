###################################################
# Copyright (c) Gaia Platform LLC
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

# Translation Engine tests.  These tests invoke gaiat on different rulesets
# to examine multiple corner case scenarios that can't be covered in unittests

set(GAIAT_TESTS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(GAIAT_TESTS_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(GAIAT_TESTS_EXECUTABLE_BINARY_DIR "${GAIA_PROD_BUILD}/tools/gaia_translate/")

configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.py
  MAIN_CONFIG
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg.py
  )

add_lit_testsuite(check-gaiat "Running Translation Engine integration tests"
  ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS gaiat
  )
set_target_properties(check-gaiat PROPERTIES FOLDER "Translation Engine integration tests")
