#include <windows.h>

#include <process.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "chu2to3.h"

/* chuniio.dll dynamic loading */
HMODULE hinstLib;
typedef uint16_t (*chuni_io_get_api_version_t)(void);
typedef HRESULT (*chuni_io_jvs_init_t)(void);
typedef void (*chuni_io_jvs_poll_t)(uint8_t*, uint8_t*);
typedef void (*chuni_io_jvs_read_coin_counter_t)(uint16_t *);

typedef HRESULT (*chuni_io_slider_init_t)(void);
typedef void (*chuni_io_slider_set_leds_t)(const uint8_t *);
typedef void (*chuni_io_slider_start_t)(chuni_io_slider_callback_t);
typedef void (*chuni_io_slider_stop_t)(void);

chuni_io_get_api_version_t _chuni_io_get_api_version;
chuni_io_jvs_init_t _chuni_io_jvs_init;
chuni_io_jvs_poll_t _chuni_io_jvs_poll;
chuni_io_jvs_read_coin_counter_t _chuni_io_jvs_read_coin_counter;
chuni_io_slider_init_t _chuni_io_slider_init;
chuni_io_slider_set_leds_t _chuni_io_slider_set_leds;
chuni_io_slider_start_t _chuni_io_slider_start;
chuni_io_slider_stop_t _chuni_io_slider_stop;

/* SHMEM Handling */
#define BUF_SIZE 1024
#define SHMEM_WRITE(buf, size) CopyMemory((PVOID)g_pBuf, buf, size)
#define SHMEM_READ(buf, size) CopyMemory(buf,(PVOID)g_pBuf, size)
TCHAR g_shmem_name[]=TEXT("Local\\Chu2to3Shmem");
HANDLE g_hMapFile;
LPVOID g_pBuf;

#pragma pack(1)
typedef struct shared_data_s {
	uint16_t coin_counter;
	uint8_t  opbtn;
	uint8_t  beams;
	uint16_t version;
} shared_data_t;

shared_data_t g_shared_data;

bool shmem_create()
{
   g_hMapFile = CreateFileMapping(
                 INVALID_HANDLE_VALUE,    // use paging file
                 NULL,                    // default security
                 PAGE_READWRITE,          // read/write access
                 0,                       // maximum object size (high-order DWORD)
                 BUF_SIZE,                // maximum object size (low-order DWORD)
                 g_shmem_name);                 // name of mapping object

   if (g_hMapFile == NULL)
   {
      printf("shmem_create : Could not create file mapping object (%d).\n",
             GetLastError());
      return 0;
   }
   g_pBuf = MapViewOfFile(g_hMapFile,   // handle to map object
                        FILE_MAP_ALL_ACCESS, // read/write permission
                        0,
                        0,
                        BUF_SIZE);

   if (g_pBuf == NULL)
   {
      printf("shmem_create : Could not map view of file (%d).\n",
             GetLastError());

       CloseHandle(g_hMapFile);

      return 0;
   }

	return 1;
}

bool shmem_load()
{
   g_hMapFile = OpenFileMapping(
                   FILE_MAP_ALL_ACCESS,   // read/write access
                   FALSE,                 // do not inherit the name
                   g_shmem_name);               // name of mapping object

   if (g_hMapFile == NULL)
   {
      printf("shmem_load : Could not open file mapping object (%d).\n",
             GetLastError());
      return 0;
   }

   g_pBuf = MapViewOfFile(g_hMapFile, // handle to map object
               FILE_MAP_ALL_ACCESS,  // read/write permission
               0,
               0,
               BUF_SIZE);

   if (g_pBuf == NULL)
   {
      printf("shmem_load : Could not map view of file (%d).\n",
             GetLastError());

      CloseHandle(g_hMapFile);

      return 0;
   }

   return 1;
}

void shmem_free()
{
   UnmapViewOfFile(g_pBuf);
   CloseHandle(g_hMapFile);
}

/* jvs polling thread (to forward info to x64 dll) */
static HANDLE jvs_poll_thread;
static bool jvs_poll_stop_flag;

static unsigned int __stdcall jvs_poll_thread_proc(void *ctx)
{
    while (1) {
		_chuni_io_jvs_read_coin_counter(&g_shared_data.coin_counter);
		g_shared_data.opbtn = 0;
		_chuni_io_jvs_poll(&g_shared_data.opbtn, &g_shared_data.beams);
		SHMEM_WRITE(&g_shared_data, sizeof(shared_data_t));
		Sleep(1);
    }
	
    return 0;
}

