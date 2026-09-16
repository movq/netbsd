/* $NetBSD: graph.c,v 1.6 2021/06/21 03:09:52 christos Exp $ */

/*
 * Copyright (c) 2009 Precedence Technologies Ltd <support@precedence.co.uk>
 * Copyright (c) 2009 Jared D. McNeill <jmcneill@invisible.ca>
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Precedence Technologies Ltd
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/ioctl.h>

#include <prop/proplib.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <dev/hdaudio/hdaudioio.h>
#include <dev/hdaudio/hdaudioreg.h>

#include "hdaudioctl.h"

int
hdaudioctl_graph(int fd, int argc, char *argv[])
{
	prop_dictionary_t request, response;
	prop_object_iterator_t iter;
	prop_dictionary_t stream;
	prop_number_t nnid;
	prop_array_t connlist, stream_state;
	const char *name;
	bool have_converter_format, have_outamp_left, have_outamp_right;
	bool have_connection_select, have_eapd;
	bool have_pin_ctrl, have_pin_sense, have_power_state;
	bool have_stream_channel;
	int error, index;
	uint32_t bdpl, bdpu, cbl, ctl, lpib;
	uint32_t cap, config, converter_format, outamp_left, outamp_right;
	uint32_t connection_select, eapd, pin_ctrl, pin_sense, power_state;
	uint32_t stream_channel;
	uint16_t fifos, format, lvi;
	uint16_t reqnid, reqcodecid;
	uint16_t vendor, product;
	uint8_t stream_index, stream_sts, stream_tag, stream_type;
	uint8_t type, nid;
	bool allocated;
	char buf[10] = "??h";

	if (argc != 2)
		usage();

	reqcodecid = strtol(argv[0], NULL, 0);
	reqnid = strtol(argv[1], NULL, 0);

	request = prop_dictionary_create();
	if (request == NULL) {
		fprintf(stderr, "out of memory\n");
		return ENOMEM;
	}

	prop_dictionary_set_uint16(request, "codecid", reqcodecid);
	prop_dictionary_set_uint16(request, "nid", reqnid);

	error = prop_dictionary_sendrecv_ioctl(request, fd,
	    HDAUDIO_FGRP_CODEC_INFO, &response);
	if (error != 0) {
		perror("HDAUDIO_FGRP_CODEC_INFO failed");
		prop_object_release(request);
		return error;
	}
	
	prop_dictionary_get_uint16(response, "vendor-id", &vendor);
	prop_dictionary_get_uint16(response, "product-id", &product);
	if (prop_dictionary_get_uint32(response, "power-state",
	    &power_state))
		fprintf(stderr, "afg power=%08X\n", power_state);
	stream_state = prop_dictionary_get(response, "stream-state");
	if (stream_state != NULL) {
		iter = prop_array_iterator(stream_state);
		while ((stream = prop_object_iterator_next(iter)) != NULL) {
			prop_dictionary_get_uint8(stream, "index",
			    &stream_index);
			prop_dictionary_get_bool(stream, "allocated",
			    &allocated);
			prop_dictionary_get_uint8(stream, "type",
			    &stream_type);
			prop_dictionary_get_uint8(stream, "tag", &stream_tag);
			prop_dictionary_get_uint32(stream, "ctl", &ctl);
			prop_dictionary_get_uint8(stream, "sts", &stream_sts);
			prop_dictionary_get_uint32(stream, "lpib", &lpib);
			prop_dictionary_get_uint32(stream, "cbl", &cbl);
			prop_dictionary_get_uint16(stream, "lvi", &lvi);
			prop_dictionary_get_uint16(stream, "fifos", &fifos);
			prop_dictionary_get_uint16(stream, "format", &format);
			prop_dictionary_get_uint32(stream, "bdpl", &bdpl);
			prop_dictionary_get_uint32(stream, "bdpu", &bdpu);
			fprintf(stderr,
			    "stream %u allocated=%u type=%u tag=%u "
			    "ctl=%06X sts=%02X lpib=%u cbl=%u lvi=%u "
			    "fifos=%u format=%04X bdl=%08X:%08X\n",
			    (unsigned int)stream_index,
			    (unsigned int)allocated,
			    (unsigned int)stream_type,
			    (unsigned int)stream_tag, ctl,
			    (unsigned int)stream_sts, lpib, cbl,
			    (unsigned int)lvi, (unsigned int)fifos,
			    (unsigned int)format, bdpu, bdpl);
		}
		prop_object_iterator_release(iter);
	}
	prop_object_release(response);

	printf("digraph \"HD Audio %04X:%04X\" {\n",
	    vendor, product);

	for (index = 0;; index++) {
		prop_dictionary_set_uint16(request, "index", index);
		error = prop_dictionary_sendrecv_ioctl(request, fd,
		    HDAUDIO_FGRP_WIDGET_INFO, &response);
		if (error != 0)
			break;
		prop_dictionary_get_string(response, "name", &name);
		prop_dictionary_get_uint32(response, "cap", &cap);
		prop_dictionary_get_uint32(response, "config", &config);
		prop_dictionary_get_uint8(response, "type", &type);
		prop_dictionary_get_uint8(response, "nid", &nid);

		have_converter_format = prop_dictionary_get_uint32(response,
		    "converter-format", &converter_format);
		have_connection_select = prop_dictionary_get_uint32(response,
		    "connection-select", &connection_select);
		have_eapd = prop_dictionary_get_uint32(response,
		    "eapd", &eapd);
		have_outamp_left = prop_dictionary_get_uint32(response,
		    "outamp-left", &outamp_left);
		have_outamp_right = prop_dictionary_get_uint32(response,
		    "outamp-right", &outamp_right);
		have_pin_ctrl = prop_dictionary_get_uint32(response,
		    "pin-ctrl", &pin_ctrl);
		have_pin_sense = prop_dictionary_get_uint32(response,
		    "pin-sense", &pin_sense);
		have_power_state = prop_dictionary_get_uint32(response,
		    "power-state", &power_state);
		have_stream_channel = prop_dictionary_get_uint32(response,
		    "stream-channel", &stream_channel);

		fprintf(stderr, "nid %02X", nid);
		if (have_power_state)
			fprintf(stderr, " power=%08X", power_state);
		if (have_pin_ctrl)
			fprintf(stderr, " pin-ctrl=%02X", pin_ctrl);
		if (have_pin_sense)
			fprintf(stderr, " pin-sense=%08X", pin_sense);
		if (have_eapd)
			fprintf(stderr, " eapd=%02X", eapd);
		if (have_connection_select)
			fprintf(stderr, " conn=%02X", connection_select);
		if (have_outamp_left)
			fprintf(stderr, " outamp-l=%02X", outamp_left);
		if (have_outamp_right)
			fprintf(stderr, " outamp-r=%02X", outamp_right);
		if (have_stream_channel)
			fprintf(stderr, " stream=%02X", stream_channel);
		if (have_converter_format)
			fprintf(stderr, " format=%04X", converter_format);
		fputc('\n', stderr);

		sprintf(buf, "widget%02Xh", nid);

		switch (type) {
		case COP_AWCAP_TYPE_AUDIO_OUTPUT:
			printf(" %s [label=\"%s\\naudio output\",shape=box,style=filled,fillcolor=\""
			    "#88ff88\"];\n", buf, buf);
			break;
		case COP_AWCAP_TYPE_AUDIO_INPUT:
			printf(" %s [label=\"%s\\naudio input\",shape=box,style=filled,fillcolor=\""
			    "#ff8888\"];\n", buf, buf);
			break;
		case COP_AWCAP_TYPE_AUDIO_MIXER:
			printf(" %s [label=\"%s\\naudio mixer\","
			    "shape=invhouse];\n", buf, buf);
			break;
		case COP_AWCAP_TYPE_AUDIO_SELECTOR:
			printf(" %s [label=\"%s\\naudio selector\","
			    "shape=invtrapezium];\n", buf, buf);
			break;
		case COP_AWCAP_TYPE_PIN_COMPLEX:
			printf(" %s [label=\"%s\\ndevice=%s\",style=filled",
			    buf, buf,
			    pin_devices[COP_CFG_DEFAULT_DEVICE(config)]);
			if (cap & COP_PINCAP_OUTPUT_CAPABLE &&
			    cap & COP_PINCAP_INPUT_CAPABLE)
				puts(",shape=doublecircle,fillcolor=\""
				    "#ffff88\"];");
			else if (cap & COP_PINCAP_OUTPUT_CAPABLE)
				puts(",shape=circle,fillcolor=\"#88ff88\"];");
			else if (cap & COP_PINCAP_INPUT_CAPABLE)
				puts(",shape=circle,fillcolor=\"#ff8888\"];");
			else
				puts(",shape=circle,fillcolor=\"#888888\"];");
			break;
		case COP_AWCAP_TYPE_POWER_WIDGET:
			printf(" %s [label=\"%s\\npower widget\","
			    "shape=box];\n", buf, buf);
			break;
		case COP_AWCAP_TYPE_VOLUME_KNOB:
			printf(" %s [label=\"%s\\nvolume knob\","
			    "shape=box];\n", buf, buf);
			break;
		case COP_AWCAP_TYPE_BEEP_GENERATOR:
			printf(" %s [label=\"%s\\nbeep generator\","
			    "shape=box];\n", buf, buf);
			break;
		case COP_AWCAP_TYPE_VENDOR_DEFINED:
			printf(" %s [label=\"%s\\nvendor defined\","
			    "shape=box];\n", buf, buf);
			break;
		}
		connlist = prop_dictionary_get(response, "connlist");
		if (connlist == NULL)
			goto next;
		iter = prop_array_iterator(connlist);
		prop_object_iterator_reset(iter);
		while ((nnid = prop_object_iterator_next(iter)) != NULL) {
			nid = prop_number_unsigned_value(nnid);
			printf(" widget%02Xh -> %s [sametail=widget%02Xh];\n",
			    nid, buf, nid);
		}
		prop_object_iterator_release(iter);
next:
		prop_object_release(response);
	}

	printf(" {rank=min;");
	for (index = 0;; index++) {
		prop_dictionary_set_uint16(request, "index", index);
		error = prop_dictionary_sendrecv_ioctl(request, fd,
		    HDAUDIO_AFG_WIDGET_INFO, &response);
		if (error != 0)
			break;
		prop_dictionary_get_string(response, "name", &name);
		prop_dictionary_get_uint8(response, "type", &type);
		prop_dictionary_get_uint8(response, "nid", &nid);

		sprintf(buf, "widget%02Xh", nid);

		switch (type) {
		case COP_AWCAP_TYPE_AUDIO_OUTPUT:
		case COP_AWCAP_TYPE_AUDIO_INPUT:
			printf(" %s;", buf);
			break;
		}
		prop_object_release(response);
	}
	printf("}\n");

	printf(" {rank=max;");
	for (index = 0;; index++) {
		prop_dictionary_set_uint16(request, "index", index);
		error = prop_dictionary_sendrecv_ioctl(request, fd,
		    HDAUDIO_AFG_WIDGET_INFO, &response);
		if (error != 0)
			break;
		prop_dictionary_get_string(response, "name", &name);
		prop_dictionary_get_uint8(response, "type", &type);
		prop_dictionary_get_uint8(response, "nid", &nid);

		sprintf(buf, "widget%02Xh", nid);

		switch (type) {
		case COP_AWCAP_TYPE_PIN_COMPLEX:
			printf(" %s;", buf);
			break;
		}
		prop_object_release(response);
	}
	printf("}\n");

	printf("}\n");

	prop_object_release(request);

	return 0;
}
