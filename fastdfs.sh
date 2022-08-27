#!/bin/bash

path=$(pwd)

echo ============fastdfs_start================
sudo fdfs_trackerd ${path}/conf/tracker.conf
sudo fdfs_storaged ${path}/conf/storage.conf
echo ============fastdfs_ok===================
ps -aux | grep fdfs*
echo ============redis_start==================
redis-server ./conf/redis.conf
echo ============redis_ok=====================
ps -aux | grep redis
echo =============fcgi_start==================

echo ==============fcgi_ok====================
ps -aux | grep cgi

