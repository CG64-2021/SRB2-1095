SRB2 1.09.5 is a fork from an existing 1.09.4 custom build (SRB2-OLD) that aims to add some gameplay and QoL improvements and add new content.
You can check the SRB2-OLD source code here: https://git.do.srb2.org/SteelT/SRB2-OLD

Compiling the Source code (Windows only):
- To compile the source code you need to follow some initial steps about MSYS2 here: https://wiki.srb2.org/wiki/Source_code_compiling/Makefiles
- You need to install NASM in MSYS2 32-bit by using this command: ```pacman -S mingw-w64-i686-nasm```
- Now, you need to specify the directory where the source code is + src folder using the ```cd``` command: ```cd "/c/my_folder/SRB2-1095/src"```
- And, finally, compile the source code by using this command: ```make CC=gcc MINGW=1 SDL=1```
