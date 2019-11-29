# make 		: debug build
# make debug 	: debug build
# make release	: release build
# make clean [debug, release] : clean
# make rebuild [debug, release] : rebuild
# make install : install

# DECKLINK_SDK_DIR = ../sdk

CC = g++
CFLAGS = -std=c++11 -Wno-multichar -fno-rtti -D__STDC_CONSTANT_MACROS -D_FILE_OFFSET_BITS=64 -DNO_FREETYPE -I../ffmpeg/include

CPPFLAGS = -std=c++11 -D__STDC_CONSTANT_MACROS -D_FILE_OFFSET_BITS=64 -I../ffmpeg/include 

LFLAGS = -lrt -lz -lm -ldl -lbz2 -lpthread -L/usr/local/lib -L../ffmpeg/lib -lavfilter -lpostproc -lswscale -lavformat -lavcodec -lavutil -lfdk-aac -lmp3lame -lopus -lvpx -lswresample -lx264 -lx265

CONFIG = release 

BUILD_DIR = ./Build/$(CONFIG)
INSTALL_DIR = /opt/tnmtech
TARGET = $(BUILD_DIR)/tmuxer

# find arg from input
ifneq "$(findstring clean, $(MAKECMDGOALS))" ""
	ARG.CLEAN = 1
endif

ifneq "$(findstring release, $(MAKECMDGOALS))" ""
	ARG.RELEASE = 1
endif

ifneq "$(findstring debug, $(MAKECMDGOALS))" ""
    ARG.DEBUG = 1
endif

ifneq "$(findstring rebuild, $(MAKECMDGOALS))" ""
    ARG.REBUILD = 1
endif

# select debug/release
ifeq ($(ARG.RELEASE), 1)
	CFLAGS += -O2 -DNDEBUG
	CPPFLAGS += -O2 -DNDEBUG
	CONFIG = release
else
	CFLAGS += -Wall -DDEBUG -g
	CPPFLAGS += -Wall -DDEBUG -g
	CONFIG = debug
endif

# actual make

.PHONY: debug release build clean rebuild install PRE_BUILD POST_BUILD all

BUILD_STEP = PRE_BUILD $(TARGET) POST_BUILD

ifeq ($(ARG.REBUILD),1)
    # rebuild인 경우,...
    # 빌드 이전에 clean을 수행한다.
    rebuild: | clean $(BUILD_STEP)
    debug: ; @true
    release: ; @true
else ifeq ($(ARG.CLEAN),1)
    # clean인 경우,...
    # clean target은 rule part에 정의되어 있다.
    release: ; @true
    debug: ; @true
else
    # clean/rebuild와 함께 쓰이지 않은 경우,...
    # 이곳에서 빌드가 이뤄진다.
    # release, debug는 단독 사용할 수 있어 @true하지 않는다.
    build: | $(BUILD_STEP)
    release: build
    debug: build
endif

# release, debug가 명령에 포함되어 조합되면,
# release, debug target을 찾게 되는데,
# 의도하지 않은
#    "make: Nothing to be done for 'release'"
#    "make: Nothing to be done for 'debug'"
# 를 방지하기 위해 @true를 사용하였다.

# ------------------------------
# Other macro
# ------------------------------
# .o를 .d로 바꿔준다.
DEPEND_FILE = $(patsubst %.o,%.d,$@)
BUILD_NUMBER_FILE=build-number.txt

GROUP.01.SRC = main.cpp\
			misc.cpp\
			core.cpp\
			recv.cpp\
			tsmuxer.cpp\
			tspes.cpp\
			videoglobal.cpp\
			jsoncpp.cpp
GROUP.01.OBJ = $(addprefix $(BUILD_DIR)/,$(GROUP.01.SRC:.cpp=.o))
GROUP.01.DEP = $(GROUP.01.OBJ:.o=.d)

# link part
$(TARGET): $(GROUP.01.OBJ) $(BUILD_NUMBER_FILE)
	@echo ----------------------------------------
	@echo Link : $(TARGET)
	@echo ----------------------------------------
	$(CC) -o $(TARGET) $(GROUP.01.OBJ) $(CFLAGS) $(LFLAGS) $(BUILD_NUMBER_LDFLAGS)
	@echo

# compile part
$(GROUP.01.OBJ): $(BUILD_DIR)/%.o: %.cpp
	@echo ----------------------------------------
	@echo Compile : [GROUP.01] $<
	@echo ----------------------------------------
	@test -d $(@D) || mkdir -p $(@D)
	$(CC) -MM -MF $(DEPEND_FILE) -MT"$(DEPEND_FILE:.d=.o)" $(CFLAGS) $<
	$(CC) $(CFLAGS) -c -o $@ $<
	@echo

#pre build
PRE_BUILD:
	@echo =======================================
	@echo Make started [config =\> $(CONFIG)]
	@echo =======================================
	@echo

#post build
POST_BUILD:
	@echo =======================================
	@echo Make finished [config =\> $(CONFIG)]
	@echo =======================================
	@echo

#clean
clean:
	rm -f $(GROUP.01.OBJ)
	rm -f $(GROUP.01.DEP)
	rm -f $(TARGET)
	@echo ---------------------------------------
	@echo Clean finished [config =\> $(CONFIG)]
	@echo ---------------------------------------

install:
	cp $(TARGET) $(INSTALL_DIR)/.

-include $(GROUP.01.DEP)
include buildnumber.mak
