bin_PROGRAMS = fatcache stg_ins_test

AM_CPPFLAGS = -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS = -g 
CXXFLAGS = -fPIC -fpermissive #--std=c++98
AM_CFLAGS = -Wall
AM_CFLAGS += -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls
AM_CFLAGS += -fno-strict-aliasing
AM_CFLAGS += -Wunused-function -Wunused-value -Wunused-variable

LDFLAGS += -rdynamic

fatcache_SOURCES =			\
	fc_core.cc fc_core.h		\
	fc_connection.cc fc_connection.h	\
	fc_server.cc fc_server.h		\
	fc_client.cc fc_client.h		\
	fc_slab.cc fc_slab.h		\
	fc_item.cc fc_item.h		\
	fc_itemx.cc fc_itemx.h		\
	fc_memcache.cc fc_memcache.h	\
	fc_message.cc fc_message.h	\
	fc_request.cc			\
	fc_response.cc			\
	fc_mbuf.cc fc_mbuf.h		\
	fc_signal.cc fc_signal.h		\
	fc_event.cc fc_event.h		\
	fc_time.cc fc_time.h		\
	fc_sha1.cc fc_sha1.h		\
	fc_log.cc fc_log.h		\
	fc_string.cc fc_string.h		\
	fc_array.cc fc_array.h		\
	fc_util.cc fc_util.h		\
	fc_stats.cc fc_stats.h		\
	fc_queue.h			\
	fc.cc

stg_ins_test_SOURCES =			\
	fc_slab.cc fc_slab.h		\
	fc_item.cc fc_item.h		\
	fc_itemx.cc fc_itemx.h		\
	fc_time.cc fc_time.h		\
	fc_sha1.cc fc_sha1.h		\
	fc_log.cc fc_log.h		\
	fc_util.cc fc_util.h		\
	fc_queue.h			\
    stg_ins_test.cc
