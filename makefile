#功能：一个命名生成目标文件， 保证依赖关系正确



PROTO_FILE=$(wildcard\
		./src/proto/*.proto\
		)
		
PROTO_CPP=$(wildcard\
		./src/proto/*.h\
		)
TARGET = ./debug/hello



all: $(TARGET) $(PROTO_CPP)
	sh build.sh



$(PROTO_CPP) : $(PROTO_FILE)	
	@echo "build proto"
	./src/proto/build.sh

		
clean:
	rm -f $(TARGET)








