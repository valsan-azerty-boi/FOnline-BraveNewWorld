# Linux easy setup

## For people who need to quickly launch a FOnline server on linux

### Requirements : 
- Linux (Debian 9+, Ubuntu, etc)
- Mysql
- Xvfb
- Wine 

### Files to modify :
- /Server/config/GetAccess.cfg => Set the admin password
- /Client/FOnline.cfg => set host adress and port

### Use :
- /Server/FOnlineServer.sh => Linux script to manage the server (start / stop / restart / monitor)
- /Client/*.exe to play (You need fallout.dat / master.dat / critters.dat)
