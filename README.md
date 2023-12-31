SRB2 1.09.5 is a fork from an existing 1.09.4 custom build (SRB2-OLD) that aims to add some gameplay and QoL improvements and add new content.
You can check the SRB2-OLD source code here: https://git.do.srb2.org/SteelT/SRB2-OLD

Compiling the Source code (Windows only):
- To compile the source code you need to follow some initial steps about MSYS2 here: https://wiki.srb2.org/wiki/Source_code_compiling/Makefiles
- You need to install NASM in MSYS2 32-bit by using this command: ```pacman -S mingw-w64-i686-nasm```
- Now, you need to specify the directory where the source code is + src folder using the ```cd``` command: ```cd "/c/my_folder/SRB2-1095/src"```
- And, finally, compile the source code by using this command: ```make CC=gcc MINGW=1 SDL=1```



If you want to contribute, here's the TODO list for Public Beta 3 and 4 below (As tasks are done, the list gets smaller. Some of them has already been done, thx):
- Fix a game crash in netgame when the server decides to select unlockable stages but you don't have them unlocked yet.
- Add new secret maps in single player mode (For public beta 3 we need 3 of them, one of them is already being made).
- Fix a bug where the game does not receive the ":" character in the masterserver option (options >> server >> Master server).
- Fix a bug where the game crashes after finishing the credits in multiplayer.
- Rewrite skicolors system so we can add more than 16 skincolors in the future using SOC.
- Secret stages should be shown in the stage select list in netgame if they are unlocked so the server can select them without using console command.
- Add Switch Team option in CTF so the players can select teams in the menu anytime (Network options -> CTF options -> Switch team).
- Add coop options in Network Options so we can add options for coop mode.
- Add "exit for all" option so the stage can only end once all players have completed the level.
- Add a option to activate countdown once half of the players have finished the level. If the time runs out, exitlevel.
- Rewrite the Team Match gametype so we can use the same team system as CTF instead of an old and scrapped one.
- Port the player list HUD from 2.0 to team match and CTF gametypes.
- Add colormap support in OpenGL mode.
- Port the ping measurement system from 2.0.
- Remove the input delay from the camera in third person in netgame.

Future Plans:

For the final release of the 1.09.5, there's some crazy plans I would like to do:
- Rewrite the splitscreen code because the original one is a pure mess that complicates the overall source code. The plan for now is to remove it in future beta releases.
- Rewrite the netcode because the original one is a mess and we would like to play some netgames without input delay and consistency failure. Maybe we can use the netcode
model from Quake 3 or Open Arena.
- Add more features to SOC so we can make more advanced mods using it.
- Remove all unnecessary macros regarding console ports and such so the code would be less complex and more clean.
- Modify the makefile to be more simple and readable.
- Rewrite the software and OGL renderers because the original code for both is broken and old. For software mode the plan is to create a 3D software renderer that renders a 3D
world using only 256 colors from the pallette. For OpenGL mode the plan is to recreate a 3D hardware-accelerated renderer that renders a 3D world with a good dynamic lights and shaders.
Both of these renderers must use triangles instead of lines to render the world.
- Remove assembly code since it probably only works in some systems, but be careful because the software renderer uses it to render more faster.
- Optimize the game so we can make it run in low end PCs with same speed as an old console like Dreamcast at 35FPS capped.
