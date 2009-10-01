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

struct node_info {
	dsp_uuid_t id;
	enum dsp_node_type type;
	char name[32];
};

static bool do_list(void)
{
	struct dsp_ndb_props props;
	unsigned num = 0, i;
	struct node_info *node_table;

	if (!dsp_enum(dsp_handle, 0, &props, sizeof(props), &num)) {
		pr_err("failed to enumerate nodes");
		return false;
	}

	node_table = calloc(num, sizeof(*node_table));
	for (i = 0; i < num; i++) {
		if (dsp_enum(dsp_handle, i, &props, sizeof(props), &num)) {
			memcpy(&node_table[i].id, &props.uiNodeID, sizeof(props.uiNodeID));
			memcpy(&node_table[i].name, props.acName, sizeof(props.acName));
			node_table[i].type = props.uNodeType;
		}
	}

	for (i = 0; i < num; i++) {
		printf("%s: %s\n",
		       node_type_to_str(node_table[i].type),
		       node_table[i].name);
	}

	free(node_table);

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
