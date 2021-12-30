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

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>
#include <pta_boot_log.h>
#include <tmesg_ta.h>

/* BOOTLOG PTA UUID */
static TEE_UUID pta_bootlog_uuid = BOOT_LOG_SERVICE_UUID;
static TEE_TASessionHandle bootlog_pta_sess;

/*
 * Called when the instance of the TA is created. This is the first call in
 * the TA.
 */
TEE_Result TA_CreateEntryPoint(void)
{
	return TEE_SUCCESS;
}

/*
 * Called when the instance of the TA is destroyed if the TA has not
 * crashed or panicked. This is the last call in the TA.
 */
void TA_DestroyEntryPoint(void)
{

}

/*
 * Called when a new session is opened to the TA. *sess_ctx can be updated
 * with a value to be able to identify this session in subsequent calls to the
 * TA.
 */
TEE_Result TA_OpenSessionEntryPoint (uint32_t param_types,
		TEE_Param  params[4], void **sess_ctx)
{
	TEE_Result res;
	uint32_t origin;
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_NONE,
			TEE_PARAM_TYPE_NONE,
			TEE_PARAM_TYPE_NONE,
			TEE_PARAM_TYPE_NONE);

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	/* Unused parameters */
	(void)&params;
	(void)&sess_ctx;

	/*
	 * The DMSG() macro is non-standard, TEE Internal API doesn't
	 * specify any means to logging from a TA.
	 */
	res = TEE_OpenTASession(&pta_bootlog_uuid, 0, exp_param_types,
			params, &bootlog_pta_sess, &origin);
	if (res != TEE_SUCCESS) {
		EMSG("TEE_OpenTASession returned 0x%x\n",
				(unsigned int)res);
		return res;
	}

	DMSG("Bootlog TA OpenSession!\n");
	return TEE_SUCCESS;
}

/*
 * Called when a session is closed, sess_ctx hold the value that was
 * assigned by TA_OpenSessionEntryPoint().
 */
void TA_CloseSessionEntryPoint(void *sess_ctx)
{
	(void)&sess_ctx; /* Unused parameter */
	TEE_CloseTASession(bootlog_pta_sess);
	DMSG("Bootlog TA Session Closed!\n");
}

static TEE_Result get_bootlog_message(uint32_t param_types, TEE_Param params[4])
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_OUTPUT,
			TEE_PARAM_TYPE_NONE,
			TEE_PARAM_TYPE_NONE,
			TEE_PARAM_TYPE_NONE);

	TEE_Result res;
	uint32_t origin;

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	res = TEE_InvokeTACommand(bootlog_pta_sess, 0, PTA_BOOT_LOG_GET_MSG,
			exp_param_types, params, &origin);

	if (res != TEE_SUCCESS) {
		EMSG("TEE_InvokeTACommand returned 0x%x\n",
				(unsigned int)res);
	}

	return TEE_SUCCESS;
}

static TEE_Result get_bootlog_size(uint32_t param_types, TEE_Param params[4])
{

	uint32_t origin;
	TEE_Result res;

	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_OUTPUT,
			TEE_PARAM_TYPE_NONE,
			TEE_PARAM_TYPE_NONE,
			TEE_PARAM_TYPE_NONE);

	if (param_types != exp_param_types) {
		EMSG("Expected: 0x%x, got: 0x%x", exp_param_types, param_types);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	res = TEE_InvokeTACommand(bootlog_pta_sess, 0, PTA_BOOT_LOG_GET_SIZE,
			exp_param_types, params, &origin);

	if (res != TEE_SUCCESS) {
		EMSG("ERROR: TA PTA_GET_BOOT_LOG_SZ TEE_InvokeTACommand
				returned 0x%x\n", (unsigned int)res);
		return res;
	}

	return TEE_SUCCESS;

}

static TEE_Result clear_bootlog(uint32_t param_types, TEE_Param params[4])
{

	uint32_t origin;
	TEE_Result res;

	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_NONE,
			TEE_PARAM_TYPE_NONE,
			TEE_PARAM_TYPE_NONE,
			TEE_PARAM_TYPE_NONE);

	if (param_types != exp_param_types) {
		EMSG("Expected: 0x%x, got: 0x%x", exp_param_types, param_types);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	res = TEE_InvokeTACommand(bootlog_pta_sess, 0, PTA_BOOT_LOG_CLEAR,
			exp_param_types, params, &origin);

	if (res != TEE_SUCCESS) {
		EMSG("ERROR: TA PTA_CLEAR_BOOTLOG TEE_InvokeTACommand
				returned 0x%x\n", (unsigned int)res);
		return res;
	}
	return TEE_SUCCESS;

}

/*
 * Called when a TA is invoked. sess_ctx hold that value that was
 * assigned by TA_OpenSessionEntryPoint(). The rest of the paramters
 * comes from normal world.
 */
TEE_Result TA_InvokeCommandEntryPoint(void *sess_ctx, uint32_t cmd_id,
		uint32_t param_types, TEE_Param params[4])
{
	(void)&sess_ctx; /* Unused parameter */

	switch (cmd_id) {
	case TA_BOOT_LOG_GET_MSG:
		return get_bootlog_message(param_types, params);
	case TA_BOOT_LOG_GET_SIZE:
		return get_bootlog_size(param_types, params);
	case TA_BOOT_LOG_CLEAR:
		return clear_bootlog(param_types, params);
	default:
		return TEE_ERROR_BAD_PARAMETERS;
	}
}
