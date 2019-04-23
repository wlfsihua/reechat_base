AUTOMAKE_OPTIONS = foreign
lib_LIBRARIES = libwebrtc_base.a

AM_CPPFLAGS = -I../
AM_CPPFLAGS += -DWEBRTC_POSIX -DWEBRTC_LINUX -DWEBRTC_BUILD_LIBEVENT -D__STDC_FORMAT_MACROS -std=gnu++11

libwebrtc_base_a_SOURCES = rtc_base/logging.cc\
rtc_base/logsinks.cc\
rtc_base/criticalsection.cc\
rtc_base/thread.cc\
rtc_base/checks.cc\
rtc_base/platform_thread.cc\
rtc_base/thread_checker_impl.cc\
rtc_base/event.cc\
rtc_base/event_tracer.cc\
rtc_base/timeutils.cc\
rtc_base/stringencode.cc\
rtc_base/copyonwritebuffer.cc\
rtc_base/stream.cc\
rtc_base/messagequeue.cc\
rtc_base/location.cc\
rtc_base/messagehandler.cc\
rtc_base/nullsocketserver.cc\
rtc_base/physicalsocketserver.cc\
rtc_base/ipaddress.cc\
rtc_base/socketaddress.cc\
rtc_base/nethelpers.cc\
rtc_base/task_queue_posix.cc\
rtc_base/asyncsocket.cc\
rtc_base/asyncresolverinterface.cc\
rtc_base/task_queue_libevent.cc
