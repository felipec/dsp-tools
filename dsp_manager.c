/*
 * Copyright (C) 2009 Nokia Corporation.
 * Copyright (C) 2009 Texas Instruments, Incorporated
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
 * version 2.1 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <stdlib.h>
#include <signal.h>
#include <unistd.h> /* for sleep */

#include "log.h"
#include "dsp_bridge.h"

#ifdef SYSLOG
#include <syslog.h>
#endif

static int dsp_handle;
static void *proc;

const char *process = "dsp-manager";
const char *recover_script = "/usr/libexec/dsp-recover";

void reset_dsp(void)
{
	int ret;

	dsp_detach(dsp_handle, proc);
	dsp_close(dsp_handle);

	ret = system(recover_script);

	sleep(2);

	if (ret == 0)
		pr_info("recovered");
	else
		pr_err("couldn't recover");
}

void signal_handler(int n)
{
	pr_info("signal %d received", n);
	reset_dsp();
}

bool attach(void)
{
	dsp_handle = dsp_open();

	if (dsp_handle < 0) {
		pr_err("could not open");
		return false;
	}

	pr_info("opened");

	if (!dsp_attach(dsp_handle, 0, NULL, &proc)) {
		pr_err("could not attach");
		return false;
	}

	pr_info("attached");

	return true;
}

int main(void)
{
	struct dsp_notification *n_mmufault = NULL;
	struct dsp_notification *n_syserror = NULL;
	struct dsp_notification *n_objects[2];
	int ret = -1;

	signal(SIGUSR1, signal_handler);

#ifdef SYSLOG
	openlog(process, 0, LOG_USER);
#endif

	n_mmufault = calloc(1, sizeof(n_mmufault));
	if (!n_mmufault) {
		pr_err("not enough memory");
		goto leave;
	}

	n_syserror = calloc(1, sizeof(n_syserror));
	if (!n_syserror) {
		pr_err("not enough memory");
		goto leave;
	}

	while (true) {
		unsigned int count;
		unsigned int index = 0;

		for (count = 0; count < 3; count++) {
			if (attach()) {
				pr_info("attached");
				break;
			}
			pr_err("failed to attach, retry");
			sleep(2);
		}

		if (count == 3) {
			pr_err("could not attach");
			goto leave;
		}

		if (!dsp_register_notify(dsp_handle, proc,
					 DSP_MMUFAULT, 1,
					 n_mmufault))
		{
			pr_err("failed to register for DSP_MMUFAULT");
			goto leave;
		}
		n_objects[0] = n_mmufault;

		if (!dsp_register_notify(dsp_handle, proc,
					 DSP_SYSERROR, 1,
					 n_syserror))
		{
			pr_err("failed to register for DSP_SYSERROR");
			goto leave;
		}
		n_objects[1] = n_syserror;

		pr_info("begin");

		if (!dsp_wait_for_events(dsp_handle, n_objects, 2, &index, -1)) {
			pr_err("failed waiting for events");
			goto leave;
		}

		if (index == 0 || index == 1) {
			pr_err("DSP crash detected: %u", index);
			reset_dsp();
		}
		else
			pr_err("what?");
	}

	ret = 0;

leave:
	free(n_mmufault);
	free(n_syserror);
	if (proc)
		dsp_detach(dsp_handle, proc);
	if (dsp_handle > 0)
		dsp_close(dsp_handle);

	pr_info("end");

	return ret;
}
