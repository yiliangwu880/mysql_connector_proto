cd `dirname $0` #进入这个目录(切换当前工作目录)
./protoc --cpp_out=./ -I=./ ./*.proto 
echo "==============build proto cpp=================="