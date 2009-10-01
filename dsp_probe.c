/*
 * Copyright (C) 2009 Nokia Corporation.
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

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "dsp_bridge.h"
#include "log.h"

static int dsp_handle;

static inline const char *
node_type_to_str(enum dsp_node_type type)
{
	switch (type) {
		case DSP_NODE_DEVICE:
			return "device";
		case DSP_NODE_MESSAGE:
			return "message";
		case DSP_NODE_TASK:
			return "task";
		case DSP_NODE_DAISSOCKET:
			return "dais socket";
		default:
			return NULL;
	}
}

static inline const char *
node_status_to_str(enum dsp_node_state state)
{
	switch (state) {
		case NODE_ALLOCATED:
			return "allocated";
		case NODE_CREATED:
			return "created";
		case NODE_RUNNING:
			return "running";
		case NODE_PAUSED:
			return "paused";
		case NODE_DONE:
			return "done";
		default:
			return NULL;
	}
}

struct node_info {
	dsp_uuid_t id;
	enum dsp_node_type type;
	char name[32];
	enum dsp_node_state state;
};

static inline bool uuidcmp(dsp_uuid_t *u1, dsp_uuid_t *u2)
{
	if (memcmp(u1, u2, sizeof(dsp_uuid_t)) == 0)
		return true;
	return false;
}

static bool do_list(void)
{
	struct dsp_ndb_props props;
	unsigned num = 0, i;
	void **tmp_table;
	void *proc_handle;
	unsigned node_count = 0, allocated_count = 0;
	struct node_info *node_table;

	if (!dsp_enum(dsp_handle, 0, &props, sizeof(props), &num)) {
		pr_err("failed to enumerate nodes");
		return false;
	}

	if (!dsp_attach(dsp_handle, 0, NULL, &proc_handle)) {
		pr_err("dsp attach failed");
		return false;
	}

	node_table = calloc(num, sizeof(*node_table));
	for (i = 0; i < num; i++) {
		if (dsp_enum(dsp_handle, i, &props, sizeof(props), &num)) {
			memcpy(&node_table[i].id, &props.uiNodeID, sizeof(props.uiNodeID));
			memcpy(&node_table[i].name, props.acName, sizeof(props.acName));
			node_table[i].type = props.uNodeType;
			node_table[i].state = -1;
		}
	}

	tmp_table = calloc(num, sizeof(*tmp_table));
	if (!dsp_enum_nodes(dsp_handle, proc_handle, tmp_table, num,
			    &node_count, &allocated_count)) {
		pr_err("failed to enumerate nodes");
		goto leave;
	}

	for (i = 0; i < node_count; i++) {
		struct dsp_node_attr attr;
		dsp_node_t node = { .handle = tmp_table[i] };
		if (dsp_node_get_attr(dsp_handle, &node, &attr, sizeof(attr))) {
			unsigned j;
			for (j = 0; j < num; j++) {
				if (uuidcmp(&node_table[j].id, &attr.info.props.uiNodeID)) {
					node_table[j].state = attr.info.state;
					break;
				}
			}
		}
	}

	for (i = 0; i < num; i++) {
		const char *state = node_status_to_str(node_table[i].state);
		if (state)
			printf("%s: %s (%s)\n",
			       node_type_to_str(node_table[i].type),
			       node_table[i].name,
			       state);
		else
			printf("%s: %s\n",
			       node_type_to_str(node_table[i].type),
			       node_table[i].name);
	}

	if (!dsp_detach(dsp_handle, proc_handle))
		pr_err("dsp detach failed");

leave:
	free(node_table);
	free(tmp_table);

	return true;
}

int main(int argc,
	 char *argv[])
{
	bool ok;

	dsp_handle = dsp_open();
	if (dsp_handle < 0) {
		pr_err("failed to open DSP");
		return -1;
	}

	ok = do_list();

	return ok ? 0 : -1;
}
