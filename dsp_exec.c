/*
 * Copyright (C) 2009-2010 Igalia S.L.
 *
 * Author: Víctor Manuel Jáquez Leal <vjaquez@igalia.com>
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

#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

#include "dsp_bridge.h"
#include "log.h"

int main(int argc, const char **argv)
{
	int ret = 0;
	int dsp_handle;
	void *proc;
	char *cmd[1];

	if (argc != 2) {
		pr_err("Wrong arguments: %s <dsp_program>", argv[0]);
		return -1;
	}

	dsp_handle = dsp_open();

	if (dsp_handle < 0) {
		pr_err("dsp open failed");
		return -1;
	}

	if (!dsp_attach(dsp_handle, 0, NULL, &proc)) {
		pr_err("dsp attach failed");
		ret = -1;
		goto leave;
	}

	if (!dsp_stop(dsp_handle, proc)) {
		pr_err("dsp stop failed");
		ret = -1;
		goto leave;
	}

	cmd[0] = (char *) argv[1];
	if (!dsp_load(dsp_handle, proc, 1, cmd, NULL)) {
		pr_err("dsp load failed");
		ret = -1;
		goto leave;
	}

	if (!dsp_start(dsp_handle, proc)) {
		pr_err("dsp start failed");
		ret = -1;
	}

leave:
	if (proc) {
		if (!dsp_detach(dsp_handle, proc)) {
			pr_err("dsp detach failed");
			ret = -1;
		}
		proc = NULL;
	}

	if (dsp_handle > 0) {
		if (dsp_close(dsp_handle) < 0) {
			pr_err("dsp close failed");
			return -1;
		}
	}

	return ret;
}
