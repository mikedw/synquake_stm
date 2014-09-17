####################################################################################################
##
## CONFIGURATION
##
####################################################################################################

## Compilation parameters
CC = icc
CXX = icc
MACHINE = `uname -n`

ifeq ($(DEBUG), )
	DEBUG = -O3
endif

# Atomic transaction
#QTM_ENABLED=-Qtm_enabled
#INTEL_TM=$(QTM_ENABLED) -DINTEL_TM
#TRANSATION_TYPE="atomic"
#ALLOCATOR_TRANSACTION="transaction_pure"
#TRANSACTION_ANNOTATION="transaction_safe"

# Relaxed transactions
#QTM_ENABLED=-Qtm_enabled
#INTEL_TM=$(QTM_ENABLED) -DINTEL_TM
#TRANSATION_TYPE="relaxed"
#TRANSACTION_ANNOTATION="transaction_callable"
#ALLOCATOR_TRANSACTION="transaction_callable"

# Use the Hoard library
#HOARD=-L./ -lhoard

CXXFLAGS := $(INTEL_TM) -D$(MACHINE) -Wall -DTRANSATION_TYPE=$(TRANSATION_TYPE) -DTRANSACTION_ANNOTATION=$(TRANSACTION_ANNOTATION) -DALLOCATOR_TRANSACTION=$(ALLOCATOR_TRANSACTION) $(DEBUG) -lpthread
LDFLAGS := $(QTM_ENABLED) -lpthread $(HOARD) $(DEBUG) #-lm
LDFLAGS_SERVER = $(LDFLAGS)
LDFLAGS_CLIENT = $(LDFLAGS) -lGL -lGLU `sdl-config --libs` -lSDL_net

## Source and binary files
BIN_DIR = build
SRC_DIR = src

UTILS_DIR    = $(SRC_DIR)/utils
COMM_DIR     = $(SRC_DIR)/comm
GAME_DIR     = $(SRC_DIR)/game
GRAPH_DIR    = $(SRC_DIR)/graphics

SERVER_DIR   = $(SRC_DIR)/server
CLIENT_DIR   = $(SRC_DIR)/client

SHARED_SRCS  = $(wildcard $(UTILS_DIR)/*.c)	$(wildcard $(GAME_DIR)/*.c)
COMM_SRCS    = $(wildcard $(COMM_DIR)/*.c)
GRAPH_SRCS   = $(wildcard $(GRAPH_DIR)/*.cpp)	$(wildcard $(GRAPH_DIR)/3ds/*.cpp)	$(wildcard $(GRAPH_DIR)/font/*.cpp) $(wildcard $(GRAPH_DIR)/texture/*.cpp) $(wildcard $(GRAPH_DIR)/vrml/*.cpp)
SERVER_SRCS  = $(wildcard $(SERVER_DIR)/*.c)	$(wildcard $(SERVER_DIR)/sgame/*.c)	$(wildcard $(SERVER_DIR)/loadbal/*.c)
CLIENT_SRCS  = $(wildcard $(CLIENT_DIR)/*.cpp)	$(wildcard $(CLIENT_DIR)/AStar/*.cpp)
CLIENT_SRCS2 = $(wildcard $(CLIENT_DIR)/cgame/*.c)

SHARED_OBJS   = $(patsubst $(SRC_DIR)/%.c,$(BIN_DIR)/%.o,$(SHARED_SRCS))
COMM_OBJS     = $(patsubst $(SRC_DIR)/%.c,$(BIN_DIR)/%.o,$(COMM_SRCS))
GRAPH_OBJS    = $(patsubst $(SRC_DIR)/%.cpp,$(BIN_DIR)/%.o,$(GRAPH_SRCS))

SERVER_OBJS   = $(patsubst $(SRC_DIR)/%.c,$(BIN_DIR)/%.o,$(SERVER_SRCS))   $(SHARED_OBJS) $(COMM_OBJS)
CLIENT_OBJS   = $(patsubst $(SRC_DIR)/%.cpp,$(BIN_DIR)/%.o,$(CLIENT_SRCS)) $(SHARED_OBJS) $(GRAPH_OBJS) $(COMM_OBJS) \
		$(patsubst $(SRC_DIR)/%.c,$(BIN_DIR)/%.o,$(CLIENT_SRCS2))

SERVER_BINARY_NAME = server
CLIENT_BINARY_NAME = client

RESULTS_LOG = ./results/results.out

#HOARD_LIBRARY = libhoard.so

####################################################################################################
##
## TARGETS
##
####################################################################################################

all: server 

## Create directories for object files
$(BIN_DIR):
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(BIN_DIR)/utils
	@mkdir -p $(BIN_DIR)/comm
	@mkdir -p $(BIN_DIR)/game
	@mkdir -p $(BIN_DIR)/server
	@mkdir -p $(BIN_DIR)/server/sgame
	@mkdir -p $(BIN_DIR)/server/loadbal
	@mkdir -p $(BIN_DIR)/graphics
	@mkdir -p $(BIN_DIR)/graphics/3ds
	@mkdir -p $(BIN_DIR)/graphics/font
	@mkdir -p $(BIN_DIR)/graphics/texture
	@mkdir -p $(BIN_DIR)/graphics/vrml
	@mkdir -p $(BIN_DIR)/client
	@mkdir -p $(BIN_DIR)/client/AStar
	@mkdir -p $(BIN_DIR)/client/cgame
	
## Build client and server
client: $(BIN_DIR) $(CLIENT_OBJS) $(HOARD_LIBRARY) 
	$(CXX) $(LDFLAGS_CLIENT) -o $(CLIENT_BINARY_NAME) $(CLIENT_OBJS)
server: $(BIN_DIR) $(SERVER_OBJS) $(HOARD_LIBRARY)
	$(CXX) $(LDFLAGS_SERVER) -o $(SERVER_BINARY_NAME) $(SERVER_OBJS)

## Target for generic cpp files
$(BIN_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

## Target for generic c files
$(BIN_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC)  $(CXXFLAGS) -c $< -o $@

## Target for Hoard
#$(HOARD_LIBRARY):
#	@cd ./hoard/src; make all; cd ../..
#	@cp ./hoard/src/libhoard.so ./

#hoard: $(HOARD_LIBRARY)

## Clean
clean:
	@rm -f -R $(BIN_DIR)
	@rm -f $(HOARD_LIBRARY) $(SERVER_BINARY_NAME) $(CLIENT_BINARY_NAME) core* *.out
#	@cd ./hoard/src; make clean; cd ../..

## Run
HOST_NAME := `uname -n`

run_client: client
	@./$(CLIENT_BINARY_NAME) $(HOST_NAME)

run_client_gui: client
	@./$(CLIENT_BINARY_NAME) --gui $(HOST_NAME)

run_script: server
	@echo $(CXXFLAGS) > $(RESULTS_LOG)
	@export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:.; export ITM_STATISTICS="verbose"; time -p ./scripts/run.script 1000 1000 >> $(RESULTS_LOG)

run: run_script

.PHONY: clean
