/*
 *  ThunderGate - an open source toolkit for PCI bus exploration
 *  Copyright (C) 2015  Saul St. John
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "fw.h"

u32 tx_pi;

u32 tx_std_enq(u32 addr_hi, u32 addr_low, u32 len)
{
    u32 i = tx_pi + 1;

    if (!(state.flags & TX_STD_SETUP))
	return 1;

    if (i > 0x1ff)
	i = 0;

    if (i == sbds.con_idx[0])
	return 1;

    txbd[tx_pi].addr_hi = addr_hi;
    txbd[tx_pi].addr_low = addr_low;
    txbd[tx_pi].length = len >> 2;
    txbd[tx_pi].flags.word = 0;
    txbd[tx_pi].flags.packet_end = 1;
    txbd[tx_pi].vlan_tag = 0;
    txbd[tx_pi].hdrlen_0_1 = 0;
    txbd[tx_pi].mss = 1518;
    tx_pi = i;
    lpmb.box[0x30].hi = tx_pi;

    return 0;
}

void tx_std_teardown()
{
	if (!(state.flags & TX_STD_SETUP))
	    return;

	sbds.mode.enable = 0;
	sdi.mode.enable = 0;
	sdc.mode.enable = 0;

	txrcb[0].flags.disabled = 1;

	grc.mode.host_stack_up = 0;

	state.flags &= ~TX_STD_SETUP;
}

void tx_std_setup()
{
	if (!(state.flags & CLOAK_ENGAGED) || (state.flags & TX_STD_SETUP))
		return;

	grc.mode.host_stack_up = 1;
	grc.mode.host_send_bds = 0;

	tx_pi = 0;
	lpmb.box[0x30].hi = tx_pi;

	txrcb[0].addr_hi = 0;
	txrcb[0].addr_low = 0;
	txrcb[0].nic_addr = 0x4000;
	txrcb[0].flags.disabled = 0;

	txrcb[1].flags.disabled = 1;

	sbds.mode.reset = 1;
	sdi.mode.reset = 1;
	sdc.mode.reset = 1;
	while (sbds.mode.reset || sdi.mode.reset || sdc.mode.reset);

	set_and_wait(sdc.mode.enable);
	set_and_wait(sdi.mode.enable);
	set_and_wait(sbds.mode.enable);

	state.flags |= TX_STD_SETUP;
}

void tx_asf(void *_src, u32 len, u16 cmd)
{
    int i = 0;
    u32 buf = 0xad;
    struct mbuf *mb = (struct mbuf *)(0x8000 + (buf << 7));
    u32 sub = buf << 16 | buf;

    u32 blen = len << 2;

    mb->hdr.c = blen > 80 ? 1 : 0;
    mb->hdr.f = 1;
    mb->hdr.length = 80;
    mb->next_frame_ptr = 0;
    mb->hdr.next_mbuf = buf + 1;

    mb->data.frame.status_ctrl = 0;
    
    mb->data.frame.len = (blen + 16) < 64 ? 64 : (blen + 16);
    mb->data.frame.qids = 0xc;
    
    mb->data.frame.mbuf = blen <= 80 ? 1 : (blen <= 200 ? 2 : 3);

    i = sizeof(mb->data.frame);
    mac_cpy(state.dest_mac, &mb->data.byte[i]);
    i += 6;
    mac_cpy(state.my_mac, &mb->data.byte[i]);
    i += 6;
    mb->data.word[i >> 2] = (config.ctrl_etype << 16) | cmd;
    i += 4;

    u8 *src = (u8 *)_src;
    for (; i < 120 && blen > 0; i++, blen--)
	    mb->data.byte[i] = *src++;

    while (i < 104)
	mb->data.byte[i++] = 0;

    if (blen > 0) {
	mb++;
	mb->hdr.c = 1;
	mb->hdr.f = 0;
	mb->hdr.length = blen > 120 ? 120 : blen;
	mb->hdr.next_mbuf = blen > 120 ? buf + 2 : 0;
	for (i = 0; i < 120 && blen > 0; i++, blen--)
		mb->data.byte[i] = *src++;
        sub++;
    }

    if (blen > 0) {
	mb++;
	mb->hdr.c = 1;
	mb->hdr.f = 0;
	mb->hdr.length = blen;
	mb->hdr.next_mbuf = 0;
	for (i = 0;  blen > 0; i++, blen--)
	    mb->data.byte[i] = *src++;
        sub++;
    }

    ftq.mac_tx.q.word = sub;

    state.dest_mac = state.broadcast_mac;
}