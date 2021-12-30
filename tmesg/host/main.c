/*
 * Copyright (c) 2021, Linaro Limited
 * Copyright (c) 2021, Devendra Devadiga
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <err.h>
#include <assert.h>

/* OP-TEE TEE client API (built by optee_client) */
#include <tee_client_api.h>

/* For the UUID (found in the TA's h-file(s)) */
#include <tmesg_ta.h>

/* TEE resources */
struct tee_ctx_resource {
	TEEC_Context context;
	TEEC_Session sess;
};

void initialize_tee_session(struct tee_ctx_resource *ctx)
{
	TEEC_UUID uuid = BOOT_LOG_SERVICE_UUID;
	uint32_t origin;
	TEEC_Result res;

	/* Initialize a context connecting us to the TEE */
	res = TEEC_InitializeContext(NULL, &ctx->context);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InitializeContext failed with code 0x%x", res);

	/* Open a session with the TA */
	res = TEEC_OpenSession(&ctx->context, &ctx->sess, &uuid,
			TEEC_LOGIN_PUBLIC, NULL, NULL, &origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x",
				res, origin);
}

void terminate_tee_session(struct tee_ctx_resource *ctx)
{
	TEEC_CloseSession(&ctx->sess);
	TEEC_FinalizeContext(&ctx->context);
}

/* Shared memory to get the bootlog message. */
static TEEC_SharedMemory bootlog_shm = {
	.size = (1024 * 1024),
	.flags = TEEC_MEM_OUTPUT,
};

uint8_t *default_rx;

static void display_ca_usage(const char *ca_name)
{
	printf("#### TEE Secure Bootlog message ####\n");
	printf("Usage: %s [-s|-S|--size] [-c|-C|--clear]\n", ca_name);
	puts("  -s 	Display current bootlog message size\n"
			"  -c 	Clear the boot log message in bootlog buffer\n");

	printf("Exmaple Usage: %s -s \n", ca_name);
	printf("               %s -S \n", ca_name);
	printf("               %s --size \n", ca_name);
	printf("               %s -c \n", ca_name);
	printf("               %s -C \n", ca_name);
	printf("               %s --clear \n", ca_name);
	printf("               %s  \n", ca_name);
	exit(1);
}

static void clear_bootlog(void)
{
	TEEC_Operation op;
	TEEC_Result res;
	uint32_t err_origin;

	struct tee_ctx_resource ctx;

	initialize_tee_session(&ctx);

	/* Clear the bootlog message which is stored in Secure memory*/
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE,
			TEEC_NONE, TEEC_NONE);

	res = TEEC_InvokeCommand(&ctx.sess, TA_BOOT_LOG_CLEAR, &op, &err_origin);
	if (res != TEEC_SUCCESS)
	{
		errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",
				res, err_origin);
	}

	terminate_tee_session(&ctx);
}

static uint32_t get_bootlog_msg_size(void)
{
	TEEC_Operation op;
	TEEC_Result res;
	uint32_t err_origin;

	uint32_t bootlog_size = 0;
	struct tee_ctx_resource ctx;

	initialize_tee_session(&ctx);

	/* Get the current size of bootlog message which is stored in Secure memory*/
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_OUTPUT, TEEC_NONE,
			TEEC_NONE, TEEC_NONE);

	res = TEEC_InvokeCommand(&ctx.sess, TA_BOOT_LOG_GET_SIZE, &op, &err_origin);
	if (res != TEEC_SUCCESS)
	{
		errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",
				res, err_origin);
		terminate_tee_session(&ctx);
	}

	bootlog_size = op.params[0].value.a;

	terminate_tee_session(&ctx);

	return bootlog_size;
}

static TEEC_Result get_bootlog_message(void)
{
	TEEC_Operation op;
	TEEC_Result res = TEEC_ERROR_GENERIC;
	uint32_t i, err_origin;

	uint32_t bootlog_size = 0;
	struct tee_ctx_resource ctx;

	initialize_tee_session(&ctx);

	/* Get the current size of bootlog message which is stored in Secure memory*/
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_OUTPUT, TEEC_NONE,
			TEEC_NONE, TEEC_NONE);

	res = TEEC_InvokeCommand(&ctx.sess, TA_BOOT_LOG_GET_SIZE, &op, &err_origin);
	if (res != TEEC_SUCCESS)
	{
		errx(1, "TEEC_InvokeCommand TA_BOOT_LOG_GET_SIZE failed with code 0x%x origin 0x%x",
				res, err_origin);
		terminate_tee_session(&ctx);
		return res;
	}

	bootlog_size = op.params[0].value.a;

	/*
	 * Allocate Shared Memory for TA buffer which holds bootlog data
	 */

	res = TEEC_AllocateSharedMemory(&ctx.context, &bootlog_shm);
	if (res != TEEC_SUCCESS)
	{
		errx(1, "TEEC_AllocateSharedMemory failed with code 0x%x", res);
		terminate_tee_session(&ctx);
		return res;
	}

	bootlog_shm.size = bootlog_size + 8;

	memset(&op, 0, sizeof(op));

	op.paramTypes = TEEC_PARAM_TYPES (TEEC_MEMREF_WHOLE, TEEC_NONE,
			TEEC_NONE, TEEC_NONE);

	default_rx = bootlog_shm.buffer;

	/* Bootlog buffer as 1st param. */
	op.params[0].memref.parent = &bootlog_shm;
	op.params[0].memref.offset = 0;
	op.params[0].memref.size = bootlog_shm.size;

	res = TEEC_InvokeCommand(&ctx.sess, TA_BOOT_LOG_GET_MSG, &op, &err_origin);
	if (res != TEEC_SUCCESS)
	{
		errx(1, "TEEC_InvokeCommand TA_BOOT_LOG_GET_MSG failed with code 0x%x origin 0x%x",
				res, err_origin);
		terminate_tee_session(&ctx);
		return res;
	}

	for (i = 0; i < (bootlog_size); i++, default_rx++)
		putchar(*default_rx);

	putchar('\n');

	terminate_tee_session(&ctx);
	TEEC_ReleaseSharedMemory(&bootlog_shm);

	return res;
}


int main(int argc, char *argv[])
{
	uint32_t bootlog_size = 0;
	TEEC_Result res = TEEC_ERROR_GENERIC;

	if (argc > 2)
		display_ca_usage(argv[0]);

	if (argc == 2)
	{
		if (!strcmp(argv[1], "-s") || !strcmp(argv[1], "-S") || !strcmp(argv[1], "--size")) {
			bootlog_size = get_bootlog_msg_size();
			printf("Current bootlog message size is : %d\n", bootlog_size);
			return 0;
		}
		else if (!strcmp(argv[1], "-c") || !strcmp(argv[1], "-C") || !strcmp(argv[1], "--clear")) {
			clear_bootlog();
			return 0;
		}
		else
		{
			display_ca_usage(argv[0]);
		}
	}
	else
	{
		res = get_bootlog_message();
		if (res != TEEC_SUCCESS)
		{
			errx(1, "get_bootlog_message failed with code 0x%x", res);
			return -1;
		}

	}

	return 0;
}
