# Fallout Online - Linux easy setup

## For people who need to quickly launch a Fallout Online server on linux

### Requirements : 
- Linux (Debian 11+, Ubuntu, etc)
- Wine

### Files to modify :
- /Server/config/GetAccess.cfg => Set the admin password
- /Client/FOnline.cfg => set host adress and port

### Use :
- /Server/FOnlineServer.sh => Linux script to manage the server (start / stop / restart / monitor)
- /Client/*.exe to play (You need fallout.dat / master.dat / critters.dat)

# FOClassic + PReloaded + Brave New World update

| | FOClassic | PReloaded | Brave New World update |
|:----------:|:----------:|:-----------:|:------------------:|
| Description |Based on Fallout Online engine which became open source a few years ago. Most used revision was taken and improved by Rotators.  | A simplified version of Reloaded Season 3 as an example to use as test or integrate FOclassic into existing repos. | New features for the previous two, code fixes or improvements both to engine and scripts. |
| Aim | The purpose of this was to determined before FOnline becoming open source, probably to help existing servers with bug fixes, patches. | For testing and ease of access to a working FOclassic engine/server/client bundle. | Further updates and fixes in an independent way, the aim was to publish separate tags with easy diffs as usable parts. |
| License | GPL3 | IDK | If you use these patches together with the other two, then those licenses take priority (GPL3), otherwise do what you want with it. (MIT)|

## FOnline-BraveNewWorld
FOClassic and PReloaded project, mostly for fodev.net tutorials or module code shares.

Idea and goal of this project:
- A layer between development and server running, helping server owners to be able to set up and host their servers with minimal coding knowledge, as most important or mainstream changes or features would be available
- Allow easier development and get to know FOnline SDK, for this purpose some tutorials have also been posted on fodev.net (Slowhand)
- Allow sharing of code pieces, that any server can integrating with minimal effort and knowledge. Ofc code changes to AngelScript are prefered over code changes to Engine, because they are easier to integrate and less risky compatibility wise or even causing crashes. 
   - There were many servers with closed modifications, some did the very same thing, sometimes for competition, which is a waste of dev time. 
   - Allow for rogue developers, to share their work, without affiliating to any servers. 

So for these reasons, changes must be cleanly written and well documented, possibly both in code, discussions, wiki or issues. Every change should be commited to an issue and big updates if any, should be linked to more issues if it was not possible to segregate.

Since I only work from time to time on this project, not sure if anyone would suggest pull request, I could respond in an acceptable timeframe, so for now be free to fork it or contact me on discord ( https://discord.gg/fwwjvKbNEu ) for more info.


## Download

Well, in case you are new to git and have trouble getting all parts together, here are some snapshots:

2023-06-01 Spearfishing test: use latest below<br>
2024-03-25 Car Tracker and Hub Parkinglot<br>
2024-05-22 Tragic the Garnering Collectible<br>
2024-06-03 NPC names and quest colors, ~find command, Status and Misc text (XP, Shocked), plasma/fire/electric damage effects<br>
2024-0705 Global Radio, Broadcast filter - https://www.dropbox.com/scl/fi/84e8kz405wu3s81n1yttf/FOCLASSIC-BNW_2024-07-05_broadcastfix.zip?rlkey=a78pzsyzpaj3mzuuqi0xdmscr&st=p4wrt29t&dl=0<br>
<br>

Working versions or ones that need testing may be added later, periodaically, but should not count on them.<br>

Jinexd Jack interface: (download from Dropbox if there is fresh version, so my LFS quota doesn't drop) <br>
2024-05-30 Jinxed Jack interface, added pickup/container filter buttons: https://www.dropbox.com/scl/fi/y9khvvrfkskn1hu4ly868/JinxedJack.zip?rlkey=hzuq2ezgsl2utc63ue0bhnff2&st=wkoymh2q&dl=0

