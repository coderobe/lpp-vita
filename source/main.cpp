#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <psp2/ctrl.h>
#include <psp2/types.h>
#include <psp2/moduleinfo.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/dirent.h>
#include "include/luaplayer.h"
#include "main_menu.cpp"
#include "splash.cpp"
extern "C"{
	#include <vita2d.h>
	#include "include/draw/font.h"
	#include "include/ftp/ftp.h"
}

PSP2_MODULE_INFO(0, 0, "lpp-vita")

const char *errMsg;
unsigned char *script;
int script_files = 0;

int main()
{
	char vita_ip[16];
	unsigned short int vita_port = 0;
	vita2d_init();
	vita2d_set_clear_color(RGBA8(0x00, 0x00, 0x00, 0xFF));
	vita2d_texture* texture_splash = vita2d_load_JPEG_buffer(splash, size_splash);
	vita2d_start_drawing();
	vita2d_draw_texture(texture_splash, 0, 0);
	vita2d_end_drawing();
	vita2d_swap_buffers();
	sceKernelDelayThread(4000000);
	vita2d_start_drawing();
	vita2d_clear_screen();
	vita2d_end_drawing();
	vita2d_swap_buffers();
	vita2d_free_texture(texture_splash);
	SceCtrlData pad;
	SceCtrlData oldpad;
	while (1) {
		
		// Load main script
		SceUID id = sceIoDopen("cache0:/lpp");
		SceIoDirent entry;
		memset(&entry, 0, sizeof(SceIoDirent));
		while (sceIoDread(id, &entry) > 0){
			script_files++;
			memset(&entry, 0, sizeof(SceIoDirent));
		}
		sceIoDclose(id);
		if (script_files>1) errMsg = runScript((const char*)main_menu, true);
		else{
			SceUID main_file = sceIoOpen("cache0:/lpp/index.lua", PSP2_O_RDONLY, 0777);
			if (main_file < 0) errMsg = "index.lua not found.";
			else{
				SceOff size = sceIoLseek(main_file, 0, SEEK_END);
				if (size < 1) errMsg = "Invalid main script.";
				else{
					sceIoLseek(main_file, 0, SEEK_SET);
					script = (unsigned char*)malloc(size + 1);
					sceIoRead(main_file, script, size);
					script[size] = 0;
					sceIoClose(main_file);
					errMsg = runScript((const char*)script, true);
					free(script);
				}
			}
		}
		
		if (errMsg != NULL){
			if (strstr(errMsg, "lpp_shutdown")) break;
			else{
				int restore = 0;
				bool s = true;
				while (restore == 0){
					vita2d_start_drawing();
					vita2d_clear_screen();
					font_draw_string(10, 10, RGBA8(255, 255, 255, 255), "An error occurred:");
					font_draw_string(10, 30, RGBA8(255, 255, 255, 255), errMsg);
					font_draw_string(10, 70, RGBA8(255, 255, 255, 255), "Press X to restart.");
					font_draw_string(10, 90, RGBA8(255, 255, 255, 255), "Press O to enable/disable FTP.");
					if (vita_port != 0){
						font_draw_stringf(10, 150, RGBA8(255, 255, 255, 255), "PSVITA listening on IP %s , Port %u", vita_ip, vita_port);
					}
					vita2d_end_drawing();
					vita2d_swap_buffers();
					if (s){
						sceKernelDelayThread(800000);
						s = false;
					}
					sceCtrlPeekBufferPositive(0, &pad, 1);
					if (pad.buttons & PSP2_CTRL_CROSS) {
						errMsg = NULL;
						restore = 1;
						if (vita_port != 0){
							ftp_fini();
							vita_port = 0;
						}
						sceKernelDelayThread(800000);
					}else if ((pad.buttons & PSP2_CTRL_CIRCLE) && (!(oldpad.buttons & PSP2_CTRL_CIRCLE))){
						if (vita_port == 0) ftp_init(vita_ip, &vita_port);
						else{
							ftp_fini();
							vita_port = 0;
						}
					}
					oldpad = pad;
				}
			}
		}
	}

	vita2d_fini();
	sceKernelExitProcess(0);
	return 0;
}
