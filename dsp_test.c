/*
 * Copyright (C) 2009-2010 Felipe Contreras
 * Copyright (C) 2009-2010 Nokia Corporation
 * Copyright (C) 2009 Igalia S.L
 *
 * Author: Felipe Contreras <felipe.contreras@nokia.com>
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
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <stdio.h>

#include "dmm_buffer.h"
#include "dsp_bridge.h"
#include "log.h"

static unsigned long input_buffer_size = 0x1000;
static unsigned long output_buffer_size = 0x1000;
static bool done;
static int ntimes;
static bool do_fault;
static bool do_ping;
static bool do_write;

static int dsp_handle;
static void *proc;
struct dsp_notification *events[3];

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

static void
signal_handler(int signal)
{
	done = true;
}

static inline dsp_node_t *
create_node(void)
{
	dsp_node_t *node;
	const dsp_uuid_t test_uuid = { 0x3dac26d0, 0x6d4b, 0x11dd, 0xad, 0x8b,
		{ 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66 } };

	if (!dsp_register(dsp_handle, &test_uuid, DSP_DCD_LIBRARYTYPE, "/lib/dsp/test.dll64P"))
		return false;

	if (!dsp_register(dsp_handle, &test_uuid, DSP_DCD_NODETYPE, "/lib/dsp/test.dll64P"))
		return false;

	if (!dsp_node_allocate(dsp_handle, proc, &test_uuid, NULL, NULL, &node)) {
		pr_err("dsp node allocate failed");
		return NULL;
	}

	if (!dsp_node_create(dsp_handle, node)) {
		pr_err("dsp node create failed");
		return NULL;
	}

	pr_info("dsp node created");

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

		pr_info("dsp node deleted");
	}

	return true;
}

static inline void
configure_dsp_node(void *node,
		dmm_buffer_t *input_buffer,
		dmm_buffer_t *output_buffer)
{
	dsp_msg_t msg;

	msg.cmd = 0;
	msg.arg_1 = (uint32_t) input_buffer->map;
	msg.arg_2 = (uint32_t) output_buffer->map;
	if (do_fault)
		msg.arg_2 = 0x12345678;
	dsp_node_put_message(dsp_handle, node, &msg, -1);
}

static bool register_msgs(dsp_node_t *node)
{
	events[0] = calloc(1, sizeof(struct dsp_notification));
	if (!dsp_node_register_notify(dsp_handle, node,
				      DSP_NODEMESSAGEREADY, 1,
				      events[0]))
		return false;

	events[1] = calloc(1, sizeof(struct dsp_notification));
	if (!dsp_register_notify(dsp_handle, proc,
				 DSP_MMUFAULT, 1,
				 events[1]))
		return false;

	events[2] = calloc(1, sizeof(struct dsp_notification));
	if (!dsp_register_notify(dsp_handle, proc,
				 DSP_SYSERROR, 1,
				 events[2]))
		return false;

	return true;
}

static bool check_events(dsp_node_t *node, dsp_msg_t *msg)
{
	unsigned int index = 0;
	pr_debug("waiting for events");
	if (!dsp_wait_for_events(dsp_handle, events, 3, &index, 1000)) {
		pr_warning("failed waiting for events");
		return false;
	}

	switch (index) {
	case 0:
		dsp_node_get_message(dsp_handle, node, msg, 100);
		pr_debug("got dsp message: 0x%0x 0x%0x 0x%0x",
			 msg->cmd, msg->arg_1, msg->arg_2);
		return true;
	case 1:
		pr_err("got DSP MMUFAULT");
		return false;
	case 2:
		pr_err("got DSP SYSERROR");
		return false;
	default:
		pr_err("wrong event index");
		return false;
	}
}

static void run_dmm(dsp_node_t *node, unsigned long times)
{
	dmm_buffer_t *input_buffer;
	dmm_buffer_t *output_buffer;

	input_buffer = dmm_buffer_new(dsp_handle, proc);
	output_buffer = dmm_buffer_new(dsp_handle, proc);

	dmm_buffer_allocate(input_buffer, input_buffer_size);
	dmm_buffer_allocate(output_buffer, output_buffer_size);

	configure_dsp_node(node, input_buffer, output_buffer);

	pr_info("running %lu times", times);

	while (!done) {
		dsp_msg_t msg;

		if (do_write) {
			static unsigned char foo = 1;
			unsigned int i;
			for (i = 0; i < input_buffer->size; i++)
				((char *) input_buffer->data)[i] = foo;
			foo++;
		}

		dmm_buffer_clean(input_buffer, input_buffer->size);
		dmm_buffer_invalidate(output_buffer, output_buffer->size);
		msg.cmd = 1;
		msg.arg_1 = input_buffer->size;
		dsp_node_put_message(dsp_handle, node, &msg, -1);
		if (!check_events(node, &msg)) {
			done = true;
			break;
		}

		if (--times == 0)
			break;
	}

	dmm_buffer_unmap(output_buffer);
	dmm_buffer_unmap(input_buffer);

	dmm_buffer_free(output_buffer);
	dmm_buffer_free(input_buffer);
}

static void run_ping(dsp_node_t *node, unsigned long times)
{
	while (!done) {
		dsp_msg_t msg;

		if (!dsp_send_message(dsp_handle, node, 2, 0, 0)) {
			pr_err("dsp node put message failed");
			continue;
		}

		if (!check_events(node, &msg)) {
			done = true;
			break;
		}

		printf("ping: id=%d, msg=%d, mem=%d\n",
		       msg.cmd, msg.arg_1, msg.arg_2);

		if (--times == 0)
			break;
	}
}

static bool run_task(dsp_node_t *node, unsigned long times)
{
	unsigned long exit_status;

	register_msgs(node);

	if (!dsp_node_run(dsp_handle, node)) {
		pr_err("dsp node run failed");
		return false;
	}

	pr_info("dsp node running");

	if (do_ping)
		run_ping(node, times);
	else
		run_dmm(node, times);

	if (!dsp_node_terminate(dsp_handle, node, &exit_status)) {
		pr_err("dsp node terminate failed: %lx", exit_status);
		return false;
	}

	pr_info("dsp node terminated");

	return true;
}

static void handle_options(int *argc, const char ***argv)
{
	while (*argc > 0) {
		const char *cmd = (*argv)[0];
		if (cmd[0] != '-')
			break;

#ifdef DEBUG
		if (!strcmp(cmd, "-d") || !strcmp(cmd, "--debug"))
			debug_level = 3;
#endif

		if (!strcmp(cmd, "-n") || !strcmp(cmd, "--ntimes")) {
			if (*argc < 2) {
				pr_err("bad option");
				exit(-1);
			}
			ntimes = atoi((*argv)[1]);
			(*argv)++;
			(*argc)--;
		}

		if (!strcmp(cmd, "-s") || !strcmp(cmd, "--size")) {
			if (*argc < 2) {
				pr_err("bad option");
				exit(-1);
			}
			input_buffer_size = output_buffer_size = atol((*argv)[1]);
			(*argv)++;
			(*argc)--;
		}

		if (!strcmp(cmd, "-f") || !strcmp(cmd, "--fault"))
			do_fault = 1;

		if (!strcmp(cmd, "-p") || !strcmp(cmd, "--ping"))
			do_ping = 1;

		if (!strcmp(cmd, "-w") || !strcmp(cmd, "--write"))
			do_write = 1;

		(*argv)++;
		(*argc)--;
	}
}

int main(int argc, const char **argv)
{
	dsp_node_t *node;
	int ret = 0;
	unsigned i;

	signal(SIGINT, signal_handler);

#ifdef DEBUG
	debug_level = 2;
#endif
	ntimes = 1000;

	argc--; argv++;
	handle_options(&argc, &argv);

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

	node = create_node();
	if (!node) {
		pr_err("dsp node creation failed");
		ret = -1;
		goto leave;
	}

	run_task(node, ntimes);
	destroy_node(node);

leave:
	if (proc) {
		if (!dsp_detach(dsp_handle, proc)) {
			pr_err("dsp detach failed");
			ret = -1;
		}
		proc = NULL;
	}

	for (i = 0; i < ARRAY_SIZE(events); i++)
		free(events[i]);

	if (dsp_handle > 0) {
		if (dsp_close(dsp_handle) < 0) {
			pr_err("dsp close failed");
			return -1;
		}
	}

	return ret;
}
