#Makefile.am

AUTOMAKE_OPTIONS      = gnu
bin_PROGRAMS          = spider testclient
spider_SOURCES        = include/def.h include/threadpool.h include/common.h src/threadpool.c src/spider.c
spider_CPPFLAGS       = -I include/ -Wall
testclient_SOURCES    = test/testclient.c include/def.h include/threadpool.h include/common.h src/threadpool.c
testclient_CPPFLAGS   = -I include/ -Wall