/* chuniio exports */
uint16_t __cdecl chuni_io_get_api_version(void)
{
#ifdef _WIN64
/* x64 must just open the shmem and do nothing else */
if (!shmem_load())
{
	return -1;
}
    return g_shared_data.version;
#endif

	/* this is the first function called so let's setup the chuniio forwarding */
	hinstLib = LoadLibrary("chuniio.dll");
    if (hinstLib == NULL) {
        printf("ERROR: unable to load chuniio.dll (error %d)\n",GetLastError());
        return -1;
    }
	
	_chuni_io_get_api_version = (chuni_io_get_api_version_t)GetProcAddress(hinstLib, "chuni_io_get_api_version");
	_chuni_io_jvs_init = (chuni_io_jvs_init_t)GetProcAddress(hinstLib, "chuni_io_jvs_init");
	_chuni_io_jvs_poll = (chuni_io_jvs_poll_t)GetProcAddress(hinstLib, "chuni_io_jvs_poll");
	_chuni_io_jvs_read_coin_counter = (chuni_io_jvs_read_coin_counter_t)GetProcAddress(hinstLib, "chuni_io_jvs_read_coin_counter");
	_chuni_io_slider_init = (chuni_io_slider_init_t)GetProcAddress(hinstLib, "chuni_io_slider_init");
	_chuni_io_slider_set_leds = (chuni_io_slider_set_leds_t)GetProcAddress(hinstLib, "chuni_io_slider_set_leds");
	_chuni_io_slider_start = (chuni_io_slider_start_t)GetProcAddress(hinstLib, "chuni_io_slider_start");
	_chuni_io_slider_stop = (chuni_io_slider_stop_t)GetProcAddress(hinstLib, "chuni_io_slider_stop");

	/* x86 has to create the shmem */
	if (!shmem_create())
	{
		return -1;
	}
	
	if ( _chuni_io_get_api_version == NULL )
	{
		g_shared_data.version = 0x0100;
	}
	else
	{
		g_shared_data.version = _chuni_io_get_api_version();
		if (g_shared_data.version > 0x0101)
			g_shared_data.version = 0x0101;
	}

	SHMEM_WRITE(&g_shared_data, sizeof(shared_data_t));
	
    return g_shared_data.version;
}

HRESULT __cdecl chuni_io_jvs_init(void)
{
#ifdef _WIN64
/* x86 only */
return S_OK;
#endif
	_chuni_io_jvs_init();
	
	/* start jvs poll thread now that jvs_init is done */
	if (jvs_poll_thread != NULL) {
        return S_OK;
    }

    jvs_poll_thread = (HANDLE) _beginthreadex(NULL,
                                              0,
                                              jvs_poll_thread_proc,
                                              NULL,
                                              0,
                                              NULL);		
    return S_OK;
}

void __cdecl chuni_io_jvs_read_coin_counter(uint16_t *out)
{
	#ifndef _WIN64
	/* x86 can perform the call and update shmem (although this call never happens) */
	_chuni_io_jvs_read_coin_counter(&g_shared_data.coin_counter);
	SHMEM_WRITE(&g_shared_data, sizeof(shared_data_t));
	return;
	#endif
	/* x64 must read value from shmem and update arg */
	SHMEM_READ(&g_shared_data, sizeof(shared_data_t));
    if (out == NULL) {
        return;
    }
	*out = g_shared_data.coin_counter;
}

void __cdecl chuni_io_jvs_poll(uint8_t *opbtn, uint8_t *beams)
{
	#ifndef _WIN64
	/* x86 can perform the call and update shmem (although this call never happens) */
	_chuni_io_jvs_poll(&g_shared_data.opbtn, &g_shared_data.beams);
	SHMEM_WRITE(&g_shared_data, sizeof(shared_data_t));
	return;
	#endif
	/* x64 must read value from shmem and update args */
	SHMEM_READ(&g_shared_data, sizeof(shared_data_t));	
	*opbtn = g_shared_data.opbtn;
	*beams = g_shared_data.beams;
}

HRESULT __cdecl chuni_io_slider_init(void)
{	
	#ifdef _WIN64
	/* x86 only */
	return S_OK;
	#endif
	
    return _chuni_io_slider_init();
}

void __cdecl chuni_io_slider_start(chuni_io_slider_callback_t callback)
{
	#ifdef _WIN64
	/* x86 only */
	return;
	#endif
	
	_chuni_io_slider_start(callback);
}

void __cdecl chuni_io_slider_stop(void)
{
	#ifdef _WIN64
	/* x86 only */
	return;
	#endif
	_chuni_io_slider_stop();
}

void __cdecl chuni_io_slider_set_leds(const uint8_t *rgb)
{
	#ifdef _WIN64
	/* x86 only */
	return;
	#endif

	_chuni_io_slider_set_leds(rgb);
}