
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/threadmgr.h>
#include <psp2kern/kernel/cpu.h>
#include <psp2kern/kernel/debug.h>
#include <psp2kern/ctrl.h>
#include <psp2kern/sblaimgr.h>
#include <taihen.h>


int module_get_offset(SceUID pid, SceUID modid, int segidx, size_t offset, uintptr_t *addr);

static SceUID SceBt_sub_22999C8_hook_uid = -1;
static tai_hook_ref_t SceBt_sub_22999C8_ref;
static SceUID SceBt_sub_22947E4_hook_uid = -1;
static tai_hook_ref_t SceBt_sub_22947E4_ref;


#define DS3_VID 0x54C
#define DS3_PID 0x268

static int is_ds3(const unsigned short vid_pid[2])
{
	return vid_pid[0] == DS3_VID && vid_pid[1] == DS3_PID;
}

static int SceBt_sub_22999C8_hook_func(void *dev_base_ptr, int r1)
{
	unsigned int flags = *(unsigned int *)(r1 + 4);

	if (dev_base_ptr && !(flags & 2)) {
		const void *dev_info = *(const void **)(dev_base_ptr + 0x14A4);
		const unsigned short *vid_pid = (const unsigned short *)(dev_info + 0x28);

		if (is_ds3(vid_pid)) {
			unsigned int *v8_ptr = (unsigned int *)(*(unsigned int *)dev_base_ptr + 8);

			/*
			 * We need to enable the following bits in order to make the Vita accept the new connection, otherwise it will refuse it.
			 */
			*v8_ptr |= 0x11000;
		}
	}

	return TAI_CONTINUE(int, SceBt_sub_22999C8_ref, dev_base_ptr, r1);
}

static void *SceBt_sub_22947E4_hook_func(unsigned int r0, unsigned int r1, unsigned long long r2)
{
	void *ret = TAI_CONTINUE(void *, SceBt_sub_22947E4_ref, r0, r1, r2);

	if (ret) {
		/*
		 * We have to enable this bit in order to make the Vita accept the controller.
		 */
		*(unsigned int *)(ret + 0x24) |= 0x1000;
	}

	return ret;
}

int (* readButtons)(int port, SceCtrlData *pad_data, int count);

#if defined(DS3EXT_DEBUG)
SceUInt32 press, release, hold;
#endif

int readButtons_hook(int port, SceCtrlData *pad_data, int count){

	int res, is_ds3 = 0;

	if(port == 0){
		port = 1;
		is_ds3 = 1;
	}

	res = readButtons(port, pad_data, count);

#if defined(DS3EXT_DEBUG)
	SceUInt32 buttons = pad_data->buttons;
	SceUInt32 ctrl_cache = press | hold;

	press   = buttons & ~ctrl_cache;
	release = ~buttons & ctrl_cache;
	hold    = ctrl_cache ^ release;

	if(press != 0 || release != 0){
		ksceDebugPrintf("press: 0x%08X release: 0x%08X res: 0x%X\n", press, release, res);
	}
#endif

	if(res == 0 && is_ds3 != 0){
		res = 0x28;
	}

	return res;
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize args, void *argp){

	SceUID moduleId;

	if(ksceSblAimgrIsDolce() != 0){
		return SCE_KERNEL_START_NO_RESIDENT;
	}

	moduleId = ksceKernelSearchModuleByName("SceDs3");
	if(moduleId < 0){
		return SCE_KERNEL_START_NO_RESIDENT;
	}

	moduleId = ksceKernelSearchModuleByName("SceCtrl");
	if(moduleId >= 0){

		void *ctrl_context;

		module_get_offset(SCE_GUID_KERNEL_PROCESS_ID, moduleId, 1, 0x10, (uintptr_t *)&ctrl_context);

		ScePVoid pVirtualControllerDriver = *(ScePVoid *)(ctrl_context + 0xAB0);
		if(pVirtualControllerDriver == NULL){
			return SCE_KERNEL_START_NO_RESIDENT;
		}

		taiInjectDataForKernel(SCE_GUID_KERNEL_PROCESS_ID, moduleId, 0, 0x10B0, (char[4]){0x00, 0xF0, 0xCA, 0x84}, 4);

		readButtons = *(ScePVoid *)(pVirtualControllerDriver);
		ksceKernelCpuUnrestrictedMemcpy(pVirtualControllerDriver, (ScePVoid[]){readButtons_hook}, 4);
	}


	moduleId = ksceKernelSearchModuleByName("SceBt");

	SceBt_sub_22999C8_hook_uid = taiHookFunctionOffsetForKernel(KERNEL_PID,
		&SceBt_sub_22999C8_ref, moduleId, 0,
		0x22999C8 - 0x2280000, 1, SceBt_sub_22999C8_hook_func);

	SceBt_sub_22947E4_hook_uid = taiHookFunctionOffsetForKernel(KERNEL_PID,
		&SceBt_sub_22947E4_ref, moduleId, 0,
		0x22947E4 - 0x2280000, 1, SceBt_sub_22947E4_hook_func);

	return SCE_KERNEL_START_SUCCESS;
}
