// id_mm.c - Memory Manager (malloc/free port)
// Replaces the original DOS EMS/XMS/segment-based memory manager
// with simple malloc/free wrappers.

#include "id_mm.h"

#include <stdlib.h>
#include <string.h>

extern void Quit(char *error);

mminfotype mminfo;
memptr     bufferseg;
boolean    mmerror;
void       (*beforesort)(void);
void       (*aftersort)(void);

static boolean bombonerror = false;

void MM_Startup(void)
{
    memset(&mminfo, 0, sizeof(mminfo));
    bufferseg = malloc(BUFFERSIZE);
    if (!bufferseg)
        Quit("MM_Startup: Couldn't allocate buffer segment");
    mmerror = false;
    beforesort = NULL;
    aftersort = NULL;
}

void MM_Shutdown(void)
{
    if (bufferseg)
    {
        free(bufferseg);
        bufferseg = NULL;
    }
}

void MM_MapEMS(void)
{
    // No-op: EMS does not exist on modern systems
}

void MM_GetPtr(memptr *ptr, unsigned long size)
{
    if (!ptr)
        Quit("MM_GetPtr: Null pointer");

    *ptr = malloc((size_t)size);
    if (!*ptr)
    {
        mmerror = true;
        if (bombonerror)
            Quit("MM_GetPtr: Out of memory");
    }
}

void MM_FreePtr(memptr *ptr)
{
    if (!ptr || !*ptr)
        return;

    free(*ptr);
    *ptr = NULL;
}

void MM_SetPurge(memptr *ptr, int purge)
{
    (void)ptr; (void)purge;
}

void MM_SetLock(memptr *ptr, boolean lock)
{
    (void)ptr; (void)lock;
}

void MM_SortMem(void)
{
    if (beforesort)
        beforesort();
    if (aftersort)
        aftersort();
}

void MM_ShowMemory(void)
{
    // No-op: no VGA text-mode memory display
}

long MM_UnusedMemory(void)
{
    return 0x100000; // 1MB placeholder
}

long MM_TotalFree(void)
{
    return 0x100000; // 1MB placeholder
}

void MM_BombOnError(boolean bomb)
{
    bombonerror = bomb;
}
