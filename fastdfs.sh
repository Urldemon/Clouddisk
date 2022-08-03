#!/bin/bash
echo ============fastdfs_start================
sudo fdfs_trackerd /home/hang/github/PanServer/conf/tracker.conf
sudo fdfs_storaged /home/hang/github/PanServer/conf/storage.conf
ps -aux | grep fdfs*
echo ============fastdfs_ok==================

