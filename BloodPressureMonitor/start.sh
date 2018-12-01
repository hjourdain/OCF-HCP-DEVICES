cp ./oic_svr_db.dat ./server.dat
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:../Iotivity/out/linux/x86_64/release/ ./server
