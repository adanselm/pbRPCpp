OBJS = Util.o \
	BaseRpcChannel.o \
	RpcController.o \
	RpcMessage.o \
	BaseRpcServer.o \
	TcpRpcChannel.o \
	TcpRpcServer.o \
	UdpRpcChannel.o \
	UdpRpcServer.o 

TEST_OBJS = \
	echo.pb.o \
	EchoTest.o

LIBS = -L. -lpbrcpp -lprotobuf -lboost_system -lboost_thread -pthread -lboost_date_time -L../../boost/stage/lib -L../../protobuf-2.5.0/src/.libs

CC_FLAGS = -c -fPIC -g -ggdb -I../../protobuf-2.5.0/src -I../../boost
all: libpbrcpp test

libpbrcpp: $(OBJS)
	ar -rcs libpbrcpp.a $(OBJS) 
test: $(TEST_OBJS) libpbrcpp.a
	g++ -o test $(TEST_OBJS) $(LIBS)

.cpp.o:
	g++ $(INC) $(CC_FLAGS) $<
.cc.o:
	g++ $(INC) $(CC_FLAGS) $<
clean:
	rm -rf $(OBJS) $(TEST_OBJS) test libpbrcpp.a
