cp ./oic_svr_db.dat ./server.dat
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:../iotivity/out/linux/x86_64/release/ ./server
