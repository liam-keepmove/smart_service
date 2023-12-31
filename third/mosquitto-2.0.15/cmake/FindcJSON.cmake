INCLUDE( FindPackageHandleStandardArgs )

# Checks an environment variable; note that the first check
# does not require the usual CMake $-sign.
IF( DEFINED ENV{CJSON_DIR} )
	SET( CJSON_DIR "$ENV{CJSON_DIR}" )
ENDIF()

FIND_PATH(
		CJSON_INCLUDE_DIR
		cjson/cJSON.h
	HINTS
		CJSON_DIR
)

FIND_LIBRARY( CJSON_LIBRARY
	NAMES cjson
	HINTS ${CJSON_DIR}
)

FIND_PACKAGE_HANDLE_STANDARD_ARGS( cJSON DEFAULT_MSG
	CJSON_INCLUDE_DIR CJSON_LIBRARY
)

IF( CJSON_FOUND )
	SET( CJSON_INCLUDE_DIRS ${CJSON_INCLUDE_DIR} )
	SET( CJSON_LIBRARIES ${CJSON_LIBRARY} )

	MARK_AS_ADVANCED(
		CJSON_LIBRARY
		CJSON_INCLUDE_DIR
		CJSON_DIR
	)
ELSE()
	SET( CJSON_DIR "" CACHE STRING
		"An optional hint to a directory for finding `cJSON`"
	)
ENDIF()
