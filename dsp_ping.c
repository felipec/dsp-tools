/*
 * Copyright (C) 2009 Nokia Corporation.
 *
 * Author: Víctor M. Jáquez L. <vjaquez@igalia.com>
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
#include <stdio.h>
#include <string.h>

#include "dsp_bridge.h"
#include "log.h"

static int dsp_handle;
static void *proc;
static int count = 50;

#define SNDIR "/lib/dsp"

static inline dsp_node_t *
create_node(void)
{
	dsp_node_t *node = NULL;

	const dsp_uuid_t uuid = { 0x12a3c3c1, 0xd015, 0x11d4, 0x9f, 0x69,
				  { 0x00, 0xc0, 0x4f, 0x3a, 0x59, 0xae } };

	if (!dsp_register(dsp_handle, &uuid, DSP_DCD_LIBRARYTYPE,
			  SNDIR "/pingdyn_3430.dll64P"))
		return NULL;

	if (!dsp_register(dsp_handle, &uuid, DSP_DCD_NODETYPE,
			  SNDIR "/pingdyn_3430.dll64P"))
		return NULL;

	if (!dsp_node_allocate(dsp_handle, proc, &uuid, NULL, NULL,
			       &node)) {
		pr_err("dsp node allocate failed");
		return NULL;
	}

	if (!dsp_node_create(dsp_handle, node)) {
		pr_err("dsp node create failed");
		return NULL;
	}

	return node;

}

static inline bool
destroy_node(dsp_node_t *node)
{
	if (node) {
		if (!dsp_node_free(dsp_handle, node)) {
			pr_err("dsp node free failed");
			return false;
		}
	}

	return true;
}

static bool
run_task(dsp_node_t *node)
{
	unsigned long exit_status;
	unsigned int index;
	struct dsp_notification event, *notifications;
	int n;

	if (!dsp_node_register_notify(dsp_handle, node,
				      DSP_NODEMESSAGEREADY, 1,
				      &event))
		pr_err("dsp node register notify failed");

	if (!dsp_node_run(dsp_handle, node)) {
		pr_err("dsp node run failed");
		return false;
	}

	notifications = &event;

	for (n = 0; n < count; n++) {
                if (!dsp_send_message(dsp_handle, node, 1, 0, 0)) {
                        pr_err("dsp node put message failed");
                        continue;
                }

		if (!dsp_wait_for_events(dsp_handle, &notifications,
					 1, &index, -1)
		    && index == 0) {
			pr_err("dsp wait for events failed");
                        break;
		}

		dsp_msg_t msg;
		if (dsp_node_get_message (dsp_handle, node, &msg, 0))
			printf("Ping: Id %d Msg %d Mem %d\n",
			       msg.cmd, msg.arg_1, msg.arg_2);
	}

	if (!dsp_node_terminate (dsp_handle, node, &exit_status)) {
		pr_err("dsp node terminate failed: %lx", exit_status);
		return false;
	}

	return true;
}

int
main (int argc, char **argv)
{
	dsp_node_t *node;
	int ret = 0;

	if (argc == 2) {
		long int c = strtol(argv[1], NULL, 0);
		if (c > 0)
			count = c;
	}

	dsp_handle = dsp_open();

	if (dsp_handle < 0) {
		pr_err("failed to open DSP");
		return -1;
	}

	if (!dsp_attach(dsp_handle, 0, NULL, &proc)) {
		pr_err("dsp attach failed");
		ret = -1;
		goto leave;
	}

	node = create_node();

	if (!node) {
		pr_err("dsp node creation failed");
		ret = -1;
		goto leave;
	}

	run_task(node);

	destroy_node(node);

leave:
	if (proc) {
		if (!dsp_detach(dsp_handle, proc)) {
			pr_err("dsp detach failed");
			ret = 1;
			goto leave;
		}
		proc = NULL;
	}

	if (dsp_close(dsp_handle) < 0) {
		pr_err("dsp close failed");
		return -1;
	}

	return ret;
}
