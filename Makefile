OBJS = Util.o \
	BaseRpcChannel.o \
	RpcMessage.o \
	RpcController.o \
	BaseRpcServer.o \
	TcpRpcChannel.o \
	TcpRpcServer.o \
	UdpRpcChannel.o \
	UdpRpcServer.o 

ECHO_TEST_OBJS = \
	echo.pb.o \
	EchoTestServer.o \
	EchoTestClient.o \
	EchoTest.o

UNIT_TEST_OBJS = \
	echo.pb.o \
  EchoTestServer.o \
  EchoTestClient.o \
  RpcUnitTest.o


LIBS = -L. -L$(GTEST_DIR)/lib/.libs -L$(PROTOBUF_DIR)/src/.libs -L$(BOOST_DIR)/stage/lib -lgtest -lpbrcpp -lprotobuf -lboost_system -lboost_thread -pthread -lboost_date_time


CC_FLAGS = -c -fPIC -g -ggdb -I$(GTEST_DIR)/include -I$(PROTOBUF_DIR)/src -I$(BOOST_DIR)
all: libpbrcpp echotest unittest

libpbrcpp: $(OBJS)
	ar -rcs libpbrcpp.a $(OBJS) 
echotest: $(ECHO_TEST_OBJS) libpbrcpp.a
	g++ -o echotest $(ECHO_TEST_OBJS) $(LIBS)

unittest: $(UNIT_TEST_OBJS) libpbrcpp.a
	g++ -o rpcunittest $(UNIT_TEST_OBJS) $(LIBS)
.cpp.o:
	g++ $(INC) $(CC_FLAGS) $<
.cc.o:
	g++ $(INC) $(CC_FLAGS) $<
clean:
	rm -rf $(OBJS) $(TEST_OBJS) libpbrcpp.a echotest rpcunittest $(ECHO_TEST_OBJS) RpcUnitTest.o
