/*
 * Copyright (C) 2009-2010 Nokia Corporation
 *
 * Authors:
 * Felipe Contreras <felipe.contreras@nokia.com>
 * Johann Prieur <johann.prieur@nokia.com>
 *
 * This file may be used under the terms of the GNU Lesser General Public
 * License version 2.1, a copy of which is found in LICENSE included in the
 * packaging of this file.
 */

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h> /* for usleep */

#include "dsp_bridge.h"

static unsigned delay = 500; /* in ms */

static void display(void)
{
	int dsp_handle;
	void *proc_handle;
	struct dsp_info info;

	dsp_handle = dsp_open();
	if (dsp_handle < 0)
		return;
	if (!dsp_attach(dsp_handle, 0, NULL, &proc_handle))
		goto leave;
	do {
		dsp_proc_get_info(dsp_handle, proc_handle, DSP_RESOURCE_PROCLOAD, &info, sizeof(info));

		printf("load: %lu, freq: %lu\n",
				info.result.proc.pred_load,
				info.result.proc.pred_freq);

		usleep(delay * 1000);
	} while (true);
leave:
	if (proc_handle)
		dsp_detach(dsp_handle, proc_handle);
	dsp_close(dsp_handle);
}

int main(int argc, char *argv[])
{
	display();
	return 0;
}
