/*
*    Copyright (C) 2016-2020 Grok Image Compression Inc.
*
*    This source code is free software: you can redistribute it and/or  modify
*    it under the terms of the GNU Affero General Public License, version 3,
*    as published by the Free Software Foundation.
*
*    This source code is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU Affero General Public License for more details.
*
*    You should have received a copy of the GNU Affero General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*
*    This source code incorporates work covered by the following copyright and
*    permission notice:
*
 * The copyright in this software is being made available under the 2-clauses
 * BSD License, included below. This software may be subject to other third
 * party and contributor rights, including patent rights, and no such rights
 * are granted under this license.
 *
 * Copyright (c) 2002-2014, Universite catholique de Louvain (UCL), Belgium
 * Copyright (c) 2002-2014, Professor Benoit Macq
 * Copyright (c) 2001-2003, David Janssens
 * Copyright (c) 2002-2003, Yannick Verschueren
 * Copyright (c) 2003-2007, Francois-Olivier Devaux
 * Copyright (c) 2003-2014, Antonin Descampe
 * Copyright (c) 2005, Herve Drolon, FreeImage Team
 * Copyright (c) 2008, Jerome Fimes, Communications & Systemes <jerome.fimes@c-s.fr>
 * Copyright (c) 2006-2007, Parvatha Elangovan
 * Copyright (c) 2010-2011, Kaori Hagihara
 * Copyright (c) 2011-2012, Centre National d'Etudes Spatiales (CNES), France
 * Copyright (c) 2012, CS Systemes d'Information, France
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "grok_includes.h"
#include "j2k_static.h"


namespace grk {


bool grk_j2k::decodingTilePartHeader() {
	return (m_specific_param.m_decoder.m_state & J2K_DEC_STATE_TPH);
}
grk_tcp* grk_j2k::get_current_decode_tcp() {
	return (decodingTilePartHeader()) ?
			m_cp.tcps + m_current_tile_number :
				m_specific_param.m_decoder.m_default_tcp;
}


static const j2k_mct_function j2k_mct_read_functions_to_float[] = {
		j2k_read_int16_to_float, j2k_read_int32_to_float,
		j2k_read_float32_to_float, j2k_read_float64_to_float };

static const j2k_mct_function j2k_mct_read_functions_to_int32[] = {
		j2k_read_int16_to_int32, j2k_read_int32_to_int32,
		j2k_read_float32_to_int32, j2k_read_float64_to_int32 };

static const j2k_mct_function j2k_mct_write_functions_from_float[] = {
		j2k_write_float_to_int16, j2k_write_float_to_int32,
		j2k_write_float_to_float, j2k_write_float_to_float64 };

static const  grk_dec_memory_marker_handler  j2k_memory_marker_handler_tab[] = {
		{ J2K_MS_SOT, J2K_DEC_STATE_MH | J2K_DEC_STATE_TPHSOT, j2k_read_sot },
		{ J2K_MS_COD, J2K_DEC_STATE_MH | J2K_DEC_STATE_TPH, j2k_read_cod },
		{ J2K_MS_COC, J2K_DEC_STATE_MH | J2K_DEC_STATE_TPH, j2k_read_coc },
		{ J2K_MS_RGN, J2K_DEC_STATE_MH | J2K_DEC_STATE_TPH, j2k_read_rgn },
		{ J2K_MS_QCD, J2K_DEC_STATE_MH | J2K_DEC_STATE_TPH, j2k_read_qcd },
		{ J2K_MS_QCC, J2K_DEC_STATE_MH | J2K_DEC_STATE_TPH, j2k_read_qcc },
		{ J2K_MS_POC, J2K_DEC_STATE_MH | J2K_DEC_STATE_TPH, j2k_read_poc },
		{J2K_MS_SIZ, J2K_DEC_STATE_MHSIZ, j2k_read_siz },
		{J2K_MS_CAP, J2K_DEC_STATE_MH, j2k_read_cap },
		{J2K_MS_TLM, J2K_DEC_STATE_MH, j2k_read_tlm },
		{J2K_MS_PLM, J2K_DEC_STATE_MH, j2k_read_plm },
		{J2K_MS_PLT, J2K_DEC_STATE_TPH, j2k_read_plt },
		{J2K_MS_PPM, J2K_DEC_STATE_MH, j2k_read_ppm },
		{J2K_MS_PPT, J2K_DEC_STATE_TPH, j2k_read_ppt },
		{J2K_MS_SOP, 0, 0 },
		{J2K_MS_CRG, J2K_DEC_STATE_MH, j2k_read_crg },
		{J2K_MS_COM, J2K_DEC_STATE_MH | J2K_DEC_STATE_TPH, j2k_read_com },
		{J2K_MS_MCT, J2K_DEC_STATE_MH | J2K_DEC_STATE_TPH, j2k_read_mct },
		{J2K_MS_CBD, J2K_DEC_STATE_MH, j2k_read_cbd },
		{J2K_MS_MCC, J2K_DEC_STATE_MH | J2K_DEC_STATE_TPH, j2k_read_mcc },
		{ J2K_MS_MCO, J2K_DEC_STATE_MH | J2K_DEC_STATE_TPH, j2k_read_mco }, {
		J2K_MS_UNK, J2K_DEC_STATE_MH | J2K_DEC_STATE_TPH, 0 }/*j2k_read_unk is directly used*/
};


static const  grk_dec_memory_marker_handler  *  j2k_get_marker_handler(
		uint32_t id) {
	const  grk_dec_memory_marker_handler  *e;
	for (e = j2k_memory_marker_handler_tab; e->id != 0; ++e) {
		if (e->id == id) {
			break; /* we find a handler corresponding to the marker ID*/
		}
	}
	return e;
}

void j2k_destroy(grk_j2k *p_j2k) {
	if (p_j2k == nullptr) {
		return;
	}

	if (p_j2k->m_is_decoder) {

		if (p_j2k->m_specific_param.m_decoder.m_default_tcp != nullptr) {
			j2k_tcp_destroy(p_j2k->m_specific_param.m_decoder.m_default_tcp);
			delete p_j2k->m_specific_param.m_decoder.m_default_tcp;
			p_j2k->m_specific_param.m_decoder.m_default_tcp = nullptr;
		}

		if (p_j2k->m_specific_param.m_decoder.m_marker_scratch != nullptr) {
			grok_free(p_j2k->m_specific_param.m_decoder.m_marker_scratch);
			p_j2k->m_specific_param.m_decoder.m_marker_scratch = nullptr;
			p_j2k->m_specific_param.m_decoder.m_marker_scratch_size = 0;
		}
	} else {

		if (p_j2k->m_specific_param.m_encoder.m_tlm_sot_offsets_buffer) {
			grok_free(
					p_j2k->m_specific_param.m_encoder.m_tlm_sot_offsets_buffer);
			p_j2k->m_specific_param.m_encoder.m_tlm_sot_offsets_buffer =
					nullptr;
			p_j2k->m_specific_param.m_encoder.m_tlm_sot_offsets_current =
					nullptr;
		}
	}

	delete p_j2k->m_tileProcessor;

	p_j2k->m_cp.destroy();
	memset(&(p_j2k->m_cp), 0, sizeof(grk_coding_parameters));

	delete p_j2k->m_procedure_list;
	p_j2k->m_procedure_list = nullptr;

	delete p_j2k->m_validation_list;
	p_j2k->m_validation_list = nullptr;

	j2k_destroy_cstr_index(p_j2k->cstr_index);
	p_j2k->cstr_index = nullptr;

	grk_image_destroy(p_j2k->m_private_image);
	p_j2k->m_private_image = nullptr;

	grk_image_destroy(p_j2k->m_output_image);
	p_j2k->m_output_image = nullptr;

	grok_free(p_j2k);
}

static bool j2k_exec(grk_j2k *p_j2k, std::vector<j2k_procedure> *procs,
		BufferedStream *p_stream) {
	bool l_result = true;

	assert(procs != nullptr);
	assert(p_j2k != nullptr);
	assert(p_stream != nullptr);

	for (auto it = procs->begin(); it != procs->end(); ++it){
		auto p = (bool (*)(grk_j2k *j2k, BufferedStream*))*it;
		l_result = l_result && (p)(p_j2k, p_stream);
	}
	procs->clear();

	return l_result;
}

/*************************************
 * High Level Decode Methods
 ************************************/

static bool j2k_read_header_procedure(grk_j2k *p_j2k, BufferedStream *p_stream) {
	uint32_t l_current_marker;
	uint32_t l_marker_size;
	const  grk_dec_memory_marker_handler  *l_marker_handler = nullptr;
	bool l_has_siz = 0;
	bool l_has_cod = 0;
	bool l_has_qcd = 0;

	assert(p_stream != nullptr);
	assert(p_j2k != nullptr);


	/*  We enter in the main header */
	p_j2k->m_specific_param.m_decoder.m_state = J2K_DEC_STATE_MHSOC;

	/* Try to read the SOC marker, the codestream must begin with SOC marker */
	if (!j2k_read_soc(p_j2k, p_stream)) {
		GROK_ERROR( "Expected a SOC marker ");
		return false;
	}

	/* Try to read 2 bytes (the next marker ID) from stream and copy them into the buffer */
	if (p_stream->read(p_j2k->m_specific_param.m_decoder.m_marker_scratch, 2) != 2) {
		GROK_ERROR( "Stream too short");
		return false;
	}

	/* Read 2 bytes as the new marker ID */
	grk_read_bytes(p_j2k->m_specific_param.m_decoder.m_marker_scratch,
			&l_current_marker, 2);

	/* Try to read until the SOT is detected */
	while (l_current_marker != J2K_MS_SOT) {

		/* Check if the current marker ID is valid */
		if (l_current_marker < 0xff00) {
			GROK_ERROR(
					"A marker ID was expected (0xff--) instead of %.8x\n",
					l_current_marker);
			return false;
		}

		/* Get the marker handler from the marker ID */
		l_marker_handler = j2k_get_marker_handler(l_current_marker);

		/* Manage case where marker is unknown */
		if (l_marker_handler->id == J2K_MS_UNK) {
			GROK_WARN("Unknown marker 0x%02x detected.",l_marker_handler->id );
			if (!j2k_read_unk(p_j2k, p_stream, &l_current_marker)) {
				GROK_ERROR(
						"Unable to read unknown marker 0x%02x.", l_marker_handler->id);
				return false;
			}

			if (l_current_marker == J2K_MS_SOT)
				break; /* SOT marker is detected main header is completely read */
			else
				/* Get the marker handler from the marker ID */
				l_marker_handler = j2k_get_marker_handler(l_current_marker);
		}

		if (l_marker_handler->id == J2K_MS_SIZ) {
			/* Mark required SIZ marker as found */
			l_has_siz = 1;
		}
		if (l_marker_handler->id == J2K_MS_COD) {
			/* Mark required COD marker as found */
			l_has_cod = 1;
		}
		if (l_marker_handler->id == J2K_MS_QCD) {
			/* Mark required QCD marker as found */
			l_has_qcd = 1;
		}

		/* Check if the marker is known and if it is the right place to find it */
		if (!(p_j2k->m_specific_param.m_decoder.m_state
				& l_marker_handler->states)) {
			GROK_ERROR(
					"Marker is not compliant with its position");
			return false;
		}

		/* Try to read 2 bytes (the marker size) from stream and copy them into the buffer */
		if (p_stream->read(p_j2k->m_specific_param.m_decoder.m_marker_scratch, 2) != 2) {
			GROK_ERROR( "Stream too short");
			return false;
		}

		/* read 2 bytes as the marker size */
		grk_read_bytes(p_j2k->m_specific_param.m_decoder.m_marker_scratch,
				&l_marker_size, 2);

		/* Check marker size (does not include marker ID but includes marker size) */
		if (l_marker_size < 2) {
			GROK_ERROR( "Inconsistent marker size");
			return false;
		}

		l_marker_size -= 2; /* Subtract the size of the marker ID already read */

		/* Check if the marker size is compatible with the header data size */
		if (l_marker_size
				> p_j2k->m_specific_param.m_decoder.m_marker_scratch_size) {
			uint8_t *new_header_data = (uint8_t*) grk_realloc(
					p_j2k->m_specific_param.m_decoder.m_marker_scratch,
					l_marker_size);
			if (!new_header_data) {
				grok_free(p_j2k->m_specific_param.m_decoder.m_marker_scratch);
				p_j2k->m_specific_param.m_decoder.m_marker_scratch = nullptr;
				p_j2k->m_specific_param.m_decoder.m_marker_scratch_size = 0;
				GROK_ERROR(
						"Not enough memory to read header");
				return false;
			}
			p_j2k->m_specific_param.m_decoder.m_marker_scratch = new_header_data;
			p_j2k->m_specific_param.m_decoder.m_marker_scratch_size =
					l_marker_size;
		}

		/* Try to read the rest of the marker segment from stream and copy them into the buffer */
		if (p_stream->read(p_j2k->m_specific_param.m_decoder.m_marker_scratch,
				l_marker_size) != l_marker_size) {
			GROK_ERROR( "Stream too short");
			return false;
		}

		/* Read the marker segment with the correct marker handler */
		if (!(*(l_marker_handler->handler))(p_j2k,
				p_j2k->m_specific_param.m_decoder.m_marker_scratch, (uint16_t)l_marker_size)) {
			GROK_ERROR(
					"Marker handler function failed to read the marker segment");
			return false;
		}

		if (p_j2k->cstr_index) {
			/* Add the marker to the codestream index*/
			if (!j2k_add_mhmarker(p_j2k->cstr_index, l_marker_handler->id,
							p_stream->tell() - l_marker_size - 4,
							l_marker_size + 4)) {
				GROK_ERROR(
						"Not enough memory to add mh marker");
				return false;
			}
		}

		/* Try to read 2 bytes (the next marker ID) from stream and copy them into the buffer */
		if (p_stream->read(p_j2k->m_specific_param.m_decoder.m_marker_scratch, 2) != 2) {
			GROK_ERROR( "Stream too short");
			return false;
		}

		/* read 2 bytes as the new marker ID */
		grk_read_bytes(p_j2k->m_specific_param.m_decoder.m_marker_scratch,
				&l_current_marker, 2);
	}
	if (l_has_siz == 0) {
		GROK_ERROR(
				"required SIZ marker not found in main header");
		return false;
	}
	if (l_has_cod == 0) {
		GROK_ERROR(
				"required COD marker not found in main header");
		return false;
	}
	if (l_has_qcd == 0) {
		GROK_ERROR(
				"required QCD marker not found in main header");
		return false;
	}

	if (!j2k_merge_ppm(&(p_j2k->m_cp))) {
		GROK_ERROR( "Failed to merge PPM data");
		return false;
	}
	// event_msg( EVT_INFO, "Main header has been correctly decoded.");
	if (p_j2k->cstr_index) {
		/* Position of the last element if the main header */
		p_j2k->cstr_index->main_head_end = (uint32_t) p_stream->tell() - 2;
	}

	/* Next step: read a tile-part header */
	p_j2k->m_specific_param.m_decoder.m_state = J2K_DEC_STATE_TPHSOT;

	return true;
}

static bool j2k_decoding_validation(grk_j2k *p_j2k, BufferedStream *p_stream) {

	(void) p_stream;
	bool l_is_valid = true;

	/* preconditions*/
	assert(p_j2k != nullptr);
	assert(p_stream != nullptr);


	/* STATE checking */
	/* make sure the state is at 0 */
	l_is_valid &= (p_j2k->m_specific_param.m_decoder.m_state
			== J2K_DEC_STATE_NONE);

	/* POINTER validation */
	/* make sure a p_j2k codec is present */
	/* make sure a procedure list is present */
	l_is_valid &= (p_j2k->m_procedure_list != nullptr);
	/* make sure a validation list is present */
	l_is_valid &= (p_j2k->m_validation_list != nullptr);

	/* PARAMETER VALIDATION */
	return l_is_valid;
}

void j2k_setup_decoder(void *j2k_void,  grk_dparameters  *parameters) {
	grk_j2k *j2k = (grk_j2k*) j2k_void;
	if (j2k && parameters) {
		j2k->m_cp.m_coding_param.m_dec.m_layer = parameters->cp_layer;
		j2k->m_cp.m_coding_param.m_dec.m_reduce = parameters->cp_reduce;
	}
}

/**
 * Sets up the procedures to do on decoding data.
 * Developers wanting to extend the library can add their own reading procedures.
 */
static bool j2k_setup_decoding(grk_j2k *p_j2k) {
	/* preconditions*/
	assert(p_j2k != nullptr);

	p_j2k->m_procedure_list->push_back((j2k_procedure) j2k_decode_tiles);
	// custom procedures here

	return true;
}

bool j2k_end_decompress(grk_j2k *p_j2k, BufferedStream *p_stream) {
	(void) p_j2k;
	(void) p_stream;

	return true;
}

bool j2k_read_header(BufferedStream *p_stream, grk_j2k *p_j2k,
		 grk_header_info  *header_info, grk_image **p_image) {
	assert(p_j2k != nullptr);
	assert(p_stream != nullptr);


	/* create an empty image header */
	p_j2k->m_private_image = grk_image_create0();
	if (!p_j2k->m_private_image) {
		return false;
	}

	/* customization of the validation */
	if (!j2k_setup_decoding_validation(p_j2k)) {
		grk_image_destroy(p_j2k->m_private_image);
		p_j2k->m_private_image = nullptr;
		return false;
	}

	/* validation of the parameters codec */
	if (!j2k_exec(p_j2k, p_j2k->m_validation_list, p_stream)) {
		grk_image_destroy(p_j2k->m_private_image);
		p_j2k->m_private_image = nullptr;
		return false;
	}

	/* customization of the encoding */
	if (!j2k_setup_header_reading(p_j2k)) {
		grk_image_destroy(p_j2k->m_private_image);
		p_j2k->m_private_image = nullptr;
		return false;
	}

	/* read header */
	if (!j2k_exec(p_j2k, p_j2k->m_procedure_list, p_stream)) {
		grk_image_destroy(p_j2k->m_private_image);
		p_j2k->m_private_image = nullptr;
		return false;
	}

	if (header_info) {
		grk_coding_parameters *l_cp = nullptr;
		grk_tcp *l_tcp = nullptr;
		grk_tccp *l_tccp = nullptr;

		l_cp = &(p_j2k->m_cp);
		l_tcp = p_j2k->get_current_decode_tcp();
		l_tccp = &l_tcp->tccps[0];

		header_info->cblockw_init = 1 << l_tccp->cblkw;
		header_info->cblockh_init = 1 << l_tccp->cblkh;
		header_info->irreversible = l_tccp->qmfbid == 0;
		header_info->mct = l_tcp->mct;
		header_info->rsiz = l_cp->rsiz;
		header_info->numresolutions = l_tccp->numresolutions;
		// !!! assume that coding style is constant across all tile components
		header_info->csty = l_tccp->csty;
		// !!! assume that mode switch is constant across all tiles
		header_info->cblk_sty = l_tccp->cblk_sty;
		for (uint32_t i = 0; i < header_info->numresolutions; ++i) {
			header_info->prcw_init[i] = 1 << l_tccp->prcw[i];
			header_info->prch_init[i] = 1 << l_tccp->prch[i];
		}
		header_info->cp_tx0 = p_j2k->m_cp.tx0;
		header_info->cp_ty0 = p_j2k->m_cp.ty0;

		header_info->cp_tdx = p_j2k->m_cp.tdx;
		header_info->cp_tdy = p_j2k->m_cp.tdy;

		header_info->cp_tw = p_j2k->m_cp.tw;
		header_info->cp_th = p_j2k->m_cp.th;

		header_info->tcp_numlayers = l_tcp->numlayers;

		header_info->num_comments = p_j2k->m_cp.num_comments;
		for (size_t i = 0; i < header_info->num_comments; ++i) {
			header_info->comment[i] = p_j2k->m_cp.comment[i];
			header_info->comment_len[i] = p_j2k->m_cp.comment_len[i];
			header_info->isBinaryComment[i] = p_j2k->m_cp.isBinaryComment[i];
		}
	}
	*p_image = grk_image_create0();
	if (!(*p_image)) {
		return false;
	}
	/* Copy codestream image information to the output image */
	grk_copy_image_header(p_j2k->m_private_image, *p_image);
	if (p_j2k->cstr_index) {
		/*Allocate and initialize some elements of codestrem index*/
		if (!j2k_allocate_tile_element_cstr_index(p_j2k)) {
			return false;
		}
	}
	return true;
}

static bool j2k_setup_header_reading(grk_j2k *p_j2k) {
	/* preconditions*/
	assert(p_j2k != nullptr);

	p_j2k->m_procedure_list->push_back((j2k_procedure) j2k_read_header_procedure);
	// custom procedures here
	p_j2k->m_procedure_list->push_back((j2k_procedure) j2k_copy_default_tcp_and_create_tcd);

	return true;
}

static bool j2k_setup_decoding_validation(grk_j2k *p_j2k) {
	/* preconditions*/
	assert(p_j2k != nullptr);

	p_j2k->m_validation_list->push_back((j2k_procedure) j2k_decoding_validation);

	return true;
}


static bool j2k_need_nb_tile_parts_correction(BufferedStream *p_stream,
		uint16_t tile_no, bool *p_correction_needed) {
	uint8_t l_header_data[10];
	int64_t l_stream_pos_backup;
	uint32_t l_current_marker;
	uint32_t l_marker_size;
	uint16_t l_tile_no;
	uint8_t l_current_part, l_num_parts;
	uint32_t l_tot_len;

	/* initialize to no correction needed */
	*p_correction_needed = false;

	if (!p_stream->has_seek()) {
		/* We can't do much in this case, seek is needed */
		return true;
	}

	l_stream_pos_backup = p_stream->tell();
	if (l_stream_pos_backup == -1) {
		/* let's do nothing */
		return true;
	}

	for (;;) {
		/* Try to read 2 bytes (the next marker ID) from stream and copy them into the buffer */
		if (p_stream->read(l_header_data, 2) != 2) {
			/* assume all is OK */
			if (!p_stream->seek(l_stream_pos_backup)) {
				return false;
			}
			return true;
		}

		/* Read 2 bytes from buffer as the new marker ID */
		grk_read_bytes(l_header_data, &l_current_marker, 2);

		if (l_current_marker != J2K_MS_SOT) {
			/* assume all is OK */
			if (!p_stream->seek(l_stream_pos_backup)) {
				return false;
			}
			return true;
		}

		/* Try to read 2 bytes (the marker size) from stream and copy them into the buffer */
		if (p_stream->read(l_header_data, 2) != 2) {
			GROK_ERROR( "Stream too short");
			return false;
		}

		/* Read 2 bytes from the buffer as the marker size */
		grk_read_bytes(l_header_data, &l_marker_size, 2);

		/* Check marker size for SOT Marker */
		if (l_marker_size != 10) {
			GROK_ERROR( "Inconsistent marker size");
			return false;
		}
		l_marker_size -= 2;

		if (p_stream->read(l_header_data, l_marker_size)
				!= l_marker_size) {
			GROK_ERROR( "Stream too short");
			return false;
		}

		if (!j2k_get_sot_values(l_header_data, l_marker_size, &l_tile_no,
				&l_tot_len, &l_current_part, &l_num_parts)) {
			return false;
		}

		if (l_tile_no == tile_no) {
			/* we found what we were looking for */
			break;
		}

		if ((l_tot_len == 0U) || (l_tot_len < 14U)) {
			/* last SOT until EOC or invalid Psot value */
			/* assume all is OK */
			if (!p_stream->seek(l_stream_pos_backup)) {
				return false;
			}
			return true;
		}
		l_tot_len -= 12U;
		/* look for next SOT marker */
		if (!p_stream->skip((int64_t) (l_tot_len))) {
			/* assume all is OK */
			if (!p_stream->seek(l_stream_pos_backup)) {
				return false;
			}
			return true;
		}
	}

	/* check for correction */
	if (l_current_part == l_num_parts) {
		*p_correction_needed = true;
	}

	if (!p_stream->seek(l_stream_pos_backup)) {
		return false;
	}
	return true;
}

bool j2k_read_tile_header(grk_j2k *p_j2k, uint16_t *tile_index,
		uint64_t *data_size, uint32_t *p_tile_x0, uint32_t *p_tile_y0,
		uint32_t *p_tile_x1, uint32_t *p_tile_y1, uint32_t *p_nb_comps,
		bool *p_go_on, BufferedStream *p_stream) {

	uint32_t l_current_marker = J2K_MS_SOT;
	uint32_t l_marker_size = 0;

	assert(p_stream != nullptr);
	assert(p_j2k != nullptr);


	/* Reach the End Of Codestream ?*/
	if (p_j2k->m_specific_param.m_decoder.m_state == J2K_DEC_STATE_EOC) {
		l_current_marker = J2K_MS_EOC;
	}
	/* We need to encounter a SOT marker (a new tile-part header) */
	else if (p_j2k->m_specific_param.m_decoder.m_state
			!= J2K_DEC_STATE_TPHSOT) {
		return false;
	}

	/* Read into the codestream until reach the EOC or ! can_decode ??? FIXME */
	while ((!p_j2k->m_specific_param.m_decoder.ready_to_decode_tile_part_data)
			&& (l_current_marker != J2K_MS_EOC)) {

		/* Try to read until the Start Of Data is detected */
		while (l_current_marker != J2K_MS_SOD) {

			if (p_stream->get_number_byte_left() == 0) {
				p_j2k->m_specific_param.m_decoder.m_state = J2K_DEC_STATE_NEOC;
				GROK_WARN("Missing EOC marker");
				break;
			}

			/* Try to read 2 bytes (the marker size) from stream and copy them into the buffer */
			if (p_stream->read(p_j2k->m_specific_param.m_decoder.m_marker_scratch,
					2) != 2) {
				GROK_ERROR( "Stream too short");
				return false;
			}

			/* Read 2 bytes from the buffer as the marker size */
			grk_read_bytes(p_j2k->m_specific_param.m_decoder.m_marker_scratch,
					&l_marker_size, 2);

			/* Check marker size (does not include marker ID but includes marker size) */
			if (l_marker_size < 2) {
				GROK_ERROR( "Inconsistent marker size");
				return false;
			}

			// subtract tile part header and header marker size
			if (p_j2k->m_specific_param.m_decoder.m_state & J2K_DEC_STATE_TPH) {
				p_j2k->m_specific_param.m_decoder.tile_part_data_length -=
						(l_marker_size + 2);
			}

			l_marker_size -= 2; /* Subtract the size of the marker ID already read */

			/* Get the marker handler from the marker ID */
			auto l_marker_handler = j2k_get_marker_handler(l_current_marker);

			/* Check if the marker is known and if it is the right place to find it */
			if (!(p_j2k->m_specific_param.m_decoder.m_state
					& l_marker_handler->states)) {
				GROK_ERROR(
						"Marker is not compliant with its position");
				return false;
			}

			/* Check if the marker size is compatible with the header data size */
			if (l_marker_size
					> p_j2k->m_specific_param.m_decoder.m_marker_scratch_size) {
				uint8_t *new_header_data = nullptr;
				/* If we are here, this means we consider this marker as known & we will read it */
				/* Check enough bytes left in stream before allocation */
				if (l_marker_size
						> p_stream->get_number_byte_left()) {
					GROK_ERROR(
							"Marker size inconsistent with stream length");
					return false;
				}
				new_header_data = (uint8_t*) grk_realloc(
						p_j2k->m_specific_param.m_decoder.m_marker_scratch,
						l_marker_size);
				if (!new_header_data) {
					grok_free(p_j2k->m_specific_param.m_decoder.m_marker_scratch);
					p_j2k->m_specific_param.m_decoder.m_marker_scratch = nullptr;
					p_j2k->m_specific_param.m_decoder.m_marker_scratch_size = 0;
					GROK_ERROR(
							"Not enough memory to read header");
					return false;
				}
				p_j2k->m_specific_param.m_decoder.m_marker_scratch =
						new_header_data;
				p_j2k->m_specific_param.m_decoder.m_marker_scratch_size =
						l_marker_size;
			}

			/* Try to read the rest of the marker segment from stream and copy them into the buffer */
			if (p_stream->read(p_j2k->m_specific_param.m_decoder.m_marker_scratch,
					l_marker_size) != l_marker_size) {
				GROK_ERROR( "Stream too short");
				return false;
			}

			if (!l_marker_handler->handler) {
				/* See issue #175 */
				GROK_ERROR(
						"Not sure how that happened.");
				return false;
			}
			/* Read the marker segment with the correct marker handler */
			if (!(*(l_marker_handler->handler))(p_j2k,
					p_j2k->m_specific_param.m_decoder.m_marker_scratch,
					(uint16_t)l_marker_size)) {
				GROK_ERROR(
						"Fail to read the current marker segment (%#x)\n",
						l_current_marker);
				return false;
			}

			if (p_j2k->cstr_index) {
				/* Add the marker to the codestream index*/
				if (!j2k_add_tlmarker(p_j2k->m_current_tile_number,
								p_j2k->cstr_index, l_marker_handler->id,
								(uint32_t) p_stream->tell() - l_marker_size - 4,
								l_marker_size + 4)) {
					GROK_ERROR(
							"Not enough memory to add tl marker");
					return false;
				}
			}

			// Cache position of last SOT marker read
			if (l_marker_handler->id == J2K_MS_SOT) {
				uint64_t sot_pos = p_stream->tell() - l_marker_size
						- 4;
				if (sot_pos
						> p_j2k->m_specific_param.m_decoder.m_last_sot_read_pos) {
					p_j2k->m_specific_param.m_decoder.m_last_sot_read_pos =
							sot_pos;
				}
			}

			if (p_j2k->m_specific_param.m_decoder.m_skip_data) {
				// Skip the rest of the tile part header
				if (!p_stream->skip(
						p_j2k->m_specific_param.m_decoder.tile_part_data_length)) {
					GROK_ERROR( "Stream too short");
					return false;
				}
				l_current_marker = J2K_MS_SOD; //We force current marker to equal SOD
			} else {
				while (true) {
					// Try to read 2 bytes (the next marker ID) from stream and copy them into the buffer
					if (p_stream->read(
							p_j2k->m_specific_param.m_decoder.m_marker_scratch, 2) != 2) {
						GROK_ERROR( "Stream too short");
						return false;
					}
					// Read 2 bytes from the buffer as the new marker ID
					grk_read_bytes(p_j2k->m_specific_param.m_decoder.m_marker_scratch,
							&l_current_marker, 2);

					/* Manage case where marker is unknown */
					if (l_current_marker == J2K_MS_UNK) {
						GROK_WARN("Unknown marker 0x%02x detected.",l_current_marker );
						if (!j2k_read_unk(p_j2k, p_stream, &l_current_marker)) {
							GROK_ERROR(
									"Unable to read unknown marker 0x%02x.", l_current_marker);
							return false;
						}
						continue;
					}
					break;
				}
			}
		}

		// no bytes left and no EOC marker : we're done!
		if (!p_stream->get_number_byte_left()
				&& p_j2k->m_specific_param.m_decoder.m_state
						== J2K_DEC_STATE_NEOC)
			break;

		/* If we didn't skip data before, we need to read the SOD marker*/
		if (!p_j2k->m_specific_param.m_decoder.m_skip_data) {
			/* Try to read the SOD marker and skip data ? FIXME */
			if (!j2k_read_sod(p_j2k, p_stream)) {
				return false;
			}
			if (p_j2k->m_specific_param.m_decoder.ready_to_decode_tile_part_data
					&& !p_j2k->m_specific_param.m_decoder.m_nb_tile_parts_correction_checked) {
				/* Issue 254 */
				bool l_correction_needed;

				p_j2k->m_specific_param.m_decoder.m_nb_tile_parts_correction_checked =
						1;
				if (!j2k_need_nb_tile_parts_correction(p_stream,
						p_j2k->m_current_tile_number, &l_correction_needed)) {
					GROK_ERROR(
							"j2k_apply_nb_tile_parts_correction error");
					return false;
				}
				if (l_correction_needed) {
					uint32_t l_nb_tiles = p_j2k->m_cp.tw * p_j2k->m_cp.th;
					uint32_t l_tile_no;
					p_j2k->m_specific_param.m_decoder.ready_to_decode_tile_part_data =
							0;
					p_j2k->m_specific_param.m_decoder.m_nb_tile_parts_correction =
							1;
					/* correct tiles */
					for (l_tile_no = 0U; l_tile_no < l_nb_tiles; ++l_tile_no) {
						if (p_j2k->m_cp.tcps[l_tile_no].m_nb_tile_parts != 0U) {
							p_j2k->m_cp.tcps[l_tile_no].m_nb_tile_parts =
							        (uint8_t)(p_j2k->m_cp.tcps[l_tile_no].m_nb_tile_parts + 1);
						}
					}
					GROK_WARN(
							"Non conformant codestream TPsot==TNsot.");
				}
			}
			if (!p_j2k->m_specific_param.m_decoder.ready_to_decode_tile_part_data) {
				/* Try to read 2 bytes (the next marker ID) from stream and copy them into the buffer */
				if (p_stream->read(
						p_j2k->m_specific_param.m_decoder.m_marker_scratch, 2) != 2) {
					GROK_ERROR( "Stream too short");
					return false;
				}

				/* Read 2 bytes from buffer as the new marker ID */
				grk_read_bytes(p_j2k->m_specific_param.m_decoder.m_marker_scratch,
						&l_current_marker, 2);
			}
		} else {
			/* Indicate we will try to read a new tile-part header*/
			p_j2k->m_specific_param.m_decoder.m_skip_data = 0;
			p_j2k->m_specific_param.m_decoder.ready_to_decode_tile_part_data =
					0;
			p_j2k->m_specific_param.m_decoder.m_state = J2K_DEC_STATE_TPHSOT;

			/* Try to read 2 bytes (the next marker ID) from stream and copy them into the buffer */
			if (p_stream->read(p_j2k->m_specific_param.m_decoder.m_marker_scratch,
					2) != 2) {
				GROK_ERROR( "Stream too short");
				return false;
			}
			/* Read 2 bytes from buffer as the new marker ID */
			grk_read_bytes(p_j2k->m_specific_param.m_decoder.m_marker_scratch,
					&l_current_marker, 2);
		}
	}
	// do QCD marker quantization step size sanity check
	// see page 553 of Taubman and Marcellin for more details on this check
	auto l_tcp = p_j2k->get_current_decode_tcp();
	if (l_tcp->main_qcd_qntsty != J2K_CCP_QNTSTY_SIQNT) {
		auto numComps = p_j2k->m_private_image->numcomps;
		//1. Check main QCD
		uint32_t maxTileDecompositions = 0;
		for (uint32_t k = 0; k < numComps; ++k) {
			auto l_tccp = l_tcp->tccps + k;
			if (l_tccp->numresolutions == 0)
				continue;
			// only consider number of resolutions from a component
			// whose scope is covered by main QCD;
			// ignore components that are out of scope
			// i.e. under main QCC scope, or tile QCD/QCC scope
			if (l_tccp->fromQCC || l_tccp->fromTileHeader)
				continue;
			auto decomps = l_tccp->numresolutions - 1;
			if (maxTileDecompositions < decomps)
				maxTileDecompositions = decomps;
		}
		if ((l_tcp->main_qcd_numStepSizes < 3 * maxTileDecompositions + 1)) {
			GROK_ERROR(
					"From Main QCD marker, "
							"number of step sizes (%d) is less than 3* (tile decompositions) + 1, where tile decompositions = %d \n",
					l_tcp->main_qcd_numStepSizes, maxTileDecompositions);
			return false;
		}

		//2. Check Tile QCD
		grk_tccp *qcd_comp = nullptr;
		for (uint32_t k = 0; k < numComps; ++k) {
			auto l_tccp = l_tcp->tccps + k;
			if (l_tccp->fromTileHeader && !l_tccp->fromQCC) {
				qcd_comp = l_tccp;
				break;
			}
		}
		if (qcd_comp && (qcd_comp->qntsty != J2K_CCP_QNTSTY_SIQNT)) {
			uint32_t maxTileDecompositions = 0;
			for (uint32_t k = 0; k < numComps; ++k) {
				auto l_tccp = l_tcp->tccps + k;
				if (l_tccp->numresolutions == 0)
					continue;
				// only consider number of resolutions from a component
				// whose scope is covered by Tile QCD;
				// ignore components that are out of scope
				// i.e. under Tile QCC scope
				if (l_tccp->fromQCC && l_tccp->fromTileHeader)
					continue;
				auto decomps = l_tccp->numresolutions - 1;
				if (maxTileDecompositions < decomps)
					maxTileDecompositions = decomps;
			}
			if ((qcd_comp->numStepSizes < 3 * maxTileDecompositions + 1)) {
				GROK_ERROR(
						"From Tile QCD marker, "
								"number of step sizes (%d) is less than 3* (tile decompositions) + 1, where tile decompositions = %d \n",
						qcd_comp->numStepSizes, maxTileDecompositions);
				return false;
			}
		}
	}

	/* Current marker is the EOC marker ?*/
	if (l_current_marker == J2K_MS_EOC) {
		if (p_j2k->m_specific_param.m_decoder.m_state != J2K_DEC_STATE_EOC) {
			p_j2k->m_specific_param.m_decoder.m_state = J2K_DEC_STATE_EOC;
			p_j2k->m_current_tile_number = 0;
		}
	}

	//if we are not ready to decode tile part data, then skip tiles with no tile data
	// !! Why ???
	if (!p_j2k->m_specific_param.m_decoder.ready_to_decode_tile_part_data) {
		uint32_t l_nb_tiles = p_j2k->m_cp.th * p_j2k->m_cp.tw;
		l_tcp = p_j2k->m_cp.tcps + p_j2k->m_current_tile_number;
		while ((p_j2k->m_current_tile_number < l_nb_tiles)
				&& (!l_tcp->m_tile_data)) {
			++p_j2k->m_current_tile_number;
			++l_tcp;
		}
		if (p_j2k->m_current_tile_number == l_nb_tiles) {
			*p_go_on = false;
			return true;
		}
	}

	if (!j2k_merge_ppt(p_j2k->m_cp.tcps + p_j2k->m_current_tile_number)) {
		GROK_ERROR( "Failed to merge PPT data");
		return false;
	}
	if (!p_j2k->m_tileProcessor->init_decode_tile( p_j2k->m_output_image,
			p_j2k->m_current_tile_number)) {
		GROK_ERROR( "Cannot decode tile %d\n",
				p_j2k->m_current_tile_number);
		return false;
	}
	*tile_index = p_j2k->m_current_tile_number;
	*p_go_on = true;
	*data_size = p_j2k->m_tileProcessor->get_tile_size(true);
	*p_tile_x0 = p_j2k->m_tileProcessor->tile->x0;
	*p_tile_y0 = p_j2k->m_tileProcessor->tile->y0;
	*p_tile_x1 = p_j2k->m_tileProcessor->tile->x1;
	*p_tile_y1 = p_j2k->m_tileProcessor->tile->y1;
	*p_nb_comps = p_j2k->m_tileProcessor->tile->numcomps;
	p_j2k->m_specific_param.m_decoder.m_state |= J2K_DEC_STATE_DATA;
	return true;
}

bool j2k_decode_tile(grk_j2k *p_j2k, uint16_t tile_index, uint8_t *p_data,
		uint64_t data_size, BufferedStream *p_stream) {
	assert(p_stream != nullptr);
	assert(p_j2k != nullptr);

	if (!(p_j2k->m_specific_param.m_decoder.m_state & J2K_DEC_STATE_DATA)
			|| (tile_index != p_j2k->m_current_tile_number)) {
		return false;
	}

	auto l_tcp = p_j2k->m_cp.tcps + tile_index;
	if (!l_tcp->m_tile_data) {
		j2k_tcp_destroy(l_tcp);
		return false;
	}

	if (!p_j2k->m_tileProcessor->decode_tile(l_tcp->m_tile_data, tile_index)) {
		j2k_tcp_destroy(l_tcp);
		p_j2k->m_specific_param.m_decoder.m_state |= J2K_DEC_STATE_ERR;
		GROK_ERROR( "Failed to decode.");
		return false;
	}

	if (!p_j2k->m_tileProcessor->current_plugin_tile
			|| (p_j2k->m_tileProcessor->current_plugin_tile->decode_flags
					& GROK_DECODE_POST_T1)) {

		/* if p_data is not null, then copy decoded resolutions from tile data into p_data.
		 Otherwise, simply copy tile data pointer to output image
		 */
		if (p_data) {
			if (!p_j2k->m_tileProcessor->update_tile_data(p_data, data_size)) {
				return false;
			}
		} else {
			/* transfer data from tile component to output image */
			uint32_t compno = 0;
			for (compno = 0; compno < p_j2k->m_output_image->numcomps;
					compno++) {
				auto tilec = p_j2k->m_tileProcessor->tile->comps + compno;
				auto comp = p_j2k->m_output_image->comps + compno;

				//transfer memory from tile component to output image
				comp->data = tilec->buf->get_ptr( 0, 0, 0, 0);
				comp->owns_data = tilec->buf->owns_data;
				tilec->buf->data = nullptr;
				tilec->buf->owns_data = false;

				comp->resno_decoded =
						p_j2k->m_tileProcessor->image->comps[compno].resno_decoded;
			}
		}
		/* we only destroy the data, which will be re-read in read_tile_header*/
		delete l_tcp->m_tile_data;
		l_tcp->m_tile_data = nullptr;

		p_j2k->m_specific_param.m_decoder.ready_to_decode_tile_part_data = 0;
		p_j2k->m_specific_param.m_decoder.m_state &= (~(J2K_DEC_STATE_DATA));

		// if there is no EOC marker and there is also no data left, then simply return true
		if (p_stream->get_number_byte_left() == 0
				&& p_j2k->m_specific_param.m_decoder.m_state
						== J2K_DEC_STATE_NEOC) {
			return true;
		}
		// if EOC marker has not been read yet, then try to read the next marker (should be EOC or SOT)
		if (p_j2k->m_specific_param.m_decoder.m_state != J2K_DEC_STATE_EOC) {

			uint8_t l_data[2];
			// not enough data for another marker
			if (p_stream->read(l_data, 2) != 2) {
				GROK_WARN("j2k_decode_tile: Not enough data to read another marker. Tile may be truncated.");
				return true;
			}

			uint32_t l_current_marker=0;
			// read marker
			grk_read_bytes(l_data, &l_current_marker, 2);

			switch(l_current_marker){
			// we found the EOC marker - set state accordingly and return true;
			// we can ignore all data after EOC
			case J2K_MS_EOC:
				p_j2k->m_current_tile_number = 0;
				p_j2k->m_specific_param.m_decoder.m_state = J2K_DEC_STATE_EOC;
				return true;
				break;
			// start of another tile
			case J2K_MS_SOT:
				return true;
				break;
			default:
				{
				auto bytesLeft = p_stream->get_number_byte_left();
				// no bytes left - file ends without EOC marker
				if (bytesLeft == 0) {
					p_j2k->m_specific_param.m_decoder.m_state =
							J2K_DEC_STATE_NEOC;
					GROK_WARN("Stream does not end with EOC");
					return true;
				}
				GROK_WARN("Decode tile: expected EOC or SOT "
						"but found unknown \"marker\" %x. \n",
						l_current_marker);
				throw DecodeUnknownMarkerAtEndOfTileException();
				}
				break;
			}
		}
	}

	return true;
}

bool j2k_set_decode_area(grk_j2k *p_j2k, grk_image *p_image, uint32_t start_x,
		uint32_t start_y, uint32_t end_x, uint32_t end_y) {

	auto l_cp = &(p_j2k->m_cp);
	auto l_image = p_j2k->m_private_image;
	auto tp = p_j2k->m_tileProcessor;
	auto decoder = &p_j2k->m_specific_param.m_decoder;

	/* Check if we have read the main header */
	if (decoder->m_state != J2K_DEC_STATE_TPHSOT) {
		GROK_ERROR(
				"Need to decode the main header before setting decode area");
		return false;
	}

	if (!start_x && !start_y && !end_x && !end_y) {
		//event_msg( EVT_INFO, "No decoded area parameters, set the decoded area to the whole image");

		decoder->m_start_tile_x = 0;
		decoder->m_start_tile_y = 0;
		decoder->m_end_tile_x = l_cp->tw;
		decoder->m_end_tile_y = l_cp->th;

		return true;
	}

	/* ----- */
	/* Check if the positions provided by the user are correct */

	/* Left */
	if (start_x > l_image->x1) {
		GROK_ERROR("Left position of the decoded area (region_x0=%d)"
				" is outside the image area (Xsiz=%d).\n",
				start_x, l_image->x1);
		return false;
	} else if (start_x < l_image->x0) {
		GROK_WARN(
				"Left position of the decoded area (region_x0=%d)"
				" is outside the image area (XOsiz=%d).\n",
				start_x, l_image->x0);
		decoder->m_start_tile_x = 0;
		p_image->x0 = l_image->x0;
	} else {
		decoder->m_start_tile_x = (start_x
				- l_cp->tx0) / l_cp->tdx;
		p_image->x0 = start_x;
	}

	/* Up */
	if (start_y > l_image->y1) {
		GROK_ERROR(
				"Up position of the decoded area (region_y0=%d)"
				" is outside the image area (Ysiz=%d).\n",
				start_y, l_image->y1);
		return false;
	} else if (start_y < l_image->y0) {
		GROK_WARN(
				"Up position of the decoded area (region_y0=%d)"
				" is outside the image area (YOsiz=%d).\n",
				start_y, l_image->y0);
		decoder->m_start_tile_y = 0;
		p_image->y0 = l_image->y0;
	} else {
		decoder->m_start_tile_y = (start_y
				- l_cp->ty0) / l_cp->tdy;
		p_image->y0 = start_y;
	}

	/* Right */
	assert(end_x > 0);
	assert(end_y > 0);
	if (end_x < l_image->x0) {
		GROK_ERROR(
				"Right position of the decoded area (region_x1=%d)"
				" is outside the image area (XOsiz=%d).\n",
				end_x, l_image->x0);
		return false;
	} else if (end_x > l_image->x1) {
		GROK_WARN(
				"Right position of the decoded area (region_x1=%d)"
				" is outside the image area (Xsiz=%d).\n",
				end_x, l_image->x1);
		decoder->m_end_tile_x = l_cp->tw;
		p_image->x1 = l_image->x1;
	} else {
		// avoid divide by zero
		if (l_cp->tdx == 0) {
			return false;
		}
		decoder->m_end_tile_x = ceildiv<uint32_t>(
				end_x - l_cp->tx0, l_cp->tdx);
		p_image->x1 = end_x;
	}

	/* Bottom */
	if (end_y < l_image->y0) {
		GROK_ERROR(
				"Bottom position of the decoded area (region_y1=%d)"
				" is outside the image area (YOsiz=%d).\n",
				end_y, l_image->y0);
		return false;
	}
	if (end_y > l_image->y1) {
		GROK_WARN(
				"Bottom position of the decoded area (region_y1=%d)"
				" is outside the image area (Ysiz=%d).\n",
				end_y, l_image->y1);
		decoder->m_end_tile_y = l_cp->th;
		p_image->y1 = l_image->y1;
	} else {
		// avoid divide by zero
		if (l_cp->tdy == 0) {
			return false;
		}
		decoder->m_end_tile_y = ceildiv<uint32_t>(
				end_y - l_cp->ty0, l_cp->tdy);
		p_image->y1 = end_y;
	}
	/* ----- */

	decoder->m_discard_tiles = 1;
	tp->whole_tile_decoding = false;
	if (!update_image_dimensions(p_image, l_cp->m_coding_param.m_dec.m_reduce))
		return false;

	GROK_INFO("Setting decoding area to %d,%d,%d,%d\n",
	               p_image->x0, p_image->y0, p_image->x1, p_image->y1);
	return true;
}

grk_j2k* j2k_create_decompress(void) {
	grk_j2k *j2k = (grk_j2k*) grk_calloc(1, sizeof(grk_j2k));
	if (!j2k) {
		return nullptr;
	}

	j2k->m_is_decoder = 1;
	j2k->m_cp.m_is_decoder = 1;

#ifdef GRK_DISABLE_TPSOT_FIX
    j2k->m_coding_param.m_decoder.m_nb_tile_parts_correction_checked = 1;
#endif

	j2k->m_specific_param.m_decoder.m_default_tcp = new grk_tcp();
	if (!j2k->m_specific_param.m_decoder.m_default_tcp) {
		j2k_destroy(j2k);
		return nullptr;
	}

	j2k->m_specific_param.m_decoder.m_marker_scratch = (uint8_t*) grk_calloc(1,
			default_header_size);
	if (!j2k->m_specific_param.m_decoder.m_marker_scratch) {
		j2k_destroy(j2k);
		return nullptr;
	}

	j2k->m_specific_param.m_decoder.m_marker_scratch_size = default_header_size;

	j2k->m_specific_param.m_decoder.m_tile_ind_to_dec = -1;

	j2k->m_specific_param.m_decoder.m_last_sot_read_pos = 0;

	/* codestream index creation */
	j2k->cstr_index = j2k_create_cstr_index();
	if (!j2k->cstr_index) {
		j2k_destroy(j2k);
		return nullptr;
	}

	/* validation list creation */
	j2k->m_validation_list = new std::vector<j2k_procedure>();

	/* execution list creation */
	j2k->m_procedure_list = new std::vector<j2k_procedure>();

	return j2k;
}


static bool j2k_decode_tiles(grk_j2k *p_j2k, BufferedStream *p_stream) {
	bool go_on = true;
	uint16_t current_tile_no = 0;
	uint64_t data_size = 0, max_data_size = 0;
	uint32_t nb_comps = 0;
	uint8_t *current_data = nullptr;
	uint32_t nr_tiles = 0;
	uint32_t num_tiles_to_decode = p_j2k->m_cp.th * p_j2k->m_cp.tw;
	bool clearOutputOnInit = false;
	// if number of tiles is greater than 1, then we need to copy tile data
	if (num_tiles_to_decode > 1) {
		current_data = (uint8_t*) grk_malloc(1);
		if (!current_data) {
			GROK_ERROR(
					"Not enough memory to decode tiles");
			return false;
		}
		max_data_size = 1;
		clearOutputOnInit = num_tiles_to_decode > 1;
	}
	uint32_t num_tiles_decoded = 0;

	for (nr_tiles = 0; nr_tiles < num_tiles_to_decode; nr_tiles++) {
		uint32_t tile_x0, tile_y0, tile_x1, tile_y1;
		tile_x0 = tile_y0 = tile_x1 = tile_y1 = 0;
		if (!j2k_read_tile_header(p_j2k, &current_tile_no, &data_size,
				&tile_x0, &tile_y0, &tile_x1, &tile_y1, &nb_comps,
				&go_on, p_stream)) {
			if (current_data)
				grok_free(current_data);
			return false;
		}

		if (!go_on) {
			break;
		}

		if (current_data && (data_size > max_data_size)) {
			uint8_t *new_current_data = (uint8_t*) grk_realloc(
					current_data, data_size);
			if (!new_current_data) {
				grok_free(current_data);
				GROK_ERROR(
						"Not enough memory to decode tile %d/%d\n",
						current_tile_no + 1, num_tiles_to_decode);
				return false;
			}
			current_data = new_current_data;
			max_data_size = data_size;
		}

		try {
			if (!j2k_decode_tile(p_j2k, current_tile_no, current_data,
					data_size, p_stream)) {
				if (current_data)
					grok_free(current_data);
				GROK_ERROR( "Failed to decode tile %d/%d\n",
						current_tile_no + 1, num_tiles_to_decode);
				return false;
			}
		} catch (DecodeUnknownMarkerAtEndOfTileException &e) {
			// only worry about exception if we have more tiles to decode
			if (nr_tiles < num_tiles_to_decode - 1) {
				GROK_ERROR(
						"Stream too short, expected SOT");
				if (current_data)
					grok_free(current_data);
				GROK_ERROR( "Failed to decode tile %d/%d\n",
						current_tile_no + 1, num_tiles_to_decode);
				return false;
			}
		}
		//event_msg( EVT_INFO, "Tile %d/%d has been decoded.\n", current_tile_no +1, num_tiles_to_decode);

		/* copy from current data to output image, if necessary */
		if (current_data) {
			if (!p_j2k->m_tileProcessor->copy_decoded_tile_to_output_image(
					current_data, p_j2k->m_output_image, clearOutputOnInit)) {
				grok_free(current_data);
				return false;
			}
			// event_msg( EVT_INFO, "Image data has been updated with tile %d.\n\n", current_tile_no + 1);
		}

		num_tiles_decoded++;

		if (p_stream->get_number_byte_left() == 0
				&& p_j2k->m_specific_param.m_decoder.m_state
						== J2K_DEC_STATE_NEOC)
			break;
	}

	if (current_data) {
		grok_free(current_data);
	}

	if (num_tiles_decoded == 0) {
		GROK_ERROR( "No tiles were decoded. Exiting");
		return false;
	} else if (num_tiles_decoded < num_tiles_to_decode) {
		GROK_WARN(
				"Only %d out of %d tiles were decoded\n", num_tiles_decoded,
				num_tiles_to_decode);
		return true;
	}
	return true;
}

/*
 * Read and decode one tile.
 */
static bool j2k_decode_one_tile(grk_j2k *p_j2k, BufferedStream *p_stream) {
	bool go_on = true;
	uint16_t current_tile_no;
	uint32_t tile_no_to_dec;
	uint64_t data_size = 0, max_data_size = 0;
	uint32_t tile_x0, tile_y0, tile_x1, tile_y1;
	uint32_t nb_comps;
	uint8_t *current_data = nullptr;

	/*Allocate and initialize some elements of codestream index if not already done*/
	if (!p_j2k->cstr_index->tile_index) {
		if (!j2k_allocate_tile_element_cstr_index(p_j2k)) {
			if (current_data)
				grok_free(current_data);
			return false;
		}
	}
	/* Move into the codestream to the first SOT used to decode the desired tile */
	tile_no_to_dec = p_j2k->m_specific_param.m_decoder.m_tile_ind_to_dec;
	if (p_j2k->cstr_index->tile_index)
		if (p_j2k->cstr_index->tile_index->tp_index) {
			if (!p_j2k->cstr_index->tile_index[tile_no_to_dec].nb_tps) {
				/* the index for this tile has not been built,
				 *  so move to the last SOT read */
				if (!(p_stream->seek(
						p_j2k->m_specific_param.m_decoder.m_last_sot_read_pos
								+ 2))) {
					GROK_ERROR(
							"Problem with seek function");
					if (current_data)
						grok_free(current_data);
					return false;
				}
			} else {
				if (!(p_stream->seek(
						p_j2k->cstr_index->tile_index[tile_no_to_dec].tp_index[0].start_pos
								+ 2))) {
					GROK_ERROR(
							"Problem with seek function");
					if (current_data)
						grok_free(current_data);
					return false;
				}
			}
			/* Special case if we have previously read the EOC marker (if the previous tile decoded is the last ) */
			if (p_j2k->m_specific_param.m_decoder.m_state == J2K_DEC_STATE_EOC)
				p_j2k->m_specific_param.m_decoder.m_state =
						J2K_DEC_STATE_TPHSOT;
		}

	for (;;) {
		if (!j2k_read_tile_header(p_j2k, &current_tile_no, &data_size,
				&tile_x0, &tile_y0, &tile_x1, &tile_y1, &nb_comps,
				&go_on, p_stream)) {
			if (current_data)
				grok_free(current_data);
			return false;
		}

		if (!go_on) {
			break;
		}

		if (current_data && data_size > max_data_size) {
			uint8_t *new_current_data = (uint8_t*) grk_realloc(
					current_data, data_size);
			if (!new_current_data) {
				grok_free(current_data);
				current_data = nullptr;
				GROK_ERROR(
						"Not enough memory to decode tile %d/%d\n",
						current_tile_no + 1, p_j2k->m_cp.th * p_j2k->m_cp.tw);
				return false;
			}
			current_data = new_current_data;
			max_data_size = data_size;
		}

		try {
			if (!j2k_decode_tile(p_j2k, current_tile_no, current_data,
					data_size, p_stream)) {
				if (current_data)
					grok_free(current_data);
				return false;
			}
		} catch (DecodeUnknownMarkerAtEndOfTileException &e) {
			// suppress exception
		}
		//event_msg( EVT_INFO, "Tile %d/%d has been decoded.\n", current_tile_no+1, p_j2k->m_cp.th * p_j2k->m_cp.tw);

		if (current_data) {
			if (!p_j2k->m_tileProcessor->copy_decoded_tile_to_output_image(
					current_data, p_j2k->m_output_image, false)) {
				grok_free(current_data);
				return false;
			}
		}
		//event_msg( EVT_INFO, "Image data has been updated with tile %d.\n\n", current_tile_no+1);
		if (current_tile_no == tile_no_to_dec) {
			/* move into the codestream to the first SOT (FIXME or not move?)*/
			if (!(p_stream->seek(p_j2k->cstr_index->main_head_end + 2))) {
				GROK_ERROR( "Problem with seek function");
				if (current_data)
					grok_free(current_data);
				return false;
			}
			break;
		} else {
			GROK_WARN(
					"Tile read, decoded and updated is not the desired one (%d vs %d).\n",
					current_tile_no + 1, tile_no_to_dec + 1);
		}

	}
	if (current_data)
		grok_free(current_data);
	return true;
}

/**
 * Sets up the procedures to do on decoding one tile.
 *  Developers wanting to extend the library can add their own reading procedures.
 */
static bool j2k_setup_decoding_tile(grk_j2k *p_j2k) {
	/* preconditions*/
	assert(p_j2k != nullptr);
	p_j2k->m_procedure_list->push_back((j2k_procedure) j2k_decode_one_tile);
	//custom procedures here

	return true;
}

bool j2k_decode(grk_j2k *p_j2k, grk_plugin_tile *tile, BufferedStream *p_stream,
		grk_image *p_image) {
	if (!p_image)
		return false;

	p_j2k->m_output_image = grk_image_create0();
	if (!(p_j2k->m_output_image)) {
		return false;
	}
	grk_copy_image_header(p_image, p_j2k->m_output_image);

	/* customization of the decoding */
	if (!j2k_setup_decoding(p_j2k))
		return false;
	p_j2k->m_tileProcessor->current_plugin_tile = tile;

	/* Decode the codestream */
	if (!j2k_exec(p_j2k, p_j2k->m_procedure_list, p_stream)) {
		grk_image_destroy(p_j2k->m_private_image);
		p_j2k->m_private_image = nullptr;
		return false;
	}

	/* Move data and information from codec output image to user output image*/
	transfer_image_data(p_j2k->m_output_image, p_image);
	return true;
}

bool j2k_get_tile(grk_j2k *p_j2k, BufferedStream *p_stream, grk_image *p_image, uint16_t tile_index) {
	uint32_t compno;
	uint32_t tile_x, tile_y;
	 grk_image_comp  *img_comp;
	grk_rect originaimage_rect, tile_rect, overlap_rect;

	if (!p_image) {
		GROK_ERROR(
				"We need an image previously created.");
		return false;
	}

	if ( /*(tile_index < 0) &&*/(tile_index >= p_j2k->m_cp.tw * p_j2k->m_cp.th)) {
		GROK_ERROR(
				"Tile index provided by the user is incorrect %d (max = %d) \n",
				tile_index, (p_j2k->m_cp.tw * p_j2k->m_cp.th) - 1);
		return false;
	}

	/* Compute the dimension of the desired tile*/
	tile_x = tile_index % p_j2k->m_cp.tw;
	tile_y = tile_index / p_j2k->m_cp.tw;

	originaimage_rect = grk_rect(p_image->x0, p_image->y0, p_image->x1,
			p_image->y1);

	p_image->x0 = tile_x * p_j2k->m_cp.tdx + p_j2k->m_cp.tx0;
	if (p_image->x0 < p_j2k->m_private_image->x0)
		p_image->x0 = p_j2k->m_private_image->x0;
	p_image->x1 = (tile_x + 1) * p_j2k->m_cp.tdx + p_j2k->m_cp.tx0;
	if (p_image->x1 > p_j2k->m_private_image->x1)
		p_image->x1 = p_j2k->m_private_image->x1;

	p_image->y0 = tile_y * p_j2k->m_cp.tdy + p_j2k->m_cp.ty0;
	if (p_image->y0 < p_j2k->m_private_image->y0)
		p_image->y0 = p_j2k->m_private_image->y0;
	p_image->y1 = (tile_y + 1) * p_j2k->m_cp.tdy + p_j2k->m_cp.ty0;
	if (p_image->y1 > p_j2k->m_private_image->y1)
		p_image->y1 = p_j2k->m_private_image->y1;

	tile_rect.x0 = p_image->x0;
	tile_rect.y0 = p_image->y0;
	tile_rect.x1 = p_image->x1;
	tile_rect.y1 = p_image->y1;

	if (originaimage_rect.is_non_degenerate() && tile_rect.is_non_degenerate()
			&& originaimage_rect.clip(tile_rect, &overlap_rect)
			&& overlap_rect.is_non_degenerate()) {
		p_image->x0 = (uint32_t) overlap_rect.x0;
		p_image->y0 = (uint32_t) overlap_rect.y0;
		p_image->x1 = (uint32_t) overlap_rect.x1;
		p_image->y1 = (uint32_t) overlap_rect.y1;
	} else {
		GROK_WARN(
				"Decode region <%d,%d,%d,%d> does not overlap requested tile %d. Ignoring.\n",
				originaimage_rect.x0, originaimage_rect.y0,
				originaimage_rect.x1, originaimage_rect.y1, tile_index);
	}

	img_comp = p_image->comps;
	auto reduce = p_j2k->m_cp.m_coding_param.m_dec.m_reduce;
	for (compno = 0; compno < p_image->numcomps; ++compno) {
		uint32_t comp_x1, comp_y1;

		img_comp->x0 = ceildiv<uint32_t>(p_image->x0, img_comp->dx);
		img_comp->y0 = ceildiv<uint32_t>(p_image->y0, img_comp->dy);
		comp_x1 = ceildiv<uint32_t>(p_image->x1, img_comp->dx);
		comp_y1 = ceildiv<uint32_t>(p_image->y1, img_comp->dy);

		img_comp->w = (uint_ceildivpow2(comp_x1,reduce)
				- uint_ceildivpow2(img_comp->x0,	reduce));
		img_comp->h = (uint_ceildivpow2(comp_y1,reduce)
				- uint_ceildivpow2(img_comp->y0,	reduce));

		img_comp++;
	}

	/* Destroy the previous output image*/
	if (p_j2k->m_output_image)
		grk_image_destroy(p_j2k->m_output_image);

	/* Create the output image from the information previously computed*/
	p_j2k->m_output_image = grk_image_create0();
	if (!(p_j2k->m_output_image)) {
		return false;
	}
	grk_copy_image_header(p_image, p_j2k->m_output_image);

	p_j2k->m_specific_param.m_decoder.m_tile_ind_to_dec = (int32_t) tile_index;

	// reset tile part numbers, in case we are re-using the same codec object from previous decode
	uint32_t nb_tiles = p_j2k->m_cp.tw * p_j2k->m_cp.th;
	for (uint32_t i = 0; i < nb_tiles; ++i) {
		p_j2k->m_cp.tcps[i].m_current_tile_part_number = -1;
	}

	/* customization of the decoding */
	if (!j2k_setup_decoding_tile(p_j2k))
		return false;

	/* Decode the codestream */
	if (!j2k_exec(p_j2k, p_j2k->m_procedure_list, p_stream)) {
		grk_image_destroy(p_j2k->m_private_image);
		p_j2k->m_private_image = nullptr;
		return false;
	}

	/* Move data information from codec output image to user output image*/
	transfer_image_data(p_j2k->m_output_image, p_image);

	return true;
}

static void j2k_tcp_destroy(grk_tcp *p_tcp) {
	if (p_tcp == nullptr) {
		return;
	}
	if (p_tcp->ppt_markers != nullptr) {
		uint32_t i;
		for (i = 0U; i < p_tcp->ppt_markers_count; ++i) {
			if (p_tcp->ppt_markers[i].m_data != nullptr) {
				grok_free(p_tcp->ppt_markers[i].m_data);
			}
		}
		p_tcp->ppt_markers_count = 0U;
		grok_free(p_tcp->ppt_markers);
		p_tcp->ppt_markers = nullptr;
	}

	if (p_tcp->ppt_buffer != nullptr) {
		grok_free(p_tcp->ppt_buffer);
		p_tcp->ppt_buffer = nullptr;
	}

	if (p_tcp->tccps != nullptr) {
		grok_free(p_tcp->tccps);
		p_tcp->tccps = nullptr;
	}

	if (p_tcp->m_mct_coding_matrix != nullptr) {
		grok_free(p_tcp->m_mct_coding_matrix);
		p_tcp->m_mct_coding_matrix = nullptr;
	}

	if (p_tcp->m_mct_decoding_matrix != nullptr) {
		grok_free(p_tcp->m_mct_decoding_matrix);
		p_tcp->m_mct_decoding_matrix = nullptr;
	}

	if (p_tcp->m_mcc_records) {
		grok_free(p_tcp->m_mcc_records);
		p_tcp->m_mcc_records = nullptr;
		p_tcp->m_nb_max_mcc_records = 0;
		p_tcp->m_nb_mcc_records = 0;
	}

	if (p_tcp->m_mct_records) {
		grk_mct_data *l_mct_data = p_tcp->m_mct_records;
		uint32_t i;

		for (i = 0; i < p_tcp->m_nb_mct_records; ++i) {
			if (l_mct_data->m_data) {
				grok_free(l_mct_data->m_data);
				l_mct_data->m_data = nullptr;
			}

			++l_mct_data;
		}

		grok_free(p_tcp->m_mct_records);
		p_tcp->m_mct_records = nullptr;
	}

	if (p_tcp->mct_norms != nullptr) {
		grok_free(p_tcp->mct_norms);
		p_tcp->mct_norms = nullptr;
	}

	delete p_tcp->m_tile_data;
	p_tcp->m_tile_data = nullptr;

}


/************************************************
 * High level Encode Methods
 ***********************************************/

grk_j2k* j2k_create_compress(void) {
	grk_j2k *j2k = (grk_j2k*) grk_calloc(1, sizeof(grk_j2k));
	if (!j2k) {
		return nullptr;
	}

	j2k->m_is_decoder = 0;
	j2k->m_cp.m_is_decoder = 0;

	/* validation list creation*/
	j2k->m_validation_list = new std::vector<j2k_procedure>();

	/* execution list creation*/
	j2k->m_procedure_list = new std::vector<j2k_procedure>();

	return j2k;
}

bool j2k_setup_encoder(grk_j2k *p_j2k,  grk_cparameters  *parameters,
		grk_image *image) {
	uint32_t i, j, tileno, numpocs_tile;
	grk_coding_parameters *cp = nullptr;

	if (!p_j2k || !parameters || !image) {
		return false;
	}
	//sanity check on image
	if (image->numcomps < 1 || image->numcomps > max_num_components) {
		GROK_ERROR(
				"Invalid number of components specified while setting up JP2 encoder");
		return false;
	}
	if ((image->x1 < image->x0) || (image->y1 < image->y0)) {
		GROK_ERROR(
				"Invalid input image dimensions found while setting up JP2 encoder");
		return false;
	}
	for (i = 0; i < image->numcomps; ++i) {
		auto comp = image->comps + i;
		if (comp->w == 0 || comp->h == 0) {
			GROK_ERROR(
					"Invalid input image component dimensions found while setting up JP2 encoder");
			return false;
		}
		if (comp->prec == 0) {
			GROK_ERROR(
					"Invalid component precision of 0 found while setting up JP2 encoder");
			return false;
		}
	}

	if ((parameters->numresolution == 0)
			|| (parameters->numresolution > GRK_J2K_MAXRLVLS)) {
		GROK_ERROR(
				"Invalid number of resolutions : %d not in range [1,%d]\n",
				parameters->numresolution, GRK_J2K_MAXRLVLS);
		return false;
	}

    if (GRK_IS_IMF(parameters->rsiz) && parameters->max_cs_size > 0 &&
            parameters->tcp_numlayers == 1 && parameters->tcp_rates[0] == 0) {
        parameters->tcp_rates[0] = (float)(image->numcomps * image->comps[0].w *
                                   image->comps[0].h * image->comps[0].prec) /
                                   (float)(((uint32_t)parameters->max_cs_size) * 8 * image->comps[0].dx *
                                           image->comps[0].dy);
    }

	/* if no rate entered, lossless by default */
	if (parameters->tcp_numlayers == 0) {
		parameters->tcp_rates[0] = 0;
		parameters->tcp_numlayers = 1;
		parameters->cp_disto_alloc = 1;
	}

	/* see if max_codestream_size does limit input rate */
	double image_bytes = ((double) image->numcomps * image->comps[0].w
			* image->comps[0].h * image->comps[0].prec)
			/ (8 * image->comps[0].dx * image->comps[0].dy);
	if (parameters->max_cs_size == 0) {
		if (parameters->tcp_numlayers > 0
				&& parameters->tcp_rates[parameters->tcp_numlayers - 1] > 0) {
			parameters->max_cs_size = (uint64_t) floor(
					image_bytes
							/ parameters->tcp_rates[parameters->tcp_numlayers
									- 1]);
		}
	} else {
		bool cap = false;
		auto min_rate = image_bytes / (double) parameters->max_cs_size;
		for (i = 0; i < parameters->tcp_numlayers; i++) {
			if (parameters->tcp_rates[i] < min_rate) {
				parameters->tcp_rates[i] = min_rate;
				cap = true;
			}
		}
		if (cap) {
			GROK_WARN(
					"The desired maximum codestream size has limited\n"
							"at least one of the desired quality layers");
		}
	}

	/* Manage profiles and applications and set RSIZ */
	/* set cinema parameters if required */
	if (parameters->isHT){
		parameters->rsiz |= GRK_JPH_RSIZ_FLAG;
	}
	if (GRK_IS_CINEMA(parameters->rsiz)) {
		if ((parameters->rsiz == GRK_PROFILE_CINEMA_S2K)
				|| (parameters->rsiz == GRK_PROFILE_CINEMA_S4K)) {
			GROK_WARN(
					"JPEG 2000 Scalable Digital Cinema profiles not supported");
			parameters->rsiz = GRK_PROFILE_NONE;
		} else {
			if (j2k_is_cinema_compliant(image, parameters->rsiz)) {
				j2k_set_cinema_parameters(parameters, image);
			} else {
				parameters->rsiz = GRK_PROFILE_NONE;
			}
		}
	} else if (GRK_IS_STORAGE(parameters->rsiz)) {
		GROK_WARN(
				"JPEG 2000 Long Term Storage profile not supported");
		parameters->rsiz = GRK_PROFILE_NONE;
	} else if (GRK_IS_BROADCAST(parameters->rsiz)) {
		// Todo: sanity check on tiling (must be done for each frame)
		// Standard requires either single-tile or multi-tile. Multi-tile only permits
		// 1 or 4 tiles per frame, where multiple tiles have identical
		// sizes, and are configured in either 2x2 or 1x4 layout.

		//sanity check on reversible vs. irreversible
		uint16_t profile = parameters->rsiz & 0xFF00;
		if (profile == GRK_PROFILE_BC_MULTI_R) {
			if (parameters->irreversible) {
				GROK_WARN(
						"JPEG 2000 Broadcast profile; multi-tile reversible: forcing irreversible flag to false");
				parameters->irreversible = false;
			}
		} else {
			if (!parameters->irreversible) {
				GROK_WARN(
						"JPEG 2000 Broadcast profile: forcing irreversible flag to true");
				parameters->irreversible = true;
			}
		}
		// sanity check on levels
		auto level = parameters->rsiz & 0xF;
		if (level > maxMainLevel) {
			GROK_WARN(
					"JPEG 2000 Broadcast profile: invalid level %d", level);
			parameters->rsiz = GRK_PROFILE_NONE;
		}
	} else if (GRK_IS_IMF(parameters->rsiz)) {
		//sanity check on reversible vs. irreversible
		uint16_t profile = parameters->rsiz & 0xFF00;
		if ((profile == GRK_PROFILE_IMF_2K_R)
				|| (profile == GRK_PROFILE_IMF_4K_R)
				|| (profile == GRK_PROFILE_IMF_8K_R)) {
			if (parameters->irreversible) {
				GROK_WARN(
						"JPEG 2000 IMF profile; forcing irreversible flag to false");
				parameters->irreversible = false;
			}
		} else {
			if (!parameters->irreversible) {
				GROK_WARN(
						"JPEG 2000 IMF profile: forcing irreversible flag to true");
				parameters->irreversible = true;
			}
		}
		//sanity check on main and sub levels
		auto main_level = parameters->rsiz & 0xF;
		if (main_level > maxMainLevel) {
			GROK_WARN(
					"JPEG 2000 IMF profile: invalid main-level %d\n",
					main_level);
			parameters->rsiz = GRK_PROFILE_NONE;
		}
		auto sub_level = (parameters->rsiz >> 4) & 0xF;
		bool invalidSubLevel = sub_level > maxSubLevel;
		if (main_level > 3) {
			invalidSubLevel = invalidSubLevel || (sub_level > main_level - 2);
		} else {
			invalidSubLevel = invalidSubLevel || (sub_level > 1);
		}
		if (invalidSubLevel) {
			GROK_WARN(
					"JPEG 2000 IMF profile: invalid sub-level %d", sub_level);
			parameters->rsiz = GRK_PROFILE_NONE;
		}

	    j2k_set_imf_parameters(parameters, image);
        if (!j2k_is_imf_compliant(parameters, image)) {
            parameters->rsiz = GRK_PROFILE_NONE;
        }

	} else if (GRK_IS_PART2(parameters->rsiz)) {
		if (parameters->rsiz == ((GRK_PROFILE_PART2) | (GRK_EXTENSION_NONE))) {
			GROK_WARN(
					"JPEG 2000 Part-2 profile defined\n"
							"but no Part-2 extension enabled.\n"
							"Profile set to NONE.");
			parameters->rsiz = GRK_PROFILE_NONE;
		} else if (parameters->rsiz
				!= ((GRK_PROFILE_PART2) | (GRK_EXTENSION_MCT))) {
			GROK_WARN(
					"Unsupported Part-2 extension enabled\n"
							"Profile set to NONE.");
			parameters->rsiz = GRK_PROFILE_NONE;
		}
	}

	if (parameters->numpocs) {
		/* initialisation of POC */
		if (!j2k_check_poc_val(parameters->POC, parameters->numpocs,
				parameters->numresolution, image->numcomps,
				parameters->tcp_numlayers)) {
			GROK_ERROR( "Failed to initialize POC");
			return false;
		}
	}

	/*
	 copy user encoding parameters
	 */

	/* keep a link to cp so that we can destroy it later in j2k_destroy_compress */
	cp = &(p_j2k->m_cp);

	/* set default values for cp */
	cp->tw = 1;
	cp->th = 1;

	cp->m_coding_param.m_enc.m_max_comp_size = parameters->max_comp_size;
	cp->rsiz = parameters->rsiz;
	cp->m_coding_param.m_enc.m_disto_alloc = parameters->cp_disto_alloc & 1u;
	cp->m_coding_param.m_enc.m_fixed_quality = parameters->cp_fixed_quality
			& 1u;
	cp->m_coding_param.m_enc.rateControlAlgorithm =
			parameters->rateControlAlgorithm;

	/* tiles */
	cp->tdx = parameters->cp_tdx;
	cp->tdy = parameters->cp_tdy;

	/* tile offset */
	cp->tx0 = parameters->cp_tx0;
	cp->ty0 = parameters->cp_ty0;

	/* comment string */
	if (parameters->cp_num_comments) {
		for (size_t i = 0; i < parameters->cp_num_comments; ++i) {
			cp->comment_len[i] = parameters->cp_comment_len[i];
			if (!cp->comment_len[i]) {
				GROK_WARN( "Empty comment. Ignoring");
				continue;
			}
			if (cp->comment_len[i] > GRK_MAX_COMMENT_LENGTH) {
				GROK_WARN(
						"Comment length %s is greater than maximum comment length %d. Ignoring\n",
						cp->comment_len[i], GRK_MAX_COMMENT_LENGTH);
				continue;
			}
			cp->comment[i] = (char*) grk_buffer_new(cp->comment_len[i]);
			if (!cp->comment[i]) {
				GROK_ERROR(
						"Not enough memory to allocate copy of comment string");
				return false;
			}
			memcpy(cp->comment[i], parameters->cp_comment[i],
					cp->comment_len[i]);
			cp->isBinaryComment[i] = parameters->cp_is_binary_comment[i];
			cp->num_comments++;
		}
	} else {
		/* Create default comment for codestream */
		const char comment[] = "Created by Grok     version ";
		const size_t clen = strlen(comment);
		const char *version = grk_version();

		cp->comment[0] = (char*) grk_buffer_new(clen + strlen(version) + 1);
		if (!cp->comment[0]) {
			GROK_ERROR(
					"Not enough memory to allocate comment string");
			return false;
		}
		sprintf(cp->comment[0], "%s%s", comment, version);
		cp->comment_len[0] = (uint16_t) strlen(cp->comment[0]);
		cp->num_comments = 1;
		cp->isBinaryComment[0] = false;
	}

	/*
	 calculate other encoding parameters
	 */

	if (parameters->tile_size_on) {
		// avoid divide by zero
		if (cp->tdx == 0 || cp->tdy == 0) {
			return false;
		}
		cp->tw = ceildiv<uint32_t>((image->x1 - cp->tx0), cp->tdx);
		cp->th = ceildiv<uint32_t>((image->y1 - cp->ty0), cp->tdy);
	} else {
		cp->tdx = image->x1 - cp->tx0;
		cp->tdy = image->y1 - cp->ty0;
	}

	if (parameters->tp_on) {
		cp->m_coding_param.m_enc.m_tp_flag = (uint8_t) parameters->tp_flag;
		cp->m_coding_param.m_enc.m_tp_on = 1;
	}

    uint8_t numgbits = parameters->isHT ? 1 : 2;
	cp->tcps = new grk_tcp[cp->tw * cp->th];
	for (tileno = 0; tileno < cp->tw * cp->th; tileno++) {
		grk_tcp *tcp = cp->tcps + tileno;
		tcp->isHT = parameters->isHT;
		if (tcp->isHT) {
			tcp->qcd.generate(numgbits,
							parameters->numresolution-1,
							!parameters->irreversible,
							image->comps[0].prec,
							tcp->mct > 0,
							image->comps[0].sgnd);
		}
		tcp->numlayers = parameters->tcp_numlayers;

		for (j = 0; j < tcp->numlayers; j++) {
			if (GRK_IS_CINEMA(cp->rsiz)  || GRK_IS_IMF(cp->rsiz)) {
				if (cp->m_coding_param.m_enc.m_fixed_quality) {
					tcp->distoratio[j] = parameters->tcp_distoratio[j];
				}
				tcp->rates[j] = parameters->tcp_rates[j];
			} else {
				/* add fixed_quality */
				if (cp->m_coding_param.m_enc.m_fixed_quality) {
					tcp->distoratio[j] = parameters->tcp_distoratio[j];
				} else {
					tcp->rates[j] = parameters->tcp_rates[j];
				}
			}
		}

		tcp->csty = parameters->csty;
		tcp->prg = parameters->prog_order;
		tcp->mct = parameters->tcp_mct;

		numpocs_tile = 0;
		tcp->POC = 0;

		if (parameters->numpocs) {
			/* initialisation of POC */
			tcp->POC = 1;
			for (i = 0; i < parameters->numpocs; i++) {
				if (tileno + 1 == parameters->POC[i].tile) {
					 grk_poc  *tcp_poc = &tcp->pocs[numpocs_tile];

					tcp_poc->resno0 = parameters->POC[numpocs_tile].resno0;
					tcp_poc->compno0 = parameters->POC[numpocs_tile].compno0;
					tcp_poc->layno1 = parameters->POC[numpocs_tile].layno1;
					tcp_poc->resno1 = parameters->POC[numpocs_tile].resno1;
					tcp_poc->compno1 = parameters->POC[numpocs_tile].compno1;
					tcp_poc->prg1 = parameters->POC[numpocs_tile].prg1;
					tcp_poc->tile = parameters->POC[numpocs_tile].tile;

					numpocs_tile++;
				}
			}
			if (numpocs_tile == 0) {
				GROK_ERROR(
						"Problem with specified progression order changes");
				return false;
			}
			tcp->numpocs = numpocs_tile - 1;
		} else {
			tcp->numpocs = 0;
		}

		tcp->tccps = (grk_tccp*) grk_calloc(image->numcomps, sizeof(grk_tccp));
		if (!tcp->tccps) {
			GROK_ERROR(
					"Not enough memory to allocate tile component coding parameters");
			return false;
		}
		if (parameters->mct_data) {

			uint32_t lMctSize = image->numcomps * image->numcomps
					* (uint32_t) sizeof(float);
			float *lTmpBuf = (float*) grk_malloc(lMctSize);
			int32_t *l_dc_shift = (int32_t*) ((uint8_t*) parameters->mct_data
					+ lMctSize);

			if (!lTmpBuf) {
				GROK_ERROR(
						"Not enough memory to allocate temp buffer");
				return false;
			}

			tcp->mct = 2;
			tcp->m_mct_coding_matrix = (float*) grk_malloc(lMctSize);
			if (!tcp->m_mct_coding_matrix) {
				grok_free(lTmpBuf);
				lTmpBuf = nullptr;
				GROK_ERROR(
						"Not enough memory to allocate encoder MCT coding matrix ");
				return false;
			}
			memcpy(tcp->m_mct_coding_matrix, parameters->mct_data, lMctSize);
			memcpy(lTmpBuf, parameters->mct_data, lMctSize);

			tcp->m_mct_decoding_matrix = (float*) grk_malloc(lMctSize);
			if (!tcp->m_mct_decoding_matrix) {
				grok_free(lTmpBuf);
				lTmpBuf = nullptr;
				GROK_ERROR(
						"Not enough memory to allocate encoder MCT decoding matrix ");
				return false;
			}
			if (matrix_inversion_f(lTmpBuf, (tcp->m_mct_decoding_matrix),
					image->numcomps) == false) {
				grok_free(lTmpBuf);
				lTmpBuf = nullptr;
				GROK_ERROR(
						"Failed to inverse encoder MCT decoding matrix ");
				return false;
			}

			tcp->mct_norms = (double*) grk_malloc(
					image->numcomps * sizeof(double));
			if (!tcp->mct_norms) {
				grok_free(lTmpBuf);
				lTmpBuf = nullptr;
				GROK_ERROR(
						"Not enough memory to allocate encoder MCT norms ");
				return false;
			}
			mct::calculate_norms(tcp->mct_norms, image->numcomps,
					tcp->m_mct_decoding_matrix);
			grok_free(lTmpBuf);

			for (i = 0; i < image->numcomps; i++) {
				grk_tccp *tccp = &tcp->tccps[i];
				tccp->m_dc_level_shift = l_dc_shift[i];
			}

			if (j2k_setup_mct_encoding(tcp, image) == false) {
				/* free will be handled by j2k_destroy */
				GROK_ERROR(
						"Failed to setup j2k mct encoding");
				return false;
			}
		} else {
			if (tcp->mct == 1 && image->numcomps >= 3) { /* RGB->YCC MCT is enabled */
				if ((image->comps[0].dx != image->comps[1].dx)
						|| (image->comps[0].dx != image->comps[2].dx)
						|| (image->comps[0].dy != image->comps[1].dy)
						|| (image->comps[0].dy != image->comps[2].dy)) {
					GROK_WARN(
							"Cannot perform MCT on components with different sizes. Disabling MCT.");
					tcp->mct = 0;
				}
			}
			for (i = 0; i < image->numcomps; i++) {
				grk_tccp *tccp = tcp->tccps + i;
				 grk_image_comp  *l_comp = image->comps + i;
				if (!l_comp->sgnd) {
					tccp->m_dc_level_shift = 1 << (l_comp->prec - 1);
				}
			}
		}

		for (i = 0; i < image->numcomps; i++) {
			grk_tccp *tccp = &tcp->tccps[i];

			/* 0 => one precinct || 1 => custom precinct  */
			tccp->csty = parameters->csty & J2K_CP_CSTY_PRT;
			tccp->numresolutions = parameters->numresolution;
			tccp->cblkw = int_floorlog2(parameters->cblockw_init);
			tccp->cblkh = int_floorlog2(parameters->cblockh_init);
			tccp->cblk_sty = parameters->cblk_sty;
			tccp->qmfbid = parameters->irreversible ? 0 : 1;
			tccp->qntsty =
					parameters->irreversible ?
							J2K_CCP_QNTSTY_SEQNT : J2K_CCP_QNTSTY_NOQNT;
			tccp->numgbits = numgbits;

			if ((int32_t) i == parameters->roi_compno) {
				tccp->roishift = parameters->roi_shift;
			} else {
				tccp->roishift = 0;
			}
			if ((parameters->csty & J2K_CCP_CSTY_PRT) && parameters->res_spec) {
				uint32_t p = 0;
				int32_t it_res;
				assert(tccp->numresolutions > 0);
				for (it_res = (int32_t) tccp->numresolutions - 1; it_res >= 0;
						it_res--) {
					if (p < parameters->res_spec) {
						if (parameters->prcw_init[p] < 1) {
							tccp->prcw[it_res] = 1;
						} else {
							tccp->prcw[it_res] = uint_floorlog2(
									parameters->prcw_init[p]);
						}
						if (parameters->prch_init[p] < 1) {
							tccp->prch[it_res] = 1;
						} else {
							tccp->prch[it_res] = uint_floorlog2(
									parameters->prch_init[p]);
						}
					} else {
						uint32_t res_spec = parameters->res_spec;
						uint32_t size_prcw = 0;
						uint32_t size_prch = 0;
						size_prcw = parameters->prcw_init[res_spec - 1]
								>> (p - (res_spec - 1));
						size_prch = parameters->prch_init[res_spec - 1]
								>> (p - (res_spec - 1));
						if (size_prcw < 1) {
							tccp->prcw[it_res] = 1;
						} else {
							tccp->prcw[it_res] = uint_floorlog2(size_prcw);
						}
						if (size_prch < 1) {
							tccp->prch[it_res] = 1;
						} else {
							tccp->prch[it_res] = uint_floorlog2(size_prch);
						}
					}
					p++;
					/*printf("\nsize precinct for level %d : %d,%d\n", it_res,tccp->prcw[it_res], tccp->prch[it_res]); */
				} /*end for*/
			} else {
				for (j = 0; j < tccp->numresolutions; j++) {
					tccp->prcw[j] = 15;
					tccp->prch[j] = 15;
				}
			}
			if (tcp->isHT)
				tcp->qcd.pull(tccp->stepsizes, !parameters->irreversible);
			else
				Quantizer::calc_explicit_stepsizes(tccp, image->comps[i].prec);
		}
	}
	if (parameters->mct_data) {
		grok_free(parameters->mct_data);
		parameters->mct_data = nullptr;
	}
	return true;
}


bool j2k_encode(grk_j2k *p_j2k, grk_plugin_tile *tile, BufferedStream *p_stream) {
	uint16_t i;
	uint32_t j;
	bool transfer_image_to_tile = false;

	assert(p_j2k != nullptr);
	assert(p_stream != nullptr);

	auto p_tcd = p_j2k->m_tileProcessor;
	p_tcd->current_plugin_tile = tile;

	uint32_t nb_tiles = (uint32_t)p_j2k->m_cp.th * p_j2k->m_cp.tw;
	if (nb_tiles > max_num_tiles){
		GROK_ERROR("Number of tiles %d is greater the %d max tiles "
				"allowed by the standard.", nb_tiles,max_num_tiles );
		return false;
	}
	if (nb_tiles == 1) {
		transfer_image_to_tile = true;
#ifdef __SSE__
		for (j = 0; j < p_j2k->m_tileProcessor->image->numcomps; ++j) {
			 grk_image_comp  *img_comp = p_tcd->image->comps + j;
			 /* tile data shall be aligned on 16 bytes */
			if (((size_t) img_comp->data & 0xFU) != 0U)
				transfer_image_to_tile = false;
		}
#endif
	}
	for (i = 0; i < nb_tiles; ++i) {
		if (!j2k_pre_write_tile(p_j2k, i)) {
			return false;
		}

		/* if we only have one tile, then simply set tile component data equal to
		 * image component data. Otherwise, allocate tile data and copy */
		for (j = 0; j < p_j2k->m_tileProcessor->image->numcomps; ++j) {
			auto tilec = p_tcd->tile->comps + j;
			if (transfer_image_to_tile) {
				tilec->buf->data = (p_tcd->image->comps + j)->data;
				tilec->buf->owns_data = false;
			} else {
				if (!tilec->buf->alloc_component_data_encode()) {
					GROK_ERROR(
							"Error allocating tile component data.");
					return false;
				}
			}
		}
		if (!transfer_image_to_tile)
			p_j2k->m_tileProcessor->copy_image_to_tile();
		if (!j2k_post_write_tile(p_j2k, p_stream))
			return false;
	}
	return true;
}

bool j2k_end_compress(grk_j2k *p_j2k, BufferedStream *p_stream) {
	/* customization of the encoding */
	if (!j2k_setup_end_compress(p_j2k)) {
		return false;
	}

	if (!j2k_exec(p_j2k, p_j2k->m_procedure_list, p_stream)) {
		return false;
	}
	return true;
}

bool j2k_start_compress(grk_j2k *p_j2k, BufferedStream *p_stream,
		grk_image *p_image) {

	assert(p_j2k != nullptr);
	assert(p_stream != nullptr);

	p_j2k->m_private_image = grk_image_create0();
	if (!p_j2k->m_private_image) {
		GROK_ERROR( "Failed to allocate image header.");
		return false;
	}
	grk_copy_image_header(p_image, p_j2k->m_private_image);

	/* TODO_MSD: Find a better way */
	if (p_image->comps) {
		uint32_t it_comp;
		for (it_comp = 0; it_comp < p_image->numcomps; it_comp++) {
			if (p_image->comps[it_comp].data) {
				p_j2k->m_private_image->comps[it_comp].data =
						p_image->comps[it_comp].data;
				p_image->comps[it_comp].data = nullptr;

			}
		}
	}

	/* customization of the validation */
	if (!j2k_setup_encoding_validation(p_j2k)) {
		return false;
	}

	/* validation of the parameters codec */
	if (!j2k_exec(p_j2k, p_j2k->m_validation_list, p_stream)) {
		return false;
	}

	/* customization of the encoding */
	if (!j2k_setup_header_writing(p_j2k)) {
		return false;
	}

	/* write header */
	if (!j2k_exec(p_j2k, p_j2k->m_procedure_list, p_stream)) {
		return false;
	}

	return true;
}

static bool j2k_pre_write_tile(grk_j2k *p_j2k, uint16_t tile_index) {
	if (tile_index != p_j2k->m_current_tile_number) {
		GROK_ERROR( "The given tile index does not match.");
		return false;
	}
	//event_msg( EVT_INFO, "tile number %d / %d\n", p_j2k->m_current_tile_number + 1, p_j2k->m_cp.tw * p_j2k->m_cp.th);
	p_j2k->m_specific_param.m_encoder.m_current_tile_part_number = 0;
	p_j2k->m_tileProcessor->cur_totnum_tp =
			p_j2k->m_cp.tcps[tile_index].m_nb_tile_parts;
	p_j2k->m_specific_param.m_encoder.m_current_poc_tile_part_number = 0;

	/* initialisation before tile encoding  */
	if (!p_j2k->m_tileProcessor->init_encode_tile(p_j2k->m_current_tile_number)) {
		return false;
	}

	return true;
}

static bool j2k_post_write_tile(grk_j2k *p_j2k, BufferedStream *p_stream) {
	auto cp = &(p_j2k->m_cp);
	auto image = p_j2k->m_private_image;
	auto img_comp = image->comps;
	uint64_t tile_size = 0;

	for (uint32_t i = 0; i < image->numcomps; ++i) {
		tile_size += (uint64_t) ceildiv<uint32_t>(cp->tdx, img_comp->dx)
				* ceildiv<uint32_t>(cp->tdy, img_comp->dy)
				* img_comp->prec;
		++img_comp;
	}

	tile_size = (uint64_t) ((double) (tile_size) * 0.1625); /* 1.3/8 = 0.1625 */
	tile_size += j2k_get_specific_header_sizes(p_j2k);

	// ToDo: use better estimate of signaling overhead for packets,
	// to avoid hard-coding this lower bound on tile buffer size

	// allocate at least 256 bytes per component
	if (tile_size < 256 * image->numcomps)
		tile_size = 256 * image->numcomps;

	uint64_t available_data = tile_size;
	uint64_t nb_bytes_written=0;
	if (!j2k_write_first_tile_part(p_j2k, &nb_bytes_written, available_data,
			p_stream)) {
		return false;
	}
	available_data -= nb_bytes_written;
	nb_bytes_written = 0;
	if (!j2k_write_all_tile_parts(p_j2k, &nb_bytes_written, available_data,
			p_stream)) {
		return false;
	}
	++p_j2k->m_current_tile_number;
	return true;
}

static bool j2k_setup_end_compress(grk_j2k *p_j2k) {
	assert(p_j2k != nullptr);

	p_j2k->m_procedure_list->push_back((j2k_procedure) j2k_write_eoc);
	if (GRK_IS_CINEMA(p_j2k->m_cp.rsiz) || GRK_IS_IMF(p_j2k->m_cp.rsiz))
		p_j2k->m_procedure_list->push_back((j2k_procedure) j2k_write_updated_tlm);
	p_j2k->m_procedure_list->push_back((j2k_procedure) j2k_write_epc);
	p_j2k->m_procedure_list->push_back((j2k_procedure) j2k_end_encoding);
	//custom procedures here

	return true;
}


static bool j2k_mct_validation(grk_j2k *p_j2k, BufferedStream *p_stream) {
	(void) p_stream;

	bool l_is_valid = true;
	uint32_t i, j;

	assert(p_j2k != nullptr);
	assert(p_stream != nullptr);


	if ((p_j2k->m_cp.rsiz & 0x8200) == 0x8200) {
		uint32_t l_nb_tiles = p_j2k->m_cp.th * p_j2k->m_cp.tw;
		grk_tcp *l_tcp = p_j2k->m_cp.tcps;

		for (i = 0; i < l_nb_tiles; ++i) {
			if (l_tcp->mct == 2) {
				grk_tccp *l_tccp = l_tcp->tccps;
				l_is_valid &= (l_tcp->m_mct_coding_matrix != nullptr);

				for (j = 0; j < p_j2k->m_private_image->numcomps; ++j) {
					l_is_valid &= !(l_tccp->qmfbid & 1);
					++l_tccp;
				}
			}
			++l_tcp;
		}
	}

	return l_is_valid;
}

static bool j2k_encoding_validation(grk_j2k *p_j2k, BufferedStream *p_stream) {
	(void) p_stream;
	bool l_is_valid = true;

	assert(p_j2k != nullptr);
	assert(p_stream != nullptr);

	/* STATE checking */
	/* make sure the state is at 0 */
	l_is_valid &= (p_j2k->m_specific_param.m_decoder.m_state
			== J2K_DEC_STATE_NONE);

	/* POINTER validation */
	/* make sure a p_j2k codec is present */
	l_is_valid &= (p_j2k->m_procedure_list != nullptr);
	/* make sure a validation list is present */
	l_is_valid &= (p_j2k->m_validation_list != nullptr);

	/* ISO 15444-1:2004 states between 1 & 33 (decomposition levels between 0 -> 32) */
	if ((p_j2k->m_cp.tcps->tccps->numresolutions == 0)
			|| (p_j2k->m_cp.tcps->tccps->numresolutions > GRK_J2K_MAXRLVLS)) {
		GROK_ERROR(
				"Invalid number of resolutions : %d not in range [1,%d]\n",
				p_j2k->m_cp.tcps->tccps->numresolutions, GRK_J2K_MAXRLVLS);
		return false;
	}

	if (p_j2k->m_cp.tdx == 0) {
		GROK_ERROR(
				"Tile x dimension must be greater than zero ");
		return false;
	}

	if (p_j2k->m_cp.tdy == 0) {
		GROK_ERROR(
				"Tile y dimension must be greater than zero ");
		return false;
	}

	/* PARAMETER VALIDATION */
	return l_is_valid;
}


static bool j2k_setup_encoding_validation(grk_j2k *p_j2k) {
	assert(p_j2k != nullptr);

	p_j2k->m_validation_list->push_back((j2k_procedure) j2k_encoding_validation);
	//custom validation here
	p_j2k->m_validation_list->push_back((j2k_procedure) j2k_mct_validation);

	return true;
}

static bool j2k_setup_header_writing(grk_j2k *p_j2k) {
	assert(p_j2k != nullptr);

	p_j2k->m_procedure_list->push_back((j2k_procedure) j2k_init_info);
	p_j2k->m_procedure_list->push_back((j2k_procedure) j2k_write_soc);
	p_j2k->m_procedure_list->push_back((j2k_procedure) j2k_write_siz);
	if (p_j2k->m_cp.tcps[0].isHT)
	  p_j2k->m_procedure_list->push_back((j2k_procedure) j2k_write_cap);
	p_j2k->m_procedure_list->push_back((j2k_procedure) j2k_write_cod);
	p_j2k->m_procedure_list->push_back((j2k_procedure) j2k_write_qcd);
	p_j2k->m_procedure_list->push_back((j2k_procedure) j2k_write_all_coc);
	p_j2k->m_procedure_list->push_back((j2k_procedure) j2k_write_all_qcc);


	if (GRK_IS_CINEMA(p_j2k->m_cp.rsiz)  || GRK_IS_IMF(p_j2k->m_cp.rsiz)) {
		p_j2k->m_procedure_list->push_back((j2k_procedure) j2k_write_tlm);
		if (p_j2k->m_cp.rsiz == GRK_PROFILE_CINEMA_4K) {
			p_j2k->m_procedure_list->push_back((j2k_procedure) j2k_write_poc);
		}
	}
	p_j2k->m_procedure_list->push_back((j2k_procedure) j2k_write_regions);
	p_j2k->m_procedure_list->push_back((j2k_procedure) j2k_write_com);
	//begin custom procedures

    if ((p_j2k->m_cp.rsiz & (GRK_PROFILE_PART2 | GRK_EXTENSION_MCT)) ==
            (GRK_PROFILE_PART2 | GRK_EXTENSION_MCT)) {
		p_j2k->m_procedure_list->push_back((j2k_procedure) j2k_write_mct_data_group);
	}
	//end custom procedures
	if (p_j2k->cstr_index) {
		p_j2k->m_procedure_list->push_back((j2k_procedure) j2k_get_end_header);
	}
	p_j2k->m_procedure_list->push_back((j2k_procedure) j2k_create_tcd);
	p_j2k->m_procedure_list->push_back((j2k_procedure) j2k_update_rates);

	return true;
}

static bool j2k_write_first_tile_part(grk_j2k *p_j2k, uint64_t *p_data_written,
		uint64_t total_data_size, BufferedStream *p_stream) {
	uint64_t l_nb_bytes_written = 0;
	uint64_t l_current_nb_bytes_written;
	auto l_tcd = p_j2k->m_tileProcessor;
	auto l_cp = &(p_j2k->m_cp);

	l_tcd->cur_pino = 0;

	/*Get number of tile parts*/
	p_j2k->m_specific_param.m_encoder.m_current_poc_tile_part_number = 0;

	/* INDEX >> */
	/* << INDEX */

	l_current_nb_bytes_written = 0;
	uint64_t psot_location = 0;
	if (!j2k_write_sot(p_j2k, p_stream, &psot_location,
			&l_current_nb_bytes_written)) {
		return false;
	}
	l_nb_bytes_written += l_current_nb_bytes_written;
	total_data_size -= l_current_nb_bytes_written;

	if (!GRK_IS_CINEMA(l_cp->rsiz)) {
		if (l_cp->tcps[p_j2k->m_current_tile_number].numpocs) {
			l_current_nb_bytes_written = 0;
			if (!j2k_write_poc_in_memory(p_j2k, p_stream,
					&l_current_nb_bytes_written))
				return false;
			l_nb_bytes_written += l_current_nb_bytes_written;
			total_data_size -= l_current_nb_bytes_written;
		}
	}

	l_current_nb_bytes_written = 0;
	if (!j2k_write_sod(p_j2k, l_tcd, &l_current_nb_bytes_written,
			total_data_size, p_stream)) {
		return false;
	}
	l_nb_bytes_written += l_current_nb_bytes_written;
	total_data_size -= l_current_nb_bytes_written;
	*p_data_written = l_nb_bytes_written;

	/* Writing Psot in SOT marker */
	/* PSOT */
	auto currentLocation = p_stream->tell();
	p_stream->seek(psot_location);
	if (!p_stream->write_int((uint32_t) l_nb_bytes_written)) {
		return false;
	}
	p_stream->seek(currentLocation);
	if (GRK_IS_CINEMA(l_cp->rsiz)  || GRK_IS_IMF(l_cp->rsiz) ) {
		j2k_update_tlm(p_j2k, (uint32_t) l_nb_bytes_written);
	}
	return true;
}

static bool j2k_write_all_tile_parts(grk_j2k *p_j2k, uint64_t *p_data_written,
		uint64_t total_data_size, BufferedStream *p_stream) {
	uint8_t tilepartno = 0;
	uint64_t l_nb_bytes_written = 0;
	uint64_t l_current_nb_bytes_written;
	uint32_t l_part_tile_size;
	uint32_t tot_num_tp;
	uint32_t pino;

	auto l_tcd = p_j2k->m_tileProcessor;
	auto l_cp = &(p_j2k->m_cp);
	auto l_tcp = l_cp->tcps + p_j2k->m_current_tile_number;

	/*Get number of tile parts*/
	tot_num_tp = j2k_get_num_tp(l_cp, 0, p_j2k->m_current_tile_number);
  if (tot_num_tp > 255) {
    GROK_ERROR(
        "Tile %d contains more than 255 tile parts, which is not permitted by the JPEG 2000 standard.\n",
        p_j2k->m_current_tile_number);
    return false;
  }

	/* start writing remaining tile parts */
	++p_j2k->m_specific_param.m_encoder.m_current_tile_part_number;
	for (tilepartno = 1; tilepartno < tot_num_tp; ++tilepartno) {
		p_j2k->m_specific_param.m_encoder.m_current_poc_tile_part_number =
				tilepartno;
		l_current_nb_bytes_written = 0;
		l_part_tile_size = 0;

		uint64_t psot_location = 0;
		if (!j2k_write_sot(p_j2k, p_stream, &psot_location,
				&l_current_nb_bytes_written)) {
			return false;
		}
		l_nb_bytes_written += l_current_nb_bytes_written;
		total_data_size -= l_current_nb_bytes_written;
		l_part_tile_size += (uint32_t) l_current_nb_bytes_written;

		l_current_nb_bytes_written = 0;
		if (!j2k_write_sod(p_j2k, l_tcd, &l_current_nb_bytes_written,
				total_data_size, p_stream)) {
			return false;
		}
		l_nb_bytes_written += l_current_nb_bytes_written;
		total_data_size -= l_current_nb_bytes_written;
		l_part_tile_size += (uint32_t) l_current_nb_bytes_written;

		/* Writing Psot in SOT marker */
		/* PSOT */
		auto currentLocation = p_stream->tell();
		p_stream->seek(psot_location);
		if (!p_stream->write_int(l_part_tile_size)) {
			return false;
		}
		p_stream->seek(currentLocation);
		if (GRK_IS_CINEMA(l_cp->rsiz)  || GRK_IS_IMF(l_cp->rsiz)) {
			j2k_update_tlm(p_j2k, l_part_tile_size);
		}

		++p_j2k->m_specific_param.m_encoder.m_current_tile_part_number;
	}

	for (pino = 1; pino <= l_tcp->numpocs; ++pino) {
		l_tcd->cur_pino = pino;

		/*Get number of tile parts*/
		tot_num_tp = j2k_get_num_tp(l_cp, pino, p_j2k->m_current_tile_number);
	  if (tot_num_tp > 255) {
	    GROK_ERROR(
	        "Tile %d contains more than 255 tile parts, which is not permitted by the JPEG 2000 standard.\n",
	        p_j2k->m_current_tile_number);
	    return false;
	  }

		for (tilepartno = 0; tilepartno < tot_num_tp; ++tilepartno) {
			p_j2k->m_specific_param.m_encoder.m_current_poc_tile_part_number =
					tilepartno;
			l_current_nb_bytes_written = 0;
			l_part_tile_size = 0;
			uint64_t psot_location = 0;
			if (!j2k_write_sot(p_j2k, p_stream, &psot_location,
					&l_current_nb_bytes_written)) {
				return false;
			}

			l_nb_bytes_written += l_current_nb_bytes_written;
			total_data_size -= l_current_nb_bytes_written;
			l_part_tile_size += (uint32_t) l_current_nb_bytes_written;

			l_current_nb_bytes_written = 0;
			if (!j2k_write_sod(p_j2k, l_tcd, &l_current_nb_bytes_written,
					total_data_size, p_stream)) {
				return false;
			}

			l_nb_bytes_written += l_current_nb_bytes_written;
			total_data_size -= l_current_nb_bytes_written;
			l_part_tile_size += (uint32_t) l_current_nb_bytes_written;

			/* Writing Psot in SOT marker */
			/* PSOT */
			auto currentLocation = p_stream->tell();
			p_stream->seek(psot_location);
			if (!p_stream->write_int(l_part_tile_size)) {
				return false;
			}
			p_stream->seek(currentLocation);

			if (GRK_IS_CINEMA(l_cp->rsiz) || GRK_IS_IMF(l_cp->rsiz)) {
				j2k_update_tlm(p_j2k, l_part_tile_size);
			}
			++p_j2k->m_specific_param.m_encoder.m_current_tile_part_number;
		}
	}
	*p_data_written = l_nb_bytes_written;
	return true;
}

static bool j2k_write_updated_tlm(grk_j2k *p_j2k, BufferedStream *p_stream) {
	uint32_t l_tlm_size;
	int64_t l_tlm_position, l_current_position;

	assert(p_j2k != nullptr);
	assert(p_stream != nullptr);

	l_tlm_size = 5 * p_j2k->m_specific_param.m_encoder.m_total_tile_parts;
	l_tlm_position = 6 + p_j2k->m_specific_param.m_encoder.m_tlm_start;
	l_current_position = p_stream->tell();

	if (!p_stream->seek(l_tlm_position)) {
		return false;
	}
	if (p_stream->write_bytes(
			p_j2k->m_specific_param.m_encoder.m_tlm_sot_offsets_buffer,
			l_tlm_size) != l_tlm_size) {
		return false;
	}
	if (!p_stream->seek(l_current_position)) {
		return false;
	}
	return true;
}


bool j2k_setup_mct_encoding(grk_tcp *p_tcp, grk_image *p_image) {
	uint32_t i;
	uint32_t l_indix = 1;
	grk_mct_data *l_mct_deco_data = nullptr, *l_mct_offset_data = nullptr;
	grk_simple_mcc_decorrelation_data *l_mcc_data;
	uint32_t l_mct_size, l_nb_elem;
	float *l_data, *l_current_data;
	grk_tccp *l_tccp;

	assert(p_tcp != nullptr);

	if (p_tcp->mct != 2) {
		return true;
	}

	if (p_tcp->m_mct_decoding_matrix) {
		if (p_tcp->m_nb_mct_records == p_tcp->m_nb_max_mct_records) {
			grk_mct_data *new_mct_records;
			p_tcp->m_nb_max_mct_records += default_number_mct_records;

			new_mct_records = (grk_mct_data*) grk_realloc(p_tcp->m_mct_records,
					p_tcp->m_nb_max_mct_records * sizeof(grk_mct_data));
			if (!new_mct_records) {
				grok_free(p_tcp->m_mct_records);
				p_tcp->m_mct_records = nullptr;
				p_tcp->m_nb_max_mct_records = 0;
				p_tcp->m_nb_mct_records = 0;
				/* GROK_ERROR( "Not enough memory to setup mct encoding"); */
				return false;
			}
			p_tcp->m_mct_records = new_mct_records;
			l_mct_deco_data = p_tcp->m_mct_records + p_tcp->m_nb_mct_records;

			memset(l_mct_deco_data, 0,
					(p_tcp->m_nb_max_mct_records - p_tcp->m_nb_mct_records)
							* sizeof(grk_mct_data));
		}
		l_mct_deco_data = p_tcp->m_mct_records + p_tcp->m_nb_mct_records;

		if (l_mct_deco_data->m_data) {
			grok_free(l_mct_deco_data->m_data);
			l_mct_deco_data->m_data = nullptr;
		}

		l_mct_deco_data->m_index = l_indix++;
		l_mct_deco_data->m_array_type = MCT_TYPE_DECORRELATION;
		l_mct_deco_data->m_element_type = MCT_TYPE_FLOAT;
		l_nb_elem = p_image->numcomps * p_image->numcomps;
		l_mct_size = l_nb_elem
				* MCT_ELEMENT_SIZE[l_mct_deco_data->m_element_type];
		l_mct_deco_data->m_data = (uint8_t*) grk_malloc(l_mct_size);

		if (!l_mct_deco_data->m_data) {
			return false;
		}

		j2k_mct_write_functions_from_float[l_mct_deco_data->m_element_type](
				p_tcp->m_mct_decoding_matrix, l_mct_deco_data->m_data,
				l_nb_elem);

		l_mct_deco_data->m_data_size = l_mct_size;
		++p_tcp->m_nb_mct_records;
	}

	if (p_tcp->m_nb_mct_records == p_tcp->m_nb_max_mct_records) {
		grk_mct_data *new_mct_records;
		p_tcp->m_nb_max_mct_records += default_number_mct_records;
		new_mct_records = (grk_mct_data*) grk_realloc(p_tcp->m_mct_records,
				p_tcp->m_nb_max_mct_records * sizeof(grk_mct_data));
		if (!new_mct_records) {
			grok_free(p_tcp->m_mct_records);
			p_tcp->m_mct_records = nullptr;
			p_tcp->m_nb_max_mct_records = 0;
			p_tcp->m_nb_mct_records = 0;
			/* GROK_ERROR( "Not enough memory to setup mct encoding"); */
			return false;
		}
		p_tcp->m_mct_records = new_mct_records;
		l_mct_offset_data = p_tcp->m_mct_records + p_tcp->m_nb_mct_records;

		memset(l_mct_offset_data, 0,
				(p_tcp->m_nb_max_mct_records - p_tcp->m_nb_mct_records)
						* sizeof(grk_mct_data));

		if (l_mct_deco_data) {
			l_mct_deco_data = l_mct_offset_data - 1;
		}
	}

	l_mct_offset_data = p_tcp->m_mct_records + p_tcp->m_nb_mct_records;

	if (l_mct_offset_data->m_data) {
		grok_free(l_mct_offset_data->m_data);
		l_mct_offset_data->m_data = nullptr;
	}

	l_mct_offset_data->m_index = l_indix++;
	l_mct_offset_data->m_array_type = MCT_TYPE_OFFSET;
	l_mct_offset_data->m_element_type = MCT_TYPE_FLOAT;
	l_nb_elem = p_image->numcomps;
	l_mct_size = l_nb_elem
			* MCT_ELEMENT_SIZE[l_mct_offset_data->m_element_type];
	l_mct_offset_data->m_data = (uint8_t*) grk_malloc(l_mct_size);

	if (!l_mct_offset_data->m_data) {
		return false;
	}

	l_data = (float*) grk_malloc(l_nb_elem * sizeof(float));
	if (!l_data) {
		grok_free(l_mct_offset_data->m_data);
		l_mct_offset_data->m_data = nullptr;
		return false;
	}

	l_tccp = p_tcp->tccps;
	l_current_data = l_data;

	for (i = 0; i < l_nb_elem; ++i) {
		*(l_current_data++) = (float) (l_tccp->m_dc_level_shift);
		++l_tccp;
	}

	j2k_mct_write_functions_from_float[l_mct_offset_data->m_element_type](
			l_data, l_mct_offset_data->m_data, l_nb_elem);

	grok_free(l_data);

	l_mct_offset_data->m_data_size = l_mct_size;

	++p_tcp->m_nb_mct_records;

	if (p_tcp->m_nb_mcc_records == p_tcp->m_nb_max_mcc_records) {
		grk_simple_mcc_decorrelation_data *new_mcc_records;
		p_tcp->m_nb_max_mcc_records += default_number_mct_records;
		new_mcc_records = (grk_simple_mcc_decorrelation_data*) grk_realloc(
				p_tcp->m_mcc_records,
				p_tcp->m_nb_max_mcc_records
						* sizeof(grk_simple_mcc_decorrelation_data));
		if (!new_mcc_records) {
			grok_free(p_tcp->m_mcc_records);
			p_tcp->m_mcc_records = nullptr;
			p_tcp->m_nb_max_mcc_records = 0;
			p_tcp->m_nb_mcc_records = 0;
			/* GROK_ERROR( "Not enough memory to setup mct encoding"); */
			return false;
		}
		p_tcp->m_mcc_records = new_mcc_records;
		l_mcc_data = p_tcp->m_mcc_records + p_tcp->m_nb_mcc_records;
		memset(l_mcc_data, 0,
				(p_tcp->m_nb_max_mcc_records - p_tcp->m_nb_mcc_records)
						* sizeof(grk_simple_mcc_decorrelation_data));

	}

	l_mcc_data = p_tcp->m_mcc_records + p_tcp->m_nb_mcc_records;
	l_mcc_data->m_decorrelation_array = l_mct_deco_data;
	l_mcc_data->m_is_irreversible = 1;
	l_mcc_data->m_nb_comps = p_image->numcomps;
	l_mcc_data->m_index = l_indix++;
	l_mcc_data->m_offset_array = l_mct_offset_data;
	++p_tcp->m_nb_mcc_records;

	return true;
}


static bool j2k_end_encoding(grk_j2k *p_j2k, BufferedStream *p_stream) {
	(void) p_stream;
	assert(p_j2k != nullptr);
	assert(p_stream != nullptr);

	delete p_j2k->m_tileProcessor;
	p_j2k->m_tileProcessor = nullptr;

	if (p_j2k->m_specific_param.m_encoder.m_tlm_sot_offsets_buffer) {
		grok_free(p_j2k->m_specific_param.m_encoder.m_tlm_sot_offsets_buffer);
		p_j2k->m_specific_param.m_encoder.m_tlm_sot_offsets_buffer = nullptr;
		p_j2k->m_specific_param.m_encoder.m_tlm_sot_offsets_current = 0;
	}
	return true;
}

static bool j2k_init_info(grk_j2k *p_j2k, BufferedStream *p_stream) {
	(void) p_stream;
	assert(p_j2k != nullptr);

	assert(p_stream != nullptr);
	return j2k_calculate_tp(&p_j2k->m_cp,
			&p_j2k->m_specific_param.m_encoder.m_total_tile_parts,
			p_j2k->m_private_image);
}

/**
 * Creates a tile-coder decoder.
 *
 * @param       p_stream                the stream to write data to.
 * @param       p_j2k                   J2K codec.

 */
static bool j2k_create_tcd(grk_j2k *p_j2k, BufferedStream *p_stream) {
	(void) p_stream;
	assert(p_j2k != nullptr);

	p_j2k->m_tileProcessor = new TileProcessor(false);
	if (!p_j2k->m_tileProcessor->init( p_j2k->m_private_image, &p_j2k->m_cp)) {
		delete p_j2k->m_tileProcessor;
		p_j2k->m_tileProcessor = nullptr;
		return false;
	}

	return true;
}

bool j2k_write_tile(grk_j2k *p_j2k, uint16_t tile_index, uint8_t *p_data,
		uint64_t data_size, BufferedStream *p_stream) {
	if (!j2k_pre_write_tile(p_j2k, tile_index)) {
		GROK_ERROR(
				"Error while j2k_pre_write_tile with tile index = %d\n",
				tile_index);
		return false;
	} else {
		uint32_t j;
		/* Allocate data */
		for (j = 0; j < p_j2k->m_tileProcessor->image->numcomps; ++j) {
			TileComponent *l_tilec = p_j2k->m_tileProcessor->tile->comps + j;

			if (!l_tilec->buf->alloc_component_data_encode()) {
				GROK_ERROR(
						"Error allocating tile component data.");
				return false;
			}
		}

		/* now copy data into the tile component */
		if (!p_j2k->m_tileProcessor->copy_image_data_to_tile(p_data, data_size)) {
			GROK_ERROR(
					"Size mismatch between tile data and sent data.");
			return false;
		}
		if (!j2k_post_write_tile(p_j2k, p_stream)) {
			GROK_ERROR(
					"Error while j2k_post_write_tile with tile index = %d\n",
					tile_index);
			return false;
		}
	}

	return true;
}

static float j2k_get_tp_stride(grk_tcp *p_tcp) {
	return (float) ((p_tcp->m_nb_tile_parts - 1) * 14);
}

static float j2k_get_default_stride(grk_tcp *p_tcp) {
	(void) p_tcp;
	return 0;
}


static bool j2k_update_rates(grk_j2k *p_j2k, BufferedStream *p_stream) {
	uint32_t i, j, k;
	double *l_rates = 0;
	uint32_t l_bits_empty, l_size_pixel;
	uint32_t l_last_res;
	float (*l_tp_stride_func)(grk_tcp*) = nullptr;

	assert(p_j2k != nullptr);

	assert(p_stream != nullptr);

	auto l_cp = &(p_j2k->m_cp);
	auto l_image = p_j2k->m_private_image;
	auto l_tcp = l_cp->tcps;

	auto width = l_image->x1 - l_image->x0;
	auto height = l_image->y1 - l_image->y0;
	if (width <= 0 || height <= 0)
		return false;

	l_bits_empty = 8 * l_image->comps->dx * l_image->comps->dy;
	l_size_pixel = l_image->numcomps * l_image->comps->prec;
	auto header_size = (double) p_stream->tell();

	if (l_cp->m_coding_param.m_enc.m_tp_on) {
		l_tp_stride_func = j2k_get_tp_stride;
	} else {
		l_tp_stride_func = j2k_get_default_stride;
	}

	for (i = 0; i < l_cp->th; ++i) {
		for (j = 0; j < l_cp->tw; ++j) {
			double l_offset = (double) (*l_tp_stride_func)(l_tcp)
					/ l_tcp->numlayers;

			/* 4 borders of the tile rescale on the image if necessary */
			uint32_t l_x0 = std::max<uint32_t>((l_cp->tx0 + j * l_cp->tdx),
					l_image->x0);
			uint32_t l_y0 = std::max<uint32_t>((l_cp->ty0 + i * l_cp->tdy),
					l_image->y0);
			uint32_t l_x1 = std::min<uint32_t>((l_cp->tx0 + (j + 1) * l_cp->tdx),
					l_image->x1);
			uint32_t l_y1 = std::min<uint32_t>((l_cp->ty0 + (i + 1) * l_cp->tdy),
					l_image->y1);
			uint64_t numTilePixels = (uint64_t) (l_x1 - l_x0) * (l_y1 - l_y0);

			l_rates = l_tcp->rates;
			for (k = 0; k < l_tcp->numlayers; ++k) {
				if (*l_rates > 0.0f) {
					*l_rates = ((((double)l_size_pixel * (double)numTilePixels))
							/ ((double)*l_rates * (double)l_bits_empty)) - l_offset;
				}
				++l_rates;
			}
			++l_tcp;
		}
	}
	l_tcp = l_cp->tcps;

	for (i = 0; i < l_cp->th; ++i) {
		for (j = 0; j < l_cp->tw; ++j) {
			l_rates = l_tcp->rates;
			/* 4 borders of the tile rescale on the image if necessary */
			uint32_t l_x0 = std::max<uint32_t>((l_cp->tx0 + j * l_cp->tdx),
					l_image->x0);
			uint32_t l_y0 = std::max<uint32_t>((l_cp->ty0 + i * l_cp->tdy),
					l_image->y0);
			uint32_t l_x1 = std::min<uint32_t>((l_cp->tx0 + (j + 1) * l_cp->tdx),
					l_image->x1);
			uint32_t l_y1 = std::min<uint32_t>((l_cp->ty0 + (i + 1) * l_cp->tdy),
					l_image->y1);
			uint64_t numTilePixels = (uint64_t) (l_x1 - l_x0) * (l_y1 - l_y0);
			double sot_adjust = ((double)numTilePixels * (double)header_size)
					/ ((double) width * height);

			if (*l_rates > 0.0) {
				*l_rates -= sot_adjust;

				if (*l_rates < 30.0f) {
					*l_rates = 30.0f;
				}
			}
			++l_rates;
			l_last_res = l_tcp->numlayers - 1;

			for (k = 1; k < l_last_res; ++k) {

				if (*l_rates > 0.0) {
					*l_rates -= sot_adjust;

					if (*l_rates < *(l_rates - 1) + 10.0) {
						*l_rates = (*(l_rates - 1)) + 20.0;
					}
				}
				++l_rates;
			}

			if (*l_rates > 0.0) {
				*l_rates -= (sot_adjust + 2.0);
				if (*l_rates < *(l_rates - 1) + 10.0) {
					*l_rates = (*(l_rates - 1)) + 20.0;
				}
			}
			++l_tcp;
		}
	}

	if (GRK_IS_CINEMA(l_cp->rsiz) || GRK_IS_IMF(l_cp->rsiz)) {
		p_j2k->m_specific_param.m_encoder.m_tlm_sot_offsets_buffer =
				(uint8_t*) grk_malloc(
						5
								* p_j2k->m_specific_param.m_encoder.m_total_tile_parts);
		if (!p_j2k->m_specific_param.m_encoder.m_tlm_sot_offsets_buffer) {
			return false;
		}

		p_j2k->m_specific_param.m_encoder.m_tlm_sot_offsets_current =
				p_j2k->m_specific_param.m_encoder.m_tlm_sot_offsets_buffer;
	}
	return true;
}


static uint32_t j2k_get_num_tp(grk_coding_parameters *cp, uint32_t pino, uint16_t tileno) {
	const char *prog = nullptr;
	int32_t i;
	uint32_t tpnum = 1;

	/*  preconditions */
	assert(tileno < (cp->tw * cp->th));
	assert(pino < (cp->tcps[tileno].numpocs + 1));

	/* get the given tile coding parameter */
	auto tcp = &cp->tcps[tileno];
	assert(tcp != nullptr);

	auto l_current_poc = &(tcp->pocs[pino]);
	assert(l_current_poc != 0);

	/* get the progression order as a character string */
	prog = j2k_convert_progression_order(tcp->prg);
	assert(strlen(prog) > 0);

	if (cp->m_coding_param.m_enc.m_tp_on == 1) {
		for (i = 0; i < 4; ++i) {
			switch (prog[i]) {
			/* component wise */
			case 'C':
				tpnum *= l_current_poc->compE;
				break;
				/* resolution wise */
			case 'R':
				tpnum *= l_current_poc->resE;
				break;
				/* precinct wise */
			case 'P':
				tpnum *= l_current_poc->prcE;
				break;
				/* layer wise */
			case 'L':
				tpnum *= l_current_poc->layE;
				break;
			}
			//we start a new tile part with every progression change
			if (cp->m_coding_param.m_enc.m_tp_flag == prog[i]) {
				cp->m_coding_param.m_enc.m_tp_pos = i;
				break;
			}
		}
	} else {
		tpnum = 1;
	}
	return tpnum;
}

char* j2k_convert_progression_order(GRK_PROG_ORDER prg_order) {
	j2k_prog_order *po;
	for (po = j2k_prog_order_list; po->enum_prog != -1; po++) {
		if (po->enum_prog == prg_order) {
			return (char*) po->str_prog;
		}
	}
	return po->str_prog;
}


static bool j2k_calculate_tp(grk_coding_parameters *cp, uint32_t *p_nb_tile_parts, grk_image *image) {
	uint32_t pino;
	uint16_t tileno;
	uint32_t l_nb_tiles;
	grk_tcp *tcp;

	assert(p_nb_tile_parts != nullptr);
	assert(cp != nullptr);
	assert(image != nullptr);


	l_nb_tiles = cp->tw * cp->th;
	*p_nb_tile_parts = 0;
	tcp = cp->tcps;

	/* INDEX >> */
	/* TODO mergeV2: check this part which use cstr_info */
	/*if (p_j2k->cstr_info) {
	  grk_tile_info  * l_info_tile_ptr = p_j2k->cstr_info->tile;
	 for (tileno = 0; tileno < l_nb_tiles; ++tileno) {
	 uint32_t cur_totnum_tp = 0;
	 pi_update_encoding_parameters(image,cp,tileno);
	 for (pino = 0; pino <= tcp->numpocs; ++pino)
	 {
	 uint32_t tp_num = j2k_get_num_tp(cp,pino,tileno);
	 *p_nb_tiles = *p_nb_tiles + tp_num;
	 cur_totnum_tp += tp_num;
	 }
	 tcp->m_nb_tile_parts = cur_totnum_tp;
	 l_info_tile_ptr->tp = ( grk_tp_info  *) grk_malloc(cur_totnum_tp * sizeof( grk_tp_info) );
	 if (l_info_tile_ptr->tp == nullptr) {
	 return false;
	 }
	 memset(l_info_tile_ptr->tp,0,cur_totnum_tp * sizeof( grk_tp_info) );
	 l_info_tile_ptr->num_tps = cur_totnum_tp;
	 ++l_info_tile_ptr;
	 ++tcp;
	 }
	 }
	 else */{
		for (tileno = 0; tileno < l_nb_tiles; ++tileno) {
			uint8_t cur_totnum_tp = 0;
			pi_update_encoding_parameters(image, cp, tileno);
			for (pino = 0; pino <= tcp->numpocs; ++pino) {
				uint32_t tp_num = j2k_get_num_tp(cp, pino, tileno);
				if (tp_num > 255) {
					GROK_ERROR(
							"Tile %d contains more than 255 tile parts, which is not permitted by the JPEG 2000 standard.\n",
							tileno);
					return false;
				}
				*p_nb_tile_parts += tp_num;
				cur_totnum_tp = (uint8_t)(cur_totnum_tp + tp_num);
			}
			tcp->m_nb_tile_parts = cur_totnum_tp;
			++tcp;
		}
	}
	return true;
}

template<typename S, typename D> void j2k_write(const void *p_src_data,
		void *p_dest_data, uint32_t nb_elem) {
	uint8_t *l_dest_data = (uint8_t*) p_dest_data;
	S *l_src_data = (S*) p_src_data;
	uint32_t i;
	D l_temp;
	for (i = 0; i < nb_elem; ++i) {
		l_temp = (D) *(l_src_data++);
		grk_write<D>(l_dest_data, l_temp, sizeof(D));
		l_dest_data += sizeof(D);
	}
}

static void j2k_read_int16_to_float(const void *p_src_data, void *p_dest_data,
		uint32_t nb_elem) {
	j2k_write<int16_t, float>(p_src_data, p_dest_data, nb_elem);
}
static void j2k_read_int32_to_float(const void *p_src_data, void *p_dest_data,
		uint32_t nb_elem) {
	j2k_write<int32_t, float>(p_src_data, p_dest_data, nb_elem);
}
static void j2k_read_float32_to_float(const void *p_src_data, void *p_dest_data,
		uint32_t nb_elem) {
	j2k_write<float, float>(p_src_data, p_dest_data, nb_elem);
}
static void j2k_read_float64_to_float(const void *p_src_data, void *p_dest_data,
		uint32_t nb_elem) {
	j2k_write<double, float>(p_src_data, p_dest_data, nb_elem);
}
static void j2k_read_int16_to_int32(const void *p_src_data, void *p_dest_data,
		uint32_t nb_elem) {
	j2k_write<int16_t, int32_t>(p_src_data, p_dest_data, nb_elem);
}
static void j2k_read_int32_to_int32(const void *p_src_data, void *p_dest_data,
		uint32_t nb_elem) {
	j2k_write<int32_t, int32_t>(p_src_data, p_dest_data, nb_elem);
}
static void j2k_read_float32_to_int32(const void *p_src_data, void *p_dest_data,
		uint32_t nb_elem) {
	j2k_write<float, int32_t>(p_src_data, p_dest_data, nb_elem);
}
static void j2k_read_float64_to_int32(const void *p_src_data, void *p_dest_data,
		uint32_t nb_elem) {
	j2k_write<double, int32_t>(p_src_data, p_dest_data, nb_elem);
}
static void j2k_write_float_to_int16(const void *p_src_data, void *p_dest_data,
		uint32_t nb_elem) {
	j2k_write<float, int16_t>(p_src_data, p_dest_data, nb_elem);
}
static void j2k_write_float_to_int32(const void *p_src_data, void *p_dest_data,
		uint32_t nb_elem) {
	j2k_write<float, int32_t>(p_src_data, p_dest_data, nb_elem);
}
static void j2k_write_float_to_float(const void *p_src_data, void *p_dest_data,
		uint32_t nb_elem) {
	j2k_write<float, float>(p_src_data, p_dest_data, nb_elem);
}
static void j2k_write_float_to_float64(const void *p_src_data,
		void *p_dest_data, uint32_t nb_elem) {
	j2k_write<float, double>(p_src_data, p_dest_data, nb_elem);
}

/**************************
 * Read/Write Markers
 *************************/

static bool j2k_write_soc(grk_j2k *p_j2k, BufferedStream *p_stream) {
	assert(p_stream != nullptr);
	assert(p_j2k != nullptr);
	
	(void) p_j2k;
	return p_stream->write_short(J2K_MS_SOC);
}

/**
 * Reads a SOC marker (Start of Codestream)
 * @param       p_j2k           the jpeg2000 file codec.
 * @param       p_stream        FIXME DOC

 */
static bool j2k_read_soc(grk_j2k *p_j2k, BufferedStream *p_stream) {
	uint8_t l_data[2];
	uint32_t l_marker;

	assert(p_j2k != nullptr);
	assert(p_stream != nullptr);

	if (p_stream->read(l_data, 2) != 2) {
		return false;
	}

	grk_read_bytes(l_data, &l_marker, 2);
	if (l_marker != J2K_MS_SOC) {
		return false;
	}

	/* Next marker should be a SIZ marker in the main header */
	p_j2k->m_specific_param.m_decoder.m_state = J2K_DEC_STATE_MHSIZ;

	if (p_j2k->cstr_index) {
		/* FIXME move it in a index structure included in p_j2k*/
		p_j2k->cstr_index->main_head_start = p_stream->tell() - 2;

		//event_msg( EVT_INFO, "Start to read j2k main header (%d).\n", p_j2k->cstr_index->main_head_start);

		/* Add the marker to the codestream index*/
		if (!j2k_add_mhmarker(p_j2k->cstr_index, J2K_MS_SOC,
						p_j2k->cstr_index->main_head_start, 2)) {
			GROK_ERROR(
					"Not enough memory to add mh marker");
			return false;
		}
	}
	return true;
}

static bool j2k_write_siz(grk_j2k *p_j2k, BufferedStream *p_stream) {
	uint32_t i;
	uint32_t l_size_len;

	assert(p_stream != nullptr);
	assert(p_j2k != nullptr);
	

	auto l_image = p_j2k->m_private_image;
	auto cp = &(p_j2k->m_cp);
	l_size_len = 40 + 3 * l_image->numcomps;
	auto l_img_comp = l_image->comps;

	/* write SOC identifier */

	/* SIZ */
	if (!p_stream->write_short(J2K_MS_SIZ)) {
		return false;
	}

	/* L_SIZ */
	if (!p_stream->write_short((uint16_t) (l_size_len - 2))) {
		return false;
	}

	/* Rsiz (capabilities) */
	if (!p_stream->write_short(cp->rsiz)) {
		return false;
	}

	/* Xsiz */
	if (!p_stream->write_int(l_image->x1)) {
		return false;
	}

	/* Ysiz */
	if (!p_stream->write_int(l_image->y1)) {
		return false;
	}

	/* X0siz */
	if (!p_stream->write_int(l_image->x0)) {
		return false;
	}

	/* Y0siz */
	if (!p_stream->write_int(l_image->y0)) {
		return false;
	}

	/* XTsiz */
	if (!p_stream->write_int(cp->tdx)) {
		return false;
	}

	/* YTsiz */
	if (!p_stream->write_int(cp->tdy)) {
		return false;
	}

	/* XT0siz */
	if (!p_stream->write_int(cp->tx0)) {
		return false;
	}

	/* YT0siz */
	if (!p_stream->write_int(cp->ty0)) {
		return false;
	}

	/* Csiz */
	if (!p_stream->write_short((uint16_t) l_image->numcomps)) {
		return false;
	}

	for (i = 0; i < l_image->numcomps; ++i) {
		/* TODO here with MCT ? */
		/* Ssiz_i */
		if (!p_stream->write_byte(
				(uint8_t) (l_img_comp->prec - 1 + (l_img_comp->sgnd << 7)))) {
			return false;
		}

		/* XRsiz_i */
		if (!p_stream->write_byte((uint8_t) l_img_comp->dx)) {
			return false;
		}

		/* YRsiz_i */
		if (!p_stream->write_byte((uint8_t) l_img_comp->dy)) {
			return false;
		}
		++l_img_comp;
	}
	return true;
}

/**
 * Reads a CAP marker
 * @param       p_j2k           the jpeg2000 file codec.
 * @param       p_header_data   the data contained in the SIZ box.
 * @param       header_size   the size of the data contained in the SIZ marker.

 */
static bool j2k_read_cap(grk_j2k *p_j2k, uint8_t *p_header_data,
		uint16_t header_size) {
	grk_coding_parameters *l_cp = &(p_j2k->m_cp);

	if (header_size < 6) {
		GROK_ERROR( "Error with SIZ marker size");
		return false;
	}

	uint32_t l_tmp;
	grk_read_bytes(p_header_data, &l_tmp, 4); /* Pcap */
	bool validPcap = true;
    if (l_tmp & 0xFFFDFFFF){
      GROK_WARN("Pcap in CAP marker has unsupported options.");
    }
    if ((l_tmp & 0x00020000) == 0) {
    	GROK_WARN("Pcap in CAP marker should have its 15th MSB set. "
        " Ignoring CAP.");
        validPcap = false;
    }
    if (validPcap){
    	l_cp->pcap = l_tmp;
    	grk_read_bytes(p_header_data, &l_tmp, 2); /* Ccap */
    	l_cp->ccap = (uint16_t)l_tmp;
    }

	return true;
}


static bool j2k_write_cap(grk_j2k *p_j2k, BufferedStream *p_stream) {
	assert(p_j2k != nullptr);
	assert(p_stream != nullptr);

	auto l_cp = &(p_j2k->m_cp);
	auto l_tcp = &l_cp->tcps[0];
	auto l_tccp0 = &l_tcp->tccps[0];

    //marker size excluding header
    uint16_t Lcap = 8;

    uint32_t Pcap = 0x00020000; //for jph, Pcap^15 must be set, the 15th MSB
    uint16_t Ccap[32]; //a maximum of 32
    memset(Ccap, 0, sizeof(Ccap));

    bool reversible = l_tccp0->qmfbid == 1;
    if (reversible)
	  Ccap[0] &= 0xFFDF;
	else
	  Ccap[0] |= 0x0020;
	Ccap[0] &= 0xFFE0;

	int Bp = 0;
	int B = l_tcp->qcd.get_MAGBp();
	if (B <= 8)
	  Bp = 0;
	else if (B < 28)
	  Bp = B - 8;
	else if (B < 48)
	  Bp = 13 + (B >> 2);
	else
	  Bp = 31;
	Ccap[0] = (uint16_t)(Ccap[0] | Bp);

	/* CAP */
	if (!p_stream->write_short(J2K_MS_CAP)) {
		return false;
	}

	/* L_CAP */
	if (!p_stream->write_short(Lcap)) {
		return false;
	}

	/* PCAP */
	if (!p_stream->write_int(Pcap)) {
		return false;
	}

	/* CCAP */
	if (!p_stream->write_short(Ccap[0])) {
		return false;
	}
	return true;
}

/**
 * Reads a SIZ marker (image and tile size)
 * @param       p_j2k           the jpeg2000 file codec.
 * @param       p_header_data   the data contained in the SIZ box.
 * @param       header_size   the size of the data contained in the SIZ marker.

 */
static bool j2k_read_siz(grk_j2k *p_j2k, uint8_t *p_header_data,
		uint16_t header_size) {
	uint32_t i;
	uint32_t l_nb_comp;
	uint32_t l_nb_comp_remain;
	uint32_t l_remaining_size;
	uint32_t l_nb_tiles;
	uint32_t l_tmp, l_tx1, l_ty1;
	 grk_image_comp  *l_img_comp = nullptr;
	grk_tcp *l_current_tile_param = nullptr;

	assert(p_j2k != nullptr);
	assert(p_header_data != nullptr);

	auto l_image = p_j2k->m_private_image;
	auto l_cp = &(p_j2k->m_cp);

	/* minimum size == 39 - 3 (= minimum component parameter) */
	if (header_size < 36) {
		GROK_ERROR( "Error with SIZ marker size");
		return false;
	}

	l_remaining_size = header_size - 36;
	l_nb_comp = l_remaining_size / 3;
	l_nb_comp_remain = l_remaining_size % 3;
	if (l_nb_comp_remain != 0) {
		GROK_ERROR( "Error with SIZ marker size");
		return false;
	}

	grk_read_bytes(p_header_data, &l_tmp, 2); /* Rsiz (capabilities) */
	p_header_data += 2;

	// sanity check on RSIZ
	uint16_t profile = 0;
	uint16_t part2_extensions = 0;
	// check for Part 2
	if (l_tmp & GRK_PROFILE_PART2) {
		profile = GRK_PROFILE_PART2;
		part2_extensions = l_tmp & GRK_PROFILE_PART2_EXTENSIONS_MASK;
		(void) part2_extensions;
	} else {
		profile = l_tmp & GRK_PROFILE_MASK;
		if ((profile > GRK_PROFILE_CINEMA_LTS)
				&& !GRK_IS_BROADCAST(profile) && !GRK_IS_IMF(profile)) {
			GROK_ERROR(
					"Non-compliant Rsiz value 0x%x in SIZ marker", l_tmp);
			return false;
		}
	}

	l_cp->rsiz = (uint16_t) l_tmp;
	grk_read_bytes(p_header_data, (uint32_t*) &l_image->x1, 4); /* Xsiz */
	p_header_data += 4;
	grk_read_bytes(p_header_data, (uint32_t*) &l_image->y1, 4); /* Ysiz */
	p_header_data += 4;
	grk_read_bytes(p_header_data, (uint32_t*) &l_image->x0, 4); /* X0siz */
	p_header_data += 4;
	grk_read_bytes(p_header_data, (uint32_t*) &l_image->y0, 4); /* Y0siz */
	p_header_data += 4;
	grk_read_bytes(p_header_data, (uint32_t*) &l_cp->tdx, 4); /* XTsiz */
	p_header_data += 4;
	grk_read_bytes(p_header_data, (uint32_t*) &l_cp->tdy, 4); /* YTsiz */
	p_header_data += 4;
	grk_read_bytes(p_header_data, (uint32_t*) &l_cp->tx0, 4); /* XT0siz */
	p_header_data += 4;
	grk_read_bytes(p_header_data, (uint32_t*) &l_cp->ty0, 4); /* YT0siz */
	p_header_data += 4;
	grk_read_bytes(p_header_data, (uint32_t*) &l_tmp, 2); /* Csiz */
	p_header_data += 2;
	if (l_tmp <= max_num_components)
		l_image->numcomps = (uint16_t) l_tmp;
	else {
		GROK_ERROR(
				"Error with SIZ marker: number of component is illegal -> %d\n",
				l_tmp);
		return false;
	}

	if (l_image->numcomps != l_nb_comp) {
		GROK_ERROR(
				"Error with SIZ marker: number of component is not compatible with the remaining number of parameters ( %d vs %d)\n",
				l_image->numcomps, l_nb_comp);
		return false;
	}

	/* testcase 4035.pdf.SIGSEGV.d8b.3375 */
	/* testcase issue427-null-image-size.jp2 */
	if ((l_image->x0 >= l_image->x1) || (l_image->y0 >= l_image->y1)) {
		std::stringstream ss;
		ss << "Error with SIZ marker: negative or zero image size ("
				<< (int64_t) l_image->x1 - l_image->x0 << " x "
				<< (int64_t) l_image->y1 - l_image->y0 << ")" << std::endl;
		GROK_ERROR( "%s", ss.str().c_str());
		return false;
	}
	/* testcase 2539.pdf.SIGFPE.706.1712 (also 3622.pdf.SIGFPE.706.2916 and 4008.pdf.SIGFPE.706.3345 and maybe more) */
	if ((l_cp->tdx == 0U) || (l_cp->tdy == 0U)) {
		GROK_ERROR(
				"Error with SIZ marker: invalid tile size (tdx: %d, tdy: %d)\n",
				l_cp->tdx, l_cp->tdy);
		return false;
	}

	/* testcase issue427-illegal-tile-offset.jp2 */
	l_tx1 = uint_adds(l_cp->tx0, l_cp->tdx); /* manage overflow */
	l_ty1 = uint_adds(l_cp->ty0, l_cp->tdy); /* manage overflow */
	if ((l_cp->tx0 > l_image->x0) || (l_cp->ty0 > l_image->y0)
			|| (l_tx1 <= l_image->x0) || (l_ty1 <= l_image->y0)) {
		GROK_ERROR(
				"Error with SIZ marker: illegal tile offset");
		return false;
	}

	uint64_t tileArea = (uint64_t) (l_tx1 - l_cp->tx0) * (l_ty1 - l_cp->ty0);
	if (tileArea > max_tile_area) {
		GROK_ERROR(
				"Error with SIZ marker: tile area = %llu greater than max tile area = %llu\n",
				tileArea, max_tile_area);
		return false;

	}

	/* Allocate the resulting image components */
	l_image->comps = ( grk_image_comp  * ) grk_calloc(l_image->numcomps,
			sizeof( grk_image_comp) );
	if (l_image->comps == nullptr) {
		l_image->numcomps = 0;
		GROK_ERROR(
				"Not enough memory to take in charge SIZ marker");
		return false;
	}

	l_img_comp = l_image->comps;

	/* Read the component information */
	for (i = 0; i < l_image->numcomps; ++i) {
		uint32_t tmp;
		grk_read_bytes(p_header_data, &tmp, 1); /* Ssiz_i */
		++p_header_data;
		l_img_comp->prec = (tmp & 0x7f) + 1;
		l_img_comp->sgnd = tmp >> 7;
		grk_read_bytes(p_header_data, &tmp, 1); /* XRsiz_i */
		++p_header_data;
		l_img_comp->dx = tmp; /* should be between 1 and 255 */
		grk_read_bytes(p_header_data, &tmp, 1); /* YRsiz_i */
		++p_header_data;
		l_img_comp->dy = tmp; /* should be between 1 and 255 */
		if (l_img_comp->dx < 1 || l_img_comp->dx > 255 || l_img_comp->dy < 1
				|| l_img_comp->dy > 255) {
			GROK_ERROR(
					"Invalid values for comp = %d : dx=%u dy=%u\n (should be between 1 and 255 according to the JPEG2000 standard)",
					i, l_img_comp->dx, l_img_comp->dy);
			return false;
		}

		if (l_img_comp->prec == 0
				|| l_img_comp->prec > max_supported_precision) {
			GROK_ERROR(
					"Unsupported precision for comp = %d : prec=%u (Grok only supportes precision between 1 and %d)\n",
					i, l_img_comp->prec, max_supported_precision);
			return false;
		}
		l_img_comp->resno_decoded = 0; /* number of resolution decoded */
		++l_img_comp;
	}

	/* Compute the number of tiles */
	l_cp->tw = ceildiv<uint32_t>(l_image->x1 - l_cp->tx0, l_cp->tdx);
	l_cp->th = ceildiv<uint32_t>(l_image->y1 - l_cp->ty0, l_cp->tdy);

	/* Check that the number of tiles is valid */
	if (l_cp->tw == 0 || l_cp->th == 0) {
		GROK_ERROR(
				"Invalid grid of tiles: %u x %u. JPEG 2000 standard requires at least one tile in grid. \n",
				l_cp->tw, l_cp->th);
		return false;
	}
	if (l_cp->tw * l_cp->th > max_num_tiles) {
		GROK_ERROR(
				"Invalid grid of tiles : %u x %u.  JPEG 2000 standard specifies maximum of %d tiles\n",max_num_tiles,
				l_cp->tw, l_cp->th);
		return false;
	}
	l_nb_tiles = l_cp->tw * l_cp->th;

	/* Define the tiles which will be decoded */
	if (p_j2k->m_specific_param.m_decoder.m_discard_tiles) {
		p_j2k->m_specific_param.m_decoder.m_start_tile_x =
				(p_j2k->m_specific_param.m_decoder.m_start_tile_x - l_cp->tx0)
						/ l_cp->tdx;
		p_j2k->m_specific_param.m_decoder.m_start_tile_y =
				(p_j2k->m_specific_param.m_decoder.m_start_tile_y - l_cp->ty0)
						/ l_cp->tdy;
		p_j2k->m_specific_param.m_decoder.m_end_tile_x = ceildiv<uint32_t>(
				(p_j2k->m_specific_param.m_decoder.m_end_tile_x - l_cp->tx0),
				l_cp->tdx);
		p_j2k->m_specific_param.m_decoder.m_end_tile_y = ceildiv<uint32_t>(
				(p_j2k->m_specific_param.m_decoder.m_end_tile_y - l_cp->ty0),
				l_cp->tdy);
	} else {
		p_j2k->m_specific_param.m_decoder.m_start_tile_x = 0;
		p_j2k->m_specific_param.m_decoder.m_start_tile_y = 0;
		p_j2k->m_specific_param.m_decoder.m_end_tile_x = l_cp->tw;
		p_j2k->m_specific_param.m_decoder.m_end_tile_y = l_cp->th;
	}

	/* memory allocations */
	l_cp->tcps = new grk_tcp[l_nb_tiles];
	p_j2k->m_specific_param.m_decoder.m_default_tcp->tccps =
			(grk_tccp*) grk_calloc(l_image->numcomps, sizeof(grk_tccp));
	if (p_j2k->m_specific_param.m_decoder.m_default_tcp->tccps == nullptr) {
		GROK_ERROR(
				"Not enough memory to take in charge SIZ marker");
		return false;
	}

	p_j2k->m_specific_param.m_decoder.m_default_tcp->m_mct_records =
			(grk_mct_data*) grk_calloc(default_number_mct_records,
					sizeof(grk_mct_data));

	if (!p_j2k->m_specific_param.m_decoder.m_default_tcp->m_mct_records) {
		GROK_ERROR(
				"Not enough memory to take in charge SIZ marker");
		return false;
	}
	p_j2k->m_specific_param.m_decoder.m_default_tcp->m_nb_max_mct_records =
			default_number_mct_records;

	p_j2k->m_specific_param.m_decoder.m_default_tcp->m_mcc_records =
			(grk_simple_mcc_decorrelation_data*) grk_calloc(
					default_number_mcc_records,
					sizeof(grk_simple_mcc_decorrelation_data));

	if (!p_j2k->m_specific_param.m_decoder.m_default_tcp->m_mcc_records) {
		GROK_ERROR(
				"Not enough memory to take in charge SIZ marker");
		return false;
	}
	p_j2k->m_specific_param.m_decoder.m_default_tcp->m_nb_max_mcc_records =
			default_number_mcc_records;

	/* set up default dc level shift */
	for (i = 0; i < l_image->numcomps; ++i) {
		if (!l_image->comps[i].sgnd) {
			p_j2k->m_specific_param.m_decoder.m_default_tcp->tccps[i].m_dc_level_shift =
					1 << (l_image->comps[i].prec - 1);
		}
	}

	l_current_tile_param = l_cp->tcps;
	for (i = 0; i < l_nb_tiles; ++i) {
		l_current_tile_param->tccps = (grk_tccp*) grk_calloc(l_image->numcomps,
				sizeof(grk_tccp));
		if (l_current_tile_param->tccps == nullptr) {
			GROK_ERROR(
					"Not enough memory to take in charge SIZ marker");
			return false;
		}

		++l_current_tile_param;
	}

	p_j2k->m_specific_param.m_decoder.m_state = J2K_DEC_STATE_MH;
	grk_image_comp_header_update(l_image, l_cp);

	return true;
}

static bool j2k_write_com(grk_j2k *p_j2k, BufferedStream *p_stream) {
	assert(p_j2k != nullptr);
	assert(p_stream != nullptr);
	
	for (uint32_t i = 0; i < p_j2k->m_cp.num_comments; ++i) {
		const char *l_comment = p_j2k->m_cp.comment[i];
		uint16_t l_comment_size = p_j2k->m_cp.comment_len[i];
		if (!l_comment_size) {
			GROK_WARN( "Empty comment. Ignoring");
			continue;
		}
		if (l_comment_size > GRK_MAX_COMMENT_LENGTH) {
			GROK_WARN(
					"Comment length %s is greater than maximum comment length %d. Ignoring\n",
					l_comment_size, GRK_MAX_COMMENT_LENGTH);
			continue;
		}
		uint32_t l_total_com_size = (uint32_t) l_comment_size + 6;

		/* COM */
		if (!p_stream->write_short(J2K_MS_COM)) {
			return false;
		}

		/* L_COM */
		if (!p_stream->write_short((uint16_t) (l_total_com_size - 2))) {
			return false;
		}

		if (!p_stream->write_short(p_j2k->m_cp.isBinaryComment[i] ? 0 : 1)) {
			return false;
		}

		if (!p_stream->write_bytes((uint8_t*) l_comment, l_comment_size)) {
			return false;
		}
	}
	return true;
}

/**
 * Reads a COM marker (comments)
 * @param       p_j2k           the jpeg2000 file codec.
 * @param       p_header_data   the data contained in the COM box.
 * @param       header_size   the size of the data contained in the COM marker.

 */
static bool j2k_read_com(grk_j2k *p_j2k, uint8_t *p_header_data,
		uint16_t header_size) {
	assert(p_j2k != nullptr);
	assert(p_header_data != nullptr);
	assert(header_size != 0);

	if (header_size < 2) {
		GROK_ERROR( "j2k_read_com: Corrupt COM segment ");
		return false;
	} else if (header_size == 2) {
		GROK_WARN(
				"j2k_read_com: Empty COM segment. Ignoring ");
		return true;
	}
	if (p_j2k->m_cp.num_comments == GRK_NUM_COMMENTS_SUPPORTED) {
		GROK_WARN(
				"j2k_read_com: Only %d comments are supported. Ignoring\n",
				GRK_NUM_COMMENTS_SUPPORTED);
		return true;
	}

	uint32_t commentType;
	grk_read_bytes(p_header_data, &commentType, 2);
	auto numComments = p_j2k->m_cp.num_comments;
	p_j2k->m_cp.isBinaryComment[numComments] = (commentType == 0);
	if (commentType > 1) {
		GROK_WARN(
				"j2k_read_com: Unrecognized comment type 0x%x. Assuming IS 8859-15:1999 (Latin) values", commentType);
	}

	p_header_data += 2;
	uint16_t commentSize = (uint16_t)(header_size - 2);
	size_t commentSizeToAlloc = commentSize;
	if (!p_j2k->m_cp.isBinaryComment[numComments])
		commentSizeToAlloc++;
	p_j2k->m_cp.comment[numComments] = (char*) grk_buffer_new(
			commentSizeToAlloc);
	if (!p_j2k->m_cp.comment[numComments]) {
		GROK_ERROR(
				"j2k_read_com: Out of memory when allocating memory for comment ");
		return false;
	}
	memcpy(p_j2k->m_cp.comment[numComments], p_header_data, commentSize);
	p_j2k->m_cp.comment_len[numComments] = commentSize;

	// make null-terminated string
	if (!p_j2k->m_cp.isBinaryComment[numComments])
		p_j2k->m_cp.comment[numComments][commentSize] = 0;
	p_j2k->m_cp.num_comments++;
	return true;
}

static bool j2k_write_cod(grk_j2k *p_j2k, BufferedStream *p_stream) {
	uint32_t l_code_size;

	assert(p_j2k != nullptr);
	assert(p_stream != nullptr);

	auto l_cp = &(p_j2k->m_cp);
	auto l_tcp = &l_cp->tcps[p_j2k->m_current_tile_number];
	l_code_size = 9
			+ j2k_get_SPCod_SPCoc_size(p_j2k, p_j2k->m_current_tile_number, 0);

	/* COD */
	if (!p_stream->write_short(J2K_MS_COD)) {
		return false;
	}

	/* L_COD */
	if (!p_stream->write_short((uint16_t) (l_code_size - 2))) {
		return false;
	}

	/* Scod */
	if (!p_stream->write_byte((uint8_t) l_tcp->csty)) {
		return false;
	}

	/* SGcod (A) */
	if (!p_stream->write_byte((uint8_t) l_tcp->prg)) {
		return false;
	}

	/* SGcod (B) */
	if (!p_stream->write_short((uint16_t) l_tcp->numlayers)) {
		return false;
	}

	/* SGcod (C) */
	if (!p_stream->write_byte((uint8_t) l_tcp->mct)) {
		return false;
	}

	if (!j2k_write_SPCod_SPCoc(p_j2k, p_j2k->m_current_tile_number, 0, p_stream)) {
		GROK_ERROR( "Error writing COD marker");
		return false;
	}

	return true;
}


static void j2k_copy_tile_component_parameters(grk_j2k *p_j2k) {
	/* loop */
	uint32_t i;
	uint32_t l_prc_size;

	assert(p_j2k != nullptr);

	auto l_tcp = p_j2k->get_current_decode_tcp();
	auto l_ref_tccp = &l_tcp->tccps[0];
	auto l_copied_tccp = l_ref_tccp + 1;
	l_prc_size = l_ref_tccp->numresolutions * (uint32_t) sizeof(uint32_t);

	for (i = 1; i < p_j2k->m_private_image->numcomps; ++i) {
		l_copied_tccp->numresolutions = l_ref_tccp->numresolutions;
		l_copied_tccp->cblkw = l_ref_tccp->cblkw;
		l_copied_tccp->cblkh = l_ref_tccp->cblkh;
		l_copied_tccp->cblk_sty = l_ref_tccp->cblk_sty;
		l_copied_tccp->qmfbid = l_ref_tccp->qmfbid;
		memcpy(l_copied_tccp->prcw, l_ref_tccp->prcw, l_prc_size);
		memcpy(l_copied_tccp->prch, l_ref_tccp->prch, l_prc_size);
		++l_copied_tccp;
	}
}

/**
 * Reads a COD marker (Coding Style defaults)
 * @param       p_header_data   the data contained in the COD box.
 * @param       p_j2k                   the jpeg2000 codec.
 * @param       header_size   the size of the data contained in the COD marker.

 */
static bool j2k_read_cod(grk_j2k *p_j2k, uint8_t *p_header_data,
		uint16_t header_size) {
	/* loop */
	uint32_t i;
	uint32_t l_tmp;
	assert(p_header_data != nullptr);
	assert(p_j2k != nullptr);

	auto l_image = p_j2k->m_private_image;
	auto l_cp = &(p_j2k->m_cp);

	/* If we are in the first tile-part header of the current tile */
	auto l_tcp = p_j2k->get_current_decode_tcp();

	/* Only one COD per tile */
	if (l_tcp->cod) {
		GROK_WARN(
				"Multiple COD markers detected for tile part %d. The JPEG 2000 standard does not allow more than one COD marker per tile.\n",
				l_tcp->m_current_tile_part_number);
	}
	l_tcp->cod = 1;

	/* Make sure room is sufficient */
	if (header_size < 5) {
		GROK_ERROR( "Error reading COD marker");
		return false;
	}

	grk_read_bytes(p_header_data, &l_tcp->csty, 1); /* Scod */
	++p_header_data;
	/* Make sure we know how to decode this */
	if ((l_tcp->csty
			& ~(uint32_t) (J2K_CP_CSTY_PRT | J2K_CP_CSTY_SOP | J2K_CP_CSTY_EPH))
			!= 0U) {
		GROK_ERROR( "Unknown Scod value in COD marker");
		return false;
	}
	grk_read_bytes(p_header_data, &l_tmp, 1); /* SGcod (A) */
	++p_header_data;
	l_tcp->prg = (GRK_PROG_ORDER) l_tmp;
	/* Make sure progression order is valid */
	if (l_tcp->prg > GRK_CPRL) {
		GROK_ERROR(
				"Unknown progression order in COD marker");
		l_tcp->prg = GRK_PROG_UNKNOWN;
	}
	grk_read_bytes(p_header_data, &l_tcp->numlayers, 2); /* SGcod (B) */
	p_header_data += 2;

	if ((l_tcp->numlayers < 1U) || (l_tcp->numlayers > 65535U)) {
		GROK_ERROR("Invalid number of layers in COD marker : %d not in range [1-65535]\n",
				l_tcp->numlayers);
		return false;
	}

	/* If user didn't set a number layer to decode take the max specify in the codestream. */
	if (l_cp->m_coding_param.m_dec.m_layer) {
		l_tcp->num_layers_to_decode = l_cp->m_coding_param.m_dec.m_layer;
	} else {
		l_tcp->num_layers_to_decode = l_tcp->numlayers;
	}

	grk_read_bytes(p_header_data, &l_tcp->mct, 1); /* SGcod (C) */
	++p_header_data;
	if (l_tcp->mct > 1 )
	{
		GROK_ERROR("Invalid MCT value : %d. Should be either 0 or 1", l_tcp->mct);
		return false;
	}
	header_size = (uint16_t)(header_size - 5);
	for (i = 0; i < l_image->numcomps; ++i) {
		l_tcp->tccps[i].csty = l_tcp->csty & J2K_CCP_CSTY_PRT;
	}

	if (!j2k_read_SPCod_SPCoc(p_j2k, 0, p_header_data, &header_size)) {
		GROK_ERROR( "Error reading COD marker");
		return false;
	}

	if (header_size != 0) {
		GROK_ERROR( "Error reading COD marker");
		return false;
	}

	/* Apply the coding style to other components of the current tile or the m_default_tcp*/
	j2k_copy_tile_component_parameters(p_j2k);

	return true;
}

static bool j2k_write_coc(grk_j2k *p_j2k, uint32_t comp_no,
		BufferedStream *p_stream) {
	assert(p_j2k != nullptr);
	
	assert(p_stream != nullptr);
	return j2k_write_coc_in_memory(p_j2k, comp_no, p_stream);

}

static bool j2k_compare_coc(grk_j2k *p_j2k, uint32_t first_comp_no,
		uint32_t second_comp_no) {
	grk_coding_parameters *l_cp = nullptr;
	grk_tcp *l_tcp = nullptr;

	assert(p_j2k != nullptr);

	l_cp = &(p_j2k->m_cp);
	l_tcp = &l_cp->tcps[p_j2k->m_current_tile_number];

	if (l_tcp->tccps[first_comp_no].csty
			!= l_tcp->tccps[second_comp_no].csty) {
		return false;
	}
	return j2k_compare_SPCod_SPCoc(p_j2k, p_j2k->m_current_tile_number,
			first_comp_no, second_comp_no);
}

static bool j2k_write_coc_in_memory(grk_j2k *p_j2k, uint32_t comp_no,
		BufferedStream *p_stream) {
	uint32_t l_coc_size;
	uint32_t l_comp_room;

	assert(p_j2k != nullptr);

	auto l_cp = &(p_j2k->m_cp);
	auto l_tcp = &l_cp->tcps[p_j2k->m_current_tile_number];
	auto l_image = p_j2k->m_private_image;
	l_comp_room = (l_image->numcomps <= 256) ? 1 : 2;
	l_coc_size = 5 + l_comp_room
			+ j2k_get_SPCod_SPCoc_size(p_j2k, p_j2k->m_current_tile_number,
					comp_no);

	/* COC */
	if (!p_stream->write_short(J2K_MS_COC)) {
		return false;
	}

	/* L_COC */
	if (!p_stream->write_short((uint16_t) (l_coc_size - 2))) {
		return false;
	}

	/* Ccoc */
	if (l_comp_room == 2) {
		if (!p_stream->write_short((uint16_t) comp_no)) {
			return false;
		}
	} else {
		if (!p_stream->write_byte((uint8_t) comp_no)) {
			return false;
		}
	}

	/* Scoc */
	if (!p_stream->write_byte((uint8_t) l_tcp->tccps[comp_no].csty)) {
		return false;
	}
	return j2k_write_SPCod_SPCoc(p_j2k, p_j2k->m_current_tile_number, 0,
			p_stream);
}

static uint32_t j2k_get_max_coc_size(grk_j2k *p_j2k) {
	uint16_t i;
	uint32_t j;
	uint32_t l_nb_comp;
	uint32_t l_nb_tiles;
	uint32_t l_max = 0;

	l_nb_tiles = p_j2k->m_cp.tw * p_j2k->m_cp.th;
	l_nb_comp = p_j2k->m_private_image->numcomps;

	for (i = 0; i < l_nb_tiles; ++i) {
		for (j = 0; j < l_nb_comp; ++j) {
			l_max = std::max<uint32_t>(l_max,
					j2k_get_SPCod_SPCoc_size(p_j2k, i, j));
		}
	}
	return 6 + l_max;
}

/**
 * Reads a COC marker (Coding Style Component)
 * @param       p_header_data   the data contained in the COC box.
 * @param       p_j2k                   the jpeg2000 codec.
 * @param       header_size   the size of the data contained in the COC marker.

 */
static bool j2k_read_coc(grk_j2k *p_j2k, uint8_t *p_header_data,
		uint16_t header_size) {
	uint32_t l_comp_room;
	uint32_t l_comp_no;

	assert(p_header_data != nullptr);
	assert(p_j2k != nullptr);
	

	auto l_tcp = p_j2k->get_current_decode_tcp();
	auto l_image = p_j2k->m_private_image;

	l_comp_room = l_image->numcomps <= 256 ? 1 : 2;

	/* make sure room is sufficient*/
	if (header_size < l_comp_room + 1) {
		GROK_ERROR( "Error reading COC marker");
		return false;
	}
	header_size = (uint16_t)(header_size - (l_comp_room + 1));

	grk_read_bytes(p_header_data, &l_comp_no, l_comp_room); /* Ccoc */
	p_header_data += l_comp_room;
	if (l_comp_no >= l_image->numcomps) {
		GROK_ERROR(
				"Error reading COC marker (bad number of components)");
		return false;
	}

	grk_read_8(p_header_data, &l_tcp->tccps[l_comp_no].csty); /* Scoc */
	++p_header_data;

	if (!j2k_read_SPCod_SPCoc(p_j2k, l_comp_no, p_header_data, &header_size)) {
		GROK_ERROR( "Error reading COC marker");
		return false;
	}

	if (header_size != 0) {
		GROK_ERROR( "Error reading COC marker");
		return false;
	}
	return true;
}

static bool j2k_write_qcd(grk_j2k *p_j2k, BufferedStream *p_stream) {
	uint32_t l_qcd_size;

	assert(p_j2k != nullptr);
	assert(p_stream != nullptr);

	l_qcd_size = 4
			+ j2k_get_SQcd_SQcc_size(p_j2k, p_j2k->m_current_tile_number, 0);

	/* QCD */
	if (!p_stream->write_short(J2K_MS_QCD)) {
		return false;
	}

	/* L_QCD */
	if (!p_stream->write_short((uint16_t) (l_qcd_size - 2))) {
		return false;
	}

	if (!j2k_write_SQcd_SQcc(p_j2k, p_j2k->m_current_tile_number, 0, p_stream)) {
		GROK_ERROR( "Error writing QCD marker");
		return false;
	}

	return true;
}

/**
 * Reads a QCD marker (Quantization defaults)
 * @param       p_j2k           the jpeg2000 codec.
 * @param       p_header_data   the data contained in the QCD box.
 * @param       header_size   the size of the data contained in the QCD marker.

 */
static bool j2k_read_qcd(grk_j2k *p_j2k, uint8_t *p_header_data,
		uint16_t header_size) {
	assert(p_header_data != nullptr);
	assert(p_j2k != nullptr);
	
	if (!j2k_read_SQcd_SQcc(false, p_j2k, 0, p_header_data, &header_size)) {
		GROK_ERROR( "Error reading QCD marker");
		return false;
	}
	if (header_size != 0) {
		GROK_ERROR( "Error reading QCD marker");
		return false;
	}

	// Apply the quantization parameters to the other components
	// of the current tile or m_default_tcp
	auto l_tcp = p_j2k->get_current_decode_tcp();
	auto l_ref_tccp = l_tcp->tccps;
	for (uint32_t i = 1; i < p_j2k->m_private_image->numcomps; ++i) {
		auto l_target_tccp = l_ref_tccp + i;
		l_target_tccp->quant.apply_quant(l_ref_tccp, l_target_tccp);
	}
	return true;
}

static bool j2k_write_qcc(grk_j2k *p_j2k, uint32_t comp_no,
		BufferedStream *p_stream) {
	assert(p_j2k != nullptr);
	assert(p_stream != nullptr);
	return j2k_write_qcc_in_memory(p_j2k, comp_no, p_stream);
}

static bool j2k_compare_qcc(grk_j2k *p_j2k, uint32_t first_comp_no,
		uint32_t second_comp_no) {
	return j2k_compare_SQcd_SQcc(p_j2k, p_j2k->m_current_tile_number,
			first_comp_no, second_comp_no);
}

static bool j2k_write_qcc_in_memory(grk_j2k *p_j2k, uint32_t comp_no,
		BufferedStream *p_stream) {
	assert(p_j2k != nullptr);
	uint32_t l_qcc_size = 6
			+ j2k_get_SQcd_SQcc_size(p_j2k, p_j2k->m_current_tile_number,
					comp_no);

	/* QCC */
	if (!p_stream->write_short(J2K_MS_QCC)) {
		return false;
	}

	if (p_j2k->m_private_image->numcomps <= 256) {
		--l_qcc_size;

		/* L_QCC */
		if (!p_stream->write_short((uint16_t) (l_qcc_size - 2))) {
			return false;
		}

		/* Cqcc */
		if (!p_stream->write_byte((uint8_t) comp_no)) {
			return false;
		}

	} else {
		/* L_QCC */
		if (!p_stream->write_short((uint16_t) (l_qcc_size - 2))) {
			return false;
		}

		/* Cqcc */
		if (!p_stream->write_short((uint16_t) comp_no)) {
			return false;
		}
	}

	return j2k_write_SQcd_SQcc(p_j2k, p_j2k->m_current_tile_number, comp_no,
			p_stream);
}

static uint32_t j2k_get_max_qcc_size(grk_j2k *p_j2k) {
	return j2k_get_max_coc_size(p_j2k);
}

/**
 * Reads a QCC marker (Quantization component)
 * @param       p_j2k           the jpeg2000 codec.
 * @param       p_header_data   the data contained in the QCC box.
 * @param       header_size   the size of the data contained in the QCC marker.

 */
static bool j2k_read_qcc(grk_j2k *p_j2k, uint8_t *p_header_data,
		uint16_t header_size) {
	uint32_t l_num_comp, l_comp_no;

	assert(p_header_data != nullptr);
	assert(p_j2k != nullptr);
	
	l_num_comp = p_j2k->m_private_image->numcomps;

	if (l_num_comp <= 256) {
		if (header_size < 1) {
			GROK_ERROR( "Error reading QCC marker");
			return false;
		}
		grk_read_bytes(p_header_data, &l_comp_no, 1);
		++p_header_data;
		--header_size;
	} else {
		if (header_size < 2) {
			GROK_ERROR( "Error reading QCC marker");
			return false;
		}
		grk_read_bytes(p_header_data, &l_comp_no, 2);
		p_header_data += 2;
		header_size = (uint16_t)(header_size - 2);
	}

	if (l_comp_no >= p_j2k->m_private_image->numcomps) {
		GROK_ERROR(
				"Invalid component number: %d, regarding the number of components %d\n",
				l_comp_no, p_j2k->m_private_image->numcomps);
		return false;
	}

	if (!j2k_read_SQcd_SQcc(true, p_j2k, l_comp_no, p_header_data,
			&header_size)) {
		GROK_ERROR( "Error reading QCC marker");
		return false;
	}

	if (header_size != 0) {
		GROK_ERROR( "Error reading QCC marker");
		return false;
	}

	return true;
}

static uint16_t getPocSize(uint32_t l_nb_comp, uint32_t l_nb_poc) {
	uint32_t l_poc_room;

	if (l_nb_comp <= 256) {
		l_poc_room = 1;
	} else {
		l_poc_room = 2;
	}
	return (uint16_t) (4 + (5 + 2 * l_poc_room) * l_nb_poc);
}

static bool j2k_write_poc(grk_j2k *p_j2k, BufferedStream *p_stream) {
	assert(p_j2k != nullptr);
	assert(p_stream != nullptr);

	uint64_t data_written = 0;
	return j2k_write_poc_in_memory(p_j2k, p_stream, &data_written);
}

static bool j2k_write_poc_in_memory(grk_j2k *p_j2k, BufferedStream *p_stream,
		uint64_t *p_data_written) {
	
	uint32_t i;
	uint32_t l_nb_comp;
	uint32_t l_nb_poc;
	uint32_t l_poc_room;

	assert(p_j2k != nullptr);
	

	auto l_tcp = &p_j2k->m_cp.tcps[p_j2k->m_current_tile_number];
	auto l_tccp = &l_tcp->tccps[0];
	auto l_image = p_j2k->m_private_image;
	l_nb_comp = l_image->numcomps;
	l_nb_poc = l_tcp->numpocs + 1;
	if (l_nb_comp <= 256) {
		l_poc_room = 1;
	} else {
		l_poc_room = 2;
	}

	auto l_poc_size = getPocSize(l_nb_comp, 1 + l_tcp->numpocs);
	/* POC  */
	if (!p_stream->write_short(J2K_MS_POC)) {
		return false;
	}

	/* Lpoc */
	if (!p_stream->write_short((uint16_t) (l_poc_size - 2))) {
		return false;
	}
	auto l_current_poc = l_tcp->pocs;
	for (i = 0; i < l_nb_poc; ++i) {
		/* RSpoc_i */
		if (!p_stream->write_byte((uint8_t) l_current_poc->resno0)) {
			return false;
		}

		/* CSpoc_i */
		if (!p_stream->write_byte((uint8_t) l_current_poc->compno0)) {
			return false;
		}

		/* LYEpoc_i */
		if (!p_stream->write_short((uint16_t) l_current_poc->layno1)) {
			return false;
		}

		/* REpoc_i */
		if (!p_stream->write_byte((uint8_t) l_current_poc->resno1)) {
			return false;
		}

		/* CEpoc_i */
		if (l_poc_room == 2) {
			if (!p_stream->write_short((uint16_t) l_current_poc->compno1)) {
				return false;
			}

		} else {
			if (!p_stream->write_byte((uint8_t) l_current_poc->compno1)) {
				return false;
			}
		}

		/* Ppoc_i */
		if (!p_stream->write_byte((uint8_t) l_current_poc->prg)) {
			return false;
		}

		/* change the value of the max layer according to the actual number of layers in the file, components and resolutions*/
		l_current_poc->layno1 = std::min<uint32_t>(l_current_poc->layno1,
				l_tcp->numlayers);
		l_current_poc->resno1 = std::min<uint32_t>(l_current_poc->resno1,
				l_tccp->numresolutions);
		l_current_poc->compno1 = std::min<uint32_t>(l_current_poc->compno1,
				l_nb_comp);

		++l_current_poc;
	}
	*p_data_written = l_poc_size;
	return true;
}

static uint32_t j2k_get_max_poc_size(grk_j2k *p_j2k) {
	uint32_t l_nb_tiles = 0;
	uint32_t l_max_poc = 0;
	uint32_t i;

	auto l_tcp = p_j2k->m_cp.tcps;
	l_nb_tiles = p_j2k->m_cp.th * p_j2k->m_cp.tw;

	for (i = 0; i < l_nb_tiles; ++i) {
		l_max_poc = std::max<uint32_t>(l_max_poc, l_tcp->numpocs);
		++l_tcp;
	}

	++l_max_poc;

	return 4 + 9 * l_max_poc;
}

static uint32_t j2k_get_max_toc_size(grk_j2k *p_j2k) {
	uint32_t i;
	uint32_t l_nb_tiles;
	uint32_t l_max = 0;

	auto l_tcp = p_j2k->m_cp.tcps;
	l_nb_tiles = p_j2k->m_cp.tw * p_j2k->m_cp.th;

	for (i = 0; i < l_nb_tiles; ++i) {
		l_max = std::max<uint32_t>(l_max, l_tcp->m_nb_tile_parts);

		++l_tcp;
	}

	return 12 * l_max;
}

static uint64_t j2k_get_specific_header_sizes(grk_j2k *p_j2k) {
	uint64_t l_nb_bytes = 0;
	uint32_t l_nb_comps;
	uint32_t l_coc_bytes, l_qcc_bytes;

	l_nb_comps = p_j2k->m_private_image->numcomps - 1;
	l_nb_bytes += j2k_get_max_toc_size(p_j2k);

	if (!(GRK_IS_CINEMA(p_j2k->m_cp.rsiz))) {
		l_coc_bytes = j2k_get_max_coc_size(p_j2k);
		l_nb_bytes += l_nb_comps * l_coc_bytes;

		l_qcc_bytes = j2k_get_max_qcc_size(p_j2k);
		l_nb_bytes += l_nb_comps * l_qcc_bytes;
	}

	l_nb_bytes += j2k_get_max_poc_size(p_j2k);

	/*** DEVELOPER CORNER, Add room for your headers ***/

	return l_nb_bytes;
}

/**
 * Reads a POC marker (Progression Order Change)
 *
 * @param       p_header_data   the data contained in the POC box.
 * @param       p_j2k                   the jpeg2000 codec.
 * @param       header_size   the size of the data contained in the POC marker.

 */
static bool j2k_read_poc(grk_j2k *p_j2k, uint8_t *p_header_data,
		uint16_t header_size) {
	uint32_t i, l_nb_comp, l_tmp;
	uint32_t l_old_poc_nb, l_current_poc_nb, l_current_poc_remaining;
	uint32_t l_chunk_size, l_comp_room;

	assert(p_header_data != nullptr);
	assert(p_j2k != nullptr);
	
	auto l_image = p_j2k->m_private_image;
	l_nb_comp = l_image->numcomps;
	if (l_nb_comp <= 256) {
		l_comp_room = 1;
	} else {
		l_comp_room = 2;
	}
	l_chunk_size = 5 + 2 * l_comp_room;
	l_current_poc_nb = header_size / l_chunk_size;
	l_current_poc_remaining = header_size % l_chunk_size;

	if ((l_current_poc_nb <= 0) || (l_current_poc_remaining != 0)) {
		GROK_ERROR( "Error reading POC marker");
		return false;
	}

	auto l_tcp = p_j2k->get_current_decode_tcp();
	l_old_poc_nb = l_tcp->POC ? l_tcp->numpocs + 1 : 0;
	l_current_poc_nb += l_old_poc_nb;

	if (l_current_poc_nb >= 32) {
		GROK_ERROR( "Too many POCs %d", l_current_poc_nb);
		return false;
	}
	assert(l_current_poc_nb < 32);

	/* now poc is in use.*/
	l_tcp->POC = 1;

	auto l_current_poc = &l_tcp->pocs[l_old_poc_nb];
	for (i = l_old_poc_nb; i < l_current_poc_nb; ++i) {
		/* RSpoc_i */
		grk_read_bytes(p_header_data, &(l_current_poc->resno0), 1);
		++p_header_data;
		/* CSpoc_i */
		grk_read_bytes(p_header_data, &(l_current_poc->compno0), l_comp_room);
		p_header_data += l_comp_room;
		/* LYEpoc_i */
		grk_read_bytes(p_header_data, &(l_current_poc->layno1), 2);
		/* make sure layer end is in acceptable bounds */
		l_current_poc->layno1 = std::min<uint32_t>(l_current_poc->layno1,
				l_tcp->numlayers);
		p_header_data += 2;
		/* REpoc_i */
		grk_read_bytes(p_header_data, &(l_current_poc->resno1), 1);
		++p_header_data;
		/* CEpoc_i */
		grk_read_bytes(p_header_data, &(l_current_poc->compno1), l_comp_room);
		p_header_data += l_comp_room;
		/* Ppoc_i */
		grk_read_bytes(p_header_data, &l_tmp, 1);
		++p_header_data;
		l_current_poc->prg = (GRK_PROG_ORDER) l_tmp;
		/* make sure comp is in acceptable bounds */
		l_current_poc->compno1 = std::min<uint32_t>(l_current_poc->compno1,
				l_nb_comp);
		++l_current_poc;
	}
	l_tcp->numpocs = l_current_poc_nb - 1;
	return true;
}

/**
 * Reads a CRG marker (Component registration)
 *
 * @param       p_header_data   the data contained in the TLM box.
 * @param       p_j2k                   the jpeg2000 codec.
 * @param       header_size   the size of the data contained in the TLM marker.

 */
static bool j2k_read_crg(grk_j2k *p_j2k, uint8_t *p_header_data,
		uint16_t header_size) {
	uint32_t l_nb_comp;

	assert(p_header_data != nullptr);
	assert(p_j2k != nullptr);

	l_nb_comp = p_j2k->m_private_image->numcomps;

	if (header_size != l_nb_comp * 4) {
		GROK_ERROR( "Error reading CRG marker");
		return false;
	}
	uint32_t l_Xcrg_i, l_Ycrg_i;
	for (uint32_t i = 0; i < l_nb_comp; ++i) {
		// Xcrg_i
		grk_read_bytes(p_header_data, &l_Xcrg_i, 2);
		p_header_data += 2;
		// Xcrg_i
		grk_read_bytes(p_header_data, &l_Ycrg_i, 2);
		p_header_data += 2;
	}
	return true;
}

/**
 * Reads a TLM marker (Tile Length Marker)
 *
 * @param       p_header_data   the data contained in the TLM box.
 * @param       p_j2k                   the jpeg2000 codec.
 * @param       header_size   the size of the data contained in the TLM marker.

 */
static bool j2k_read_tlm(grk_j2k *p_j2k, uint8_t *p_header_data,
		uint16_t header_size) {
	(void) p_j2k;
	uint32_t i_TLM, L;
	uint32_t L_iT, L_LTP;

	assert(p_header_data != nullptr);
	assert(p_j2k != nullptr);
	
	if (header_size < 2) {
		GROK_ERROR( "Error reading TLM marker");
		return false;
	}
	// correct for length of marker
	header_size = (uint16_t)(header_size - 2);
	// read TLM marker segment index
	grk_read_bytes(p_header_data, &i_TLM, 1);
	++p_header_data;
	// read and parse L parameter, which indicates number of bytes used to represent
	// remaining parameters
	grk_read_bytes(p_header_data, &L, 1);
	++p_header_data;
	// 0x70 ==  1110000
	if ((L & ~0x70) != 0) {
		GROK_ERROR( "Illegal L value in TLM marker");
		return false;
	}
	L_iT = ((L >> 4) & 0x3);	// 0 <= L_iT <= 2
	L_LTP = (L >> 6) & 0x1;		// 0 <= L_iTP <= 1

	uint32_t bytes_per_tile_part_length = L_LTP ? 4U : 2U;
	uint32_t l_quotient = bytes_per_tile_part_length + L_iT;
	if (header_size % l_quotient != 0) {
		GROK_ERROR( "Error reading TLM marker");
		return false;
	}
  if (!p_j2k->m_cp.tl_marker)
      p_j2k->m_cp.tl_marker = new grk_tl_marker();

	uint8_t num_tp = (uint8_t)(header_size / l_quotient);
	uint32_t l_Ttlm_i=0, l_Ptlm_i=0;
	for (uint8_t i = 0; i < num_tp; ++i) {
		if (L_iT) {
			grk_read_bytes(p_header_data, &l_Ttlm_i, L_iT);
			p_header_data += L_iT;
		}
		grk_read_bytes(p_header_data, &l_Ptlm_i, bytes_per_tile_part_length);
    auto info = L_iT ? grk_tl_info((uint16_t)l_Ttlm_i,l_Ptlm_i ) : grk_tl_info(l_Ptlm_i);
    auto pair = p_j2k->m_cp.tl_marker->tile_part_lengths.find((uint8_t)i_TLM);
    if (pair != p_j2k->m_cp.tl_marker->tile_part_lengths.end()){
        pair->second.push_back(info);
    } else {
        auto vec = TL_INFO_VEC();
        vec.push_back(info);
        p_j2k->m_cp.tl_marker->tile_part_lengths[(uint8_t)i_TLM] = vec;

    }
		p_header_data += bytes_per_tile_part_length;
	}
	return true;
}

/**
 * Reads a PLM marker (Packet length, main header marker)
 *
 * @param       p_header_data   the data contained in the TLM box.
 * @param       p_j2k                   the jpeg2000 codec.
 * @param       header_size   the size of the data contained in the TLM marker.

 */
static bool j2k_read_plm(grk_j2k *p_j2k, uint8_t *p_header_data,
		uint16_t hdr_size) {
	(void) p_j2k;

	assert(p_header_data != nullptr);
	assert(p_j2k != nullptr);
	
	uint32_t l_Zplm, l_Nplm, l_tmp, l_packet_len = 0, i;
	if (hdr_size < 1) {
		GROK_ERROR( "Error reading PLM marker");
		return false;
	}

	int32_t header_size = hdr_size;

	// Zplm
	grk_read_bytes(p_header_data, &l_Zplm, 1);
	++p_header_data;
	--header_size;

	while (header_size > 0) {
		// Nplm
		grk_read_bytes(p_header_data, &l_Nplm, 1);
		++p_header_data;
		header_size -= (1 + l_Nplm);
		if (header_size < 0) {
			GROK_ERROR( "Error reading PLM marker");
			return false;
		}
		for (i = 0; i < l_Nplm; ++i) {
			// Iplm_ij
			grk_read_bytes(p_header_data, &l_tmp, 1);
			++p_header_data;
			/* take only the lower seven bits */
			l_packet_len |= (l_tmp & 0x7f);
			if (l_tmp & 0x80) {
				l_packet_len <<= 7;
			} else {
				// store packet length and proceed to next packet
				l_packet_len = 0;
			}
		}
		if (l_packet_len != 0) {
			GROK_ERROR( "Error reading PLM marker");
			return false;
		}
	}
	return true;
}

/**
 * Reads a PLT marker (Packet length, tile-part header)
 *
 * @param       p_header_data   the data contained in the PLT box.
 * @param       p_j2k           the jpeg2000 codec.
 * @param       header_size   the size of the data contained in the PLT marker.

 */
static bool j2k_read_plt(grk_j2k *p_j2k, uint8_t *p_header_data,
		uint16_t header_size) {
	(void) p_j2k;
	assert(p_header_data != nullptr);
	assert(p_j2k != nullptr);
	

	if (header_size < 1) {
		GROK_ERROR( "Error reading PLT marker");
		return false;
	}

	/* Zplt */
	uint32_t l_Zplt;
	grk_read_bytes(p_header_data, &l_Zplt, 1);
	++p_header_data;
	--header_size;

	uint32_t l_tmp;
	uint32_t l_packet_len = 0;
	for (uint32_t i = 0; i < header_size; ++i) {
		/* Iplt_ij */
		grk_read_bytes(p_header_data, &l_tmp, 1);
		++p_header_data;
		/* take only the lower seven bits */
		l_packet_len |= (l_tmp & 0x7f);
		if (l_tmp & 0x80) {
			l_packet_len <<= 7;
		} else {
			/* store packet length and proceed to next packet */
			l_packet_len = 0;
		}
	}
	if (l_packet_len != 0) {
		GROK_ERROR( "Error reading PLT marker");
		return false;
	}
	return true;
}

/**
 * Reads a PPM marker (Packed packet headers, main header)
 *
 * @param       p_header_data   the data contained in the POC box.
 * @param       p_j2k                   the jpeg2000 codec.
 * @param       header_size   the size of the data contained in the POC marker.

 */

static bool j2k_read_ppm(grk_j2k *p_j2k, uint8_t *p_header_data,
		uint16_t header_size) {
	grk_coding_parameters *l_cp = nullptr;
	uint32_t l_Z_ppm;

	assert(p_header_data != nullptr);
	assert(p_j2k != nullptr);
	

	/* We need to have the Z_ppm element + 1 byte of Nppm/Ippm at minimum */
	if (header_size < 2) {
		GROK_ERROR( "Error reading PPM marker");
		return false;
	}

	l_cp = &(p_j2k->m_cp);
	l_cp->ppm = 1;

	/* Z_ppm */
	grk_read_bytes(p_header_data, &l_Z_ppm, 1);
	++p_header_data;
	--header_size;

	/* check allocation needed */
	if (l_cp->ppm_markers == nullptr) { /* first PPM marker */
		uint32_t l_newCount = l_Z_ppm + 1U; /* can't overflow, l_Z_ppm is UINT8 */
		assert(l_cp->ppm_markers_count == 0U);

		l_cp->ppm_markers = (grk_ppx*) grk_calloc(l_newCount, sizeof(grk_ppx));
		if (l_cp->ppm_markers == nullptr) {
			GROK_ERROR(
					"Not enough memory to read PPM marker");
			return false;
		}
		l_cp->ppm_markers_count = l_newCount;
	} else if (l_cp->ppm_markers_count <= l_Z_ppm) {
		uint32_t l_newCount = l_Z_ppm + 1U; /* can't overflow, l_Z_ppm is UINT8 */
		grk_ppx *new_ppm_markers;
		new_ppm_markers = (grk_ppx*) grk_realloc(l_cp->ppm_markers,
				l_newCount * sizeof(grk_ppx));
		if (new_ppm_markers == nullptr) {
			/* clean up to be done on l_cp destruction */
			GROK_ERROR(
					"Not enough memory to read PPM marker");
			return false;
		}
		l_cp->ppm_markers = new_ppm_markers;
		memset(l_cp->ppm_markers + l_cp->ppm_markers_count, 0,
				(l_newCount - l_cp->ppm_markers_count) * sizeof(grk_ppx));
		l_cp->ppm_markers_count = l_newCount;
	}

	if (l_cp->ppm_markers[l_Z_ppm].m_data != nullptr) {
		/* clean up to be done on l_cp destruction */
		GROK_ERROR( "Zppm %u already read", l_Z_ppm);
		return false;
	}

	l_cp->ppm_markers[l_Z_ppm].m_data = (uint8_t*) grk_malloc(header_size);
	if (l_cp->ppm_markers[l_Z_ppm].m_data == nullptr) {
		/* clean up to be done on l_cp destruction */
		GROK_ERROR(
				"Not enough memory to read PPM marker");
		return false;
	}
	l_cp->ppm_markers[l_Z_ppm].m_data_size = header_size;
	memcpy(l_cp->ppm_markers[l_Z_ppm].m_data, p_header_data, header_size);

	return true;
}

/**
 * Merges all PPM markers read (Packed headers, main header)
 *
 * @param       p_cp      main coding parameters.
 
 */
static bool j2k_merge_ppm(grk_coding_parameters *p_cp) {
	uint32_t i, l_ppm_data_size, l_N_ppm_remaining;

	assert(p_cp != nullptr);
	assert(p_cp->ppm_buffer == nullptr);

	if (p_cp->ppm == 0U) {
		return true;
	}

	l_ppm_data_size = 0U;
	l_N_ppm_remaining = 0U;
	for (i = 0U; i < p_cp->ppm_markers_count; ++i) {
		if (p_cp->ppm_markers[i].m_data != nullptr) { /* standard doesn't seem to require contiguous Zppm */
			uint32_t l_N_ppm;
			uint32_t l_data_size = p_cp->ppm_markers[i].m_data_size;
			const uint8_t *l_data = p_cp->ppm_markers[i].m_data;

			if (l_N_ppm_remaining >= l_data_size) {
				l_N_ppm_remaining -= l_data_size;
				l_data_size = 0U;
			} else {
				l_data += l_N_ppm_remaining;
				l_data_size -= l_N_ppm_remaining;
				l_N_ppm_remaining = 0U;
			}

			if (l_data_size > 0U) {
				do {
					/* read Nppm */
					if (l_data_size < 4U) {
						/* clean up to be done on l_cp destruction */
						GROK_ERROR(
								"Not enough bytes to read Nppm");
						return false;
					}
					grk_read_bytes(l_data, &l_N_ppm, 4);
					l_data += 4;
					l_data_size -= 4;
					l_ppm_data_size += l_N_ppm; /* can't overflow, max 256 markers of max 65536 bytes, that is when PPM markers are not corrupted which is checked elsewhere */

					if (l_data_size >= l_N_ppm) {
						l_data_size -= l_N_ppm;
						l_data += l_N_ppm;
					} else {
						l_N_ppm_remaining = l_N_ppm - l_data_size;
						l_data_size = 0U;
					}
				} while (l_data_size > 0U);
			}
		}
	}

	if (l_N_ppm_remaining != 0U) {
		/* clean up to be done on l_cp destruction */
		GROK_ERROR( "Corrupted PPM markers");
		return false;
	}

	p_cp->ppm_buffer = (uint8_t*) grk_malloc(l_ppm_data_size);
	if (p_cp->ppm_buffer == nullptr) {
		GROK_ERROR(
				"Not enough memory to read PPM marker");
		return false;
	}
	p_cp->ppm_len = l_ppm_data_size;
	l_ppm_data_size = 0U;
	l_N_ppm_remaining = 0U;
	for (i = 0U; i < p_cp->ppm_markers_count; ++i) {
		if (p_cp->ppm_markers[i].m_data != nullptr) { /* standard doesn't seem to require contiguous Zppm */
			uint32_t l_N_ppm;
			uint32_t l_data_size = p_cp->ppm_markers[i].m_data_size;
			const uint8_t *l_data = p_cp->ppm_markers[i].m_data;

			if (l_N_ppm_remaining >= l_data_size) {
				memcpy(p_cp->ppm_buffer + l_ppm_data_size, l_data, l_data_size);
				l_ppm_data_size += l_data_size;
				l_N_ppm_remaining -= l_data_size;
				l_data_size = 0U;
			} else {
				memcpy(p_cp->ppm_buffer + l_ppm_data_size, l_data,
						l_N_ppm_remaining);
				l_ppm_data_size += l_N_ppm_remaining;
				l_data += l_N_ppm_remaining;
				l_data_size -= l_N_ppm_remaining;
				l_N_ppm_remaining = 0U;
			}

			if (l_data_size > 0U) {
				do {
					/* read Nppm */
					if (l_data_size < 4U) {
						/* clean up to be done on l_cp destruction */
						GROK_ERROR(
								"Not enough bytes to read Nppm");
						return false;
					}
					grk_read_bytes(l_data, &l_N_ppm, 4);
					l_data += 4;
					l_data_size -= 4;

					if (l_data_size >= l_N_ppm) {
						memcpy(p_cp->ppm_buffer + l_ppm_data_size, l_data,
								l_N_ppm);
						l_ppm_data_size += l_N_ppm;
						l_data_size -= l_N_ppm;
						l_data += l_N_ppm;
					} else {
						memcpy(p_cp->ppm_buffer + l_ppm_data_size, l_data,
								l_data_size);
						l_ppm_data_size += l_data_size;
						l_N_ppm_remaining = l_N_ppm - l_data_size;
						l_data_size = 0U;
					}
				} while (l_data_size > 0U);
			}
			grok_free(p_cp->ppm_markers[i].m_data);
			p_cp->ppm_markers[i].m_data = nullptr;
			p_cp->ppm_markers[i].m_data_size = 0U;
		}
	}

	p_cp->ppm_data = p_cp->ppm_buffer;
	p_cp->ppm_data_size = p_cp->ppm_len;

	p_cp->ppm_markers_count = 0U;
	grok_free(p_cp->ppm_markers);
	p_cp->ppm_markers = nullptr;

	return true;
}

/**
 * Reads a PPT marker (Packed packet headers, tile-part header)
 *
 * @param       p_header_data   the data contained in the PPT box.
 * @param       p_j2k                   the jpeg2000 codec.
 * @param       header_size   the size of the data contained in the PPT marker.

 */
static bool j2k_read_ppt(grk_j2k *p_j2k, uint8_t *p_header_data,
		uint16_t header_size) {
	uint32_t l_Z_ppt;

	assert(p_header_data != nullptr);
	assert(p_j2k != nullptr);
	

	/* We need to have the Z_ppt element + 1 byte of Ippt at minimum */
	if (header_size < 2) {
		GROK_ERROR( "Error reading PPT marker");
		return false;
	}

	auto l_cp = &(p_j2k->m_cp);
	if (l_cp->ppm) {
		GROK_ERROR(
				"Error reading PPT marker: packet header have been previously found in the main header (PPM marker).");
		return false;
	}

	auto l_tcp = &(l_cp->tcps[p_j2k->m_current_tile_number]);
	l_tcp->ppt = 1;

	/* Z_ppt */
	grk_read_bytes(p_header_data, &l_Z_ppt, 1);
	++p_header_data;
	--header_size;

	/* check allocation needed */
	if (l_tcp->ppt_markers == nullptr) { /* first PPT marker */
		uint32_t l_newCount = l_Z_ppt + 1U; /* can't overflow, l_Z_ppt is UINT8 */
		assert(l_tcp->ppt_markers_count == 0U);

		l_tcp->ppt_markers = (grk_ppx*) grk_calloc(l_newCount, sizeof(grk_ppx));
		if (l_tcp->ppt_markers == nullptr) {
			GROK_ERROR(
					"Not enough memory to read PPT marker");
			return false;
		}
		l_tcp->ppt_markers_count = l_newCount;
	} else if (l_tcp->ppt_markers_count <= l_Z_ppt) {
		uint32_t l_newCount = l_Z_ppt + 1U; /* can't overflow, l_Z_ppt is UINT8 */
		grk_ppx *new_ppt_markers;
		new_ppt_markers = (grk_ppx*) grk_realloc(l_tcp->ppt_markers,
				l_newCount * sizeof(grk_ppx));
		if (new_ppt_markers == nullptr) {
			/* clean up to be done on l_tcp destruction */
			GROK_ERROR(
					"Not enough memory to read PPT marker");
			return false;
		}
		l_tcp->ppt_markers = new_ppt_markers;
		memset(l_tcp->ppt_markers + l_tcp->ppt_markers_count, 0,
				(l_newCount - l_tcp->ppt_markers_count) * sizeof(grk_ppx));
		l_tcp->ppt_markers_count = l_newCount;
	}

	if (l_tcp->ppt_markers[l_Z_ppt].m_data != nullptr) {
		/* clean up to be done on l_tcp destruction */
		GROK_ERROR( "Zppt %u already read", l_Z_ppt);
		return false;
	}

	l_tcp->ppt_markers[l_Z_ppt].m_data = (uint8_t*) grk_malloc(header_size);
	if (l_tcp->ppt_markers[l_Z_ppt].m_data == nullptr) {
		/* clean up to be done on l_tcp destruction */
		GROK_ERROR(
				"Not enough memory to read PPT marker");
		return false;
	}
	l_tcp->ppt_markers[l_Z_ppt].m_data_size = header_size;
	memcpy(l_tcp->ppt_markers[l_Z_ppt].m_data, p_header_data, header_size);
	return true;
}

/**
 * Merges all PPT markers read (Packed packet headers, tile-part header)
 *
 * @param       p_tcp   the tile.

 */
static bool j2k_merge_ppt(grk_tcp *p_tcp) {
	uint32_t i, l_ppt_data_size;

	assert(p_tcp != nullptr);
	assert(p_tcp->ppt_buffer == nullptr);

	if (p_tcp->ppt == 0U) {
		return true;
	}
	if (p_tcp->ppt_buffer != nullptr) {
		GROK_ERROR( "multiple calls to j2k_merge_ppt()");
		return false;
	}

	l_ppt_data_size = 0U;
	for (i = 0U; i < p_tcp->ppt_markers_count; ++i) {
		l_ppt_data_size += p_tcp->ppt_markers[i].m_data_size; /* can't overflow, max 256 markers of max 65536 bytes */
	}

	p_tcp->ppt_buffer = (uint8_t*) grk_malloc(l_ppt_data_size);
	if (p_tcp->ppt_buffer == nullptr) {
		GROK_ERROR(
				"Not enough memory to read PPT marker");
		return false;
	}
	p_tcp->ppt_len = l_ppt_data_size;
	l_ppt_data_size = 0U;
	for (i = 0U; i < p_tcp->ppt_markers_count; ++i) {
		if (p_tcp->ppt_markers[i].m_data != nullptr) { /* standard doesn't seem to require contiguous Zppt */
			memcpy(p_tcp->ppt_buffer + l_ppt_data_size,
					p_tcp->ppt_markers[i].m_data,
					p_tcp->ppt_markers[i].m_data_size);
			l_ppt_data_size += p_tcp->ppt_markers[i].m_data_size; /* can't overflow, max 256 markers of max 65536 bytes */

			grok_free(p_tcp->ppt_markers[i].m_data);
			p_tcp->ppt_markers[i].m_data = nullptr;
			p_tcp->ppt_markers[i].m_data_size = 0U;
		}
	}

	p_tcp->ppt_markers_count = 0U;
	grok_free(p_tcp->ppt_markers);
	p_tcp->ppt_markers = nullptr;

	p_tcp->ppt_data = p_tcp->ppt_buffer;
	p_tcp->ppt_data_size = p_tcp->ppt_len;
	return true;
}

static bool j2k_write_tlm(grk_j2k *p_j2k, BufferedStream *p_stream) {
	assert(p_j2k != nullptr);
	assert(p_stream != nullptr);

	uint32_t l_tlm_size =
			6 + (5 * p_j2k->m_specific_param.m_encoder.m_total_tile_parts);

	/* change the way data is written to avoid seeking if possible */
	/* TODO */
	p_j2k->m_specific_param.m_encoder.m_tlm_start = p_stream->tell();

	/* TLM */
	if (!p_stream->write_short(J2K_MS_TLM)) {
		return false;
	}

	/* Lpoc */
	if (!p_stream->write_short((uint16_t) (l_tlm_size - 2))) {
		return false;
	}

	/* Ztlm=0*/
	if (!p_stream->write_byte(0)) {
		return false;
	}

	/* Stlm ST=1(8bits-255 tiles max),SP=1(Ptlm=32bits) */
	if (!p_stream->write_byte(0x50)) {
		return false;
	}
	/* do nothing on the 5 * l_j2k->m_specific_param.m_encoder.m_total_tile_parts remaining data */
	if (!p_stream->skip(
			(5 * p_j2k->m_specific_param.m_encoder.m_total_tile_parts))) {
		return false;
	}
	return true;
}

static bool j2k_write_sot(grk_j2k *p_j2k, BufferedStream *p_stream,
		uint64_t *psot_location, uint64_t *p_data_written) {
	assert(p_j2k != nullptr);

	/* SOT */
	if (!p_stream->write_short(J2K_MS_SOT)) {
		return false;
	}

	/* Lsot */
	if (!p_stream->write_short(10)) {
		return false;
	}

	/* Isot */
	if (!p_stream->write_short((uint16_t) p_j2k->m_current_tile_number)) {
		return false;
	}

	/* Psot  */
	*psot_location = p_stream->tell();
	if (!p_stream->skip(4)) {
		return false;
	}

	/* TPsot */
	if (!p_stream->write_byte(
			p_j2k->m_specific_param.m_encoder.m_current_tile_part_number)) {
		return false;
	}

	/* TNsot */
	if (!p_stream->write_byte(
			p_j2k->m_cp.tcps[p_j2k->m_current_tile_number].m_nb_tile_parts)) {
		return false;
	}

	*p_data_written += 12;
	return true;

}

static bool j2k_get_sot_values(uint8_t *p_header_data, uint32_t header_size,
		uint16_t *tile_no, uint32_t *p_tot_len, uint8_t *p_current_part,
		uint8_t *p_num_parts) {

	assert(p_header_data != nullptr);
	
	/* Size of this marker is fixed = 12 (we have already read marker and its size)*/
	if (header_size != 8) {
		GROK_ERROR( "Error reading SOT marker");
		return false;
	}
	/* Isot */
	uint32_t temp;
	grk_read_bytes(p_header_data, &temp, 2);
	p_header_data += 2;
	*tile_no = (uint16_t)temp;
	/* Psot */
	grk_read_bytes(p_header_data, p_tot_len, 4);
	p_header_data += 4;
	/* TPsot */
	grk_read_bytes(p_header_data,&temp , 1);
	*p_current_part = (uint8_t)temp;
	++p_header_data;
	/* TNsot */
	grk_read_bytes(p_header_data, &temp, 1);
  *p_num_parts = (uint8_t)temp;
	++p_header_data;
	return true;
}

static bool j2k_read_sot(grk_j2k *p_j2k, uint8_t *p_header_data,
		uint16_t header_size) {
	uint32_t l_tot_len = 0;
	uint8_t l_num_parts = 0;
	uint8_t l_current_part;
	uint32_t l_tile_x, l_tile_y;

	assert(p_j2k != nullptr);

	if (!j2k_get_sot_values(p_header_data, header_size,
			&p_j2k->m_current_tile_number, &l_tot_len, &l_current_part,
			&l_num_parts)) {
		GROK_ERROR( "Error reading SOT marker");
		return false;
	}
	auto tile_number = p_j2k->m_current_tile_number;

	auto l_cp = &(p_j2k->m_cp);

	/* testcase 2.pdf.SIGFPE.706.1112 */
	if (tile_number >= l_cp->tw * l_cp->th) {
		GROK_ERROR( "Invalid tile number %d\n",
				tile_number);
		return false;
	}

	auto l_tcp = &l_cp->tcps[tile_number];
	l_tile_x = tile_number % l_cp->tw;
	l_tile_y = tile_number / l_cp->tw;

	/* Fixes issue with id_000020,sig_06,src_001958,op_flip4,pos_149 */
	/* of https://github.com/uclouvain/openjpeg/issues/939 */
	/* We must avoid reading the same tile part number twice for a given tile */
	/* to avoid various issues, like grk_j2k_merge_ppt being called several times. */
	/* ISO 15444-1 A.4.2 Start of tile-part (SOT) mandates that tile parts */
	/* should appear in increasing order. */
	if (l_tcp->m_current_tile_part_number + 1 != (int32_t) l_current_part) {
		GROK_ERROR(
				"Invalid tile part index for tile number %d. "
						"Got %d, expected %d\n", tile_number,
				l_current_part, l_tcp->m_current_tile_part_number + 1);
		return false;
	}
	++l_tcp->m_current_tile_part_number;
	/* look for the tile in the list of already processed tile (in parts). */
	/* Optimization possible here with a more complex data structure and with the removing of tiles */
	/* since the time taken by this function can only grow at the time */

	/* PSot should be equal to zero or >=14 or <= 2^32-1 */
	if ((l_tot_len != 0) && (l_tot_len < 14)) {
		if (l_tot_len == 12) { /* special case for the PHR data which are read by kakadu*/
			GROK_WARN(
					"Empty SOT marker detected: Psot=%d.", l_tot_len);
		} else {
			GROK_ERROR(
					"Psot value is not correct regards to the JPEG2000 norm: %d.\n",
					l_tot_len);
			return false;
		}
	}

	/* Ref A.4.2: Psot may equal zero if it is the last tile-part of the codestream.*/
	if (!l_tot_len) {
		//GROK_WARN( "Psot value of the current tile-part is equal to zero; "
		//              "we assume it is the last tile-part of the codestream.");
		p_j2k->m_specific_param.m_decoder.m_last_tile_part = 1;
	}

	// ensure that current tile part number read from SOT marker
	// is not larger than total number of tile parts
	if (l_tcp->m_nb_tile_parts != 0
			&& l_current_part >= l_tcp->m_nb_tile_parts) {
		/* Fixes https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=2851 */
		GROK_ERROR(
				"Current tile part number (%d) read from SOT marker is greater than total "
						"number of tile-parts (%d).\n", l_current_part,
				l_tcp->m_nb_tile_parts);
		p_j2k->m_specific_param.m_decoder.m_last_tile_part = 1;
		return false;
	}

	if (l_num_parts != 0) { /* Number of tile-part header is provided by this tile-part header */
		l_num_parts = (uint8_t)(l_num_parts +
				p_j2k->m_specific_param.m_decoder.m_nb_tile_parts_correction);
		/* Useful to manage the case of textGBR.jp2 file because two values of TNSot are allowed: the correct numbers of
		 * tile-parts for that tile and zero (A.4.2 of 15444-1 : 2002). */
		if (l_tcp->m_nb_tile_parts) {
			if (l_current_part >= l_tcp->m_nb_tile_parts) {
				GROK_ERROR(
						"In SOT marker, TPSot (%d) is not valid regards to the current "
								"number of tile-part (%d), giving up\n",
						l_current_part, l_tcp->m_nb_tile_parts);
				p_j2k->m_specific_param.m_decoder.m_last_tile_part = 1;
				return false;
			}
		}
		if (l_current_part >= l_num_parts) {
			/* testcase 451.pdf.SIGSEGV.ce9.3723 */
			GROK_ERROR(
					"In SOT marker, TPSot (%d) is not valid regards to the current "
							"number of tile-part (header) (%d), giving up\n",
					l_current_part, l_num_parts);
			p_j2k->m_specific_param.m_decoder.m_last_tile_part = 1;
			return false;
		}
		l_tcp->m_nb_tile_parts = l_num_parts;
	}

	/* If know the number of tile part header we will check if we didn't read the last*/
	if (l_tcp->m_nb_tile_parts) {
		if (l_tcp->m_nb_tile_parts == (l_current_part + 1)) {
			p_j2k->m_specific_param.m_decoder.ready_to_decode_tile_part_data =
					1; /* Process the last tile-part header*/
		}
	}

	if (!p_j2k->m_specific_param.m_decoder.m_last_tile_part) {
		/* Keep the size of data to skip after this marker */
		p_j2k->m_specific_param.m_decoder.tile_part_data_length = l_tot_len
				- 12; /* SOT marker size = 12 */
	} else {
		/* FIXME: need to be computed from the number of bytes remaining in the codestream */
		p_j2k->m_specific_param.m_decoder.tile_part_data_length = 0;
	}

	p_j2k->m_specific_param.m_decoder.m_state = J2K_DEC_STATE_TPH;

	/* Check if the current tile is outside the area we want decode or not corresponding to the tile index*/
	if (p_j2k->m_specific_param.m_decoder.m_tile_ind_to_dec == -1) {
		p_j2k->m_specific_param.m_decoder.m_skip_data = (l_tile_x
				< p_j2k->m_specific_param.m_decoder.m_start_tile_x)
				|| (l_tile_x >= p_j2k->m_specific_param.m_decoder.m_end_tile_x)
				|| (l_tile_y < p_j2k->m_specific_param.m_decoder.m_start_tile_y)
				|| (l_tile_y >= p_j2k->m_specific_param.m_decoder.m_end_tile_y);
	} else {
		assert(p_j2k->m_specific_param.m_decoder.m_tile_ind_to_dec >= 0);
		p_j2k->m_specific_param.m_decoder.m_skip_data =
				(tile_number
						!= (uint32_t) p_j2k->m_specific_param.m_decoder.m_tile_ind_to_dec);
	}

	/* Index */
	if (p_j2k->cstr_index) {
		assert(p_j2k->cstr_index->tile_index != nullptr);
		p_j2k->cstr_index->tile_index[tile_number].tileno =
				tile_number;
		p_j2k->cstr_index->tile_index[tile_number].current_tpsno =
				l_current_part;

		if (l_num_parts != 0) {
			p_j2k->cstr_index->tile_index[tile_number].nb_tps =
					l_num_parts;
			p_j2k->cstr_index->tile_index[tile_number].current_nb_tps =
					l_num_parts;

			if (!p_j2k->cstr_index->tile_index[tile_number].tp_index) {
				p_j2k->cstr_index->tile_index[tile_number].tp_index =
						( grk_tp_index  * ) grk_calloc(l_num_parts,
								sizeof( grk_tp_index) );
				if (!p_j2k->cstr_index->tile_index[tile_number].tp_index) {
					GROK_ERROR(
							"Not enough memory to read SOT marker. Tile index allocation failed");
					return false;
				}
			} else {
				 grk_tp_index  *new_tp_index =
						( grk_tp_index  * ) grk_realloc(
								p_j2k->cstr_index->tile_index[tile_number].tp_index,
								l_num_parts * sizeof( grk_tp_index) );
				if (!new_tp_index) {
					grok_free(
							p_j2k->cstr_index->tile_index[tile_number].tp_index);
					p_j2k->cstr_index->tile_index[tile_number].tp_index =
							nullptr;
					GROK_ERROR(
							"Not enough memory to read SOT marker. Tile index allocation failed");
					return false;
				}
				p_j2k->cstr_index->tile_index[tile_number].tp_index =
						new_tp_index;
			}
		} else {
			/*if (!p_j2k->cstr_index->tile_index[tile_number].tp_index)*/{

				if (!p_j2k->cstr_index->tile_index[tile_number].tp_index) {
					p_j2k->cstr_index->tile_index[tile_number].current_nb_tps =
							10;
					p_j2k->cstr_index->tile_index[tile_number].tp_index =
							( grk_tp_index  * ) grk_calloc(
									p_j2k->cstr_index->tile_index[tile_number].current_nb_tps,
									sizeof( grk_tp_index) );
					if (!p_j2k->cstr_index->tile_index[tile_number].tp_index) {
						p_j2k->cstr_index->tile_index[tile_number].current_nb_tps =
								0;
						GROK_ERROR(
								"Not enough memory to read SOT marker. Tile index allocation failed");
						return false;
					}
				}

				if (l_current_part
						>= p_j2k->cstr_index->tile_index[tile_number].current_nb_tps) {
					 grk_tp_index  *new_tp_index;
					p_j2k->cstr_index->tile_index[tile_number].current_nb_tps =
							l_current_part + 1;
					new_tp_index =
							( grk_tp_index  * ) grk_realloc(
									p_j2k->cstr_index->tile_index[tile_number].tp_index,
									p_j2k->cstr_index->tile_index[tile_number].current_nb_tps
											* sizeof( grk_tp_index) );
					if (!new_tp_index) {
						grok_free(
								p_j2k->cstr_index->tile_index[tile_number].tp_index);
						p_j2k->cstr_index->tile_index[tile_number].tp_index =
								nullptr;
						p_j2k->cstr_index->tile_index[tile_number].current_nb_tps =
								0;
						GROK_ERROR(
								"Not enough memory to read SOT marker. Tile index allocation failed");
						return false;
					}
					p_j2k->cstr_index->tile_index[tile_number].tp_index =
							new_tp_index;
				}
			}

		}

	}
	return true;
}

static bool j2k_write_sod(grk_j2k *p_j2k, TileProcessor *p_tile_coder,
		uint64_t *p_data_written, uint64_t total_data_size,
		BufferedStream *p_stream) {
	 grk_codestream_info  *l_cstr_info = nullptr;
	uint64_t l_remaining_data;

	assert(p_j2k != nullptr);
	assert(p_stream != nullptr);

	/* SOD */
	if (!p_stream->write_short(J2K_MS_SOD)) {
		return false;
	}

	*p_data_written = 2;

	/* make room for the EOF marker */
	l_remaining_data = total_data_size - 4;

	/* update tile coder */
	p_tile_coder->tp_num =
			p_j2k->m_specific_param.m_encoder.m_current_poc_tile_part_number;
	p_tile_coder->cur_tp_num =
			p_j2k->m_specific_param.m_encoder.m_current_tile_part_number;

	/* set packno to zero when writing the first tile part */
	if (p_j2k->m_specific_param.m_encoder.m_current_tile_part_number == 0) {
		p_tile_coder->tile->packno = 0;
		if (l_cstr_info) {
			l_cstr_info->packno = 0;
		}
	}
	if (!p_tile_coder->encode_tile(p_j2k->m_current_tile_number, p_stream,
			p_data_written, l_remaining_data, l_cstr_info)) {
		GROK_ERROR( "Cannot encode tile");
		return false;
	}

	return true;
}

static bool j2k_read_sod(grk_j2k *p_j2k, BufferedStream *p_stream) {
	assert(p_j2k != nullptr);
	assert(p_stream != nullptr);

	// note: we subtract 2 to account for SOD marker
	grk_tcp *l_tcp = p_j2k->get_current_decode_tcp();
	if (p_j2k->m_specific_param.m_decoder.m_last_tile_part) {
		p_j2k->m_specific_param.m_decoder.tile_part_data_length =
				(uint32_t) (p_stream->get_number_byte_left() - 2);
	} else {
		if (p_j2k->m_specific_param.m_decoder.tile_part_data_length >= 2)
			p_j2k->m_specific_param.m_decoder.tile_part_data_length -= 2;
	}
	if (p_j2k->m_specific_param.m_decoder.tile_part_data_length) {
		auto bytesLeftInStream = p_stream->get_number_byte_left();
		// check that there are enough bytes in stream to fill tile data
		if (p_j2k->m_specific_param.m_decoder.tile_part_data_length
				> bytesLeftInStream) {
			GROK_WARN(
					"Tile part length size %lld inconsistent with stream length %lld\n",
					p_j2k->m_specific_param.m_decoder.tile_part_data_length,
					p_stream->get_number_byte_left());

			// sanitize tile_part_data_length
			p_j2k->m_specific_param.m_decoder.tile_part_data_length =
					(uint32_t)bytesLeftInStream;
		}
	}
	/* Index */
	 grk_codestream_index  *l_cstr_index = p_j2k->cstr_index;
	if (l_cstr_index) {
		int64_t l_current_pos = p_stream->tell() - 2;

		uint32_t l_current_tile_part =
				l_cstr_index->tile_index[p_j2k->m_current_tile_number].current_tpsno;
		l_cstr_index->tile_index[p_j2k->m_current_tile_number].tp_index[l_current_tile_part].end_header =
				l_current_pos;
		l_cstr_index->tile_index[p_j2k->m_current_tile_number].tp_index[l_current_tile_part].end_pos =
				l_current_pos
						+ p_j2k->m_specific_param.m_decoder.tile_part_data_length
						+ 2;

		if (!j2k_add_tlmarker(p_j2k->m_current_tile_number, l_cstr_index,
				J2K_MS_SOD, l_current_pos,0)) {
			GROK_ERROR("Not enough memory to add tl marker");
			return false;
		}

		/*l_cstr_index->packno = 0;*/
	}
	size_t l_current_read_size = 0;
	if (p_j2k->m_specific_param.m_decoder.tile_part_data_length) {
		if (!l_tcp->m_tile_data)
			l_tcp->m_tile_data = new ChunkBuffer();

		auto len = p_j2k->m_specific_param.m_decoder.tile_part_data_length;
		uint8_t *buff = nullptr;
		auto zeroCopy = p_stream->supportsZeroCopy();
		if (!zeroCopy) {
			try {
				buff = new uint8_t[len];
			} catch (std::bad_alloc &ex) {
				GROK_ERROR(
						"Not enough memory to allocate segment");
				return false;
			}
		} else {
			buff = p_stream->getCurrentPtr();
		}
		l_current_read_size = p_stream->read(zeroCopy ? nullptr : buff, len);
		l_tcp->m_tile_data->add_chunk(buff, len, !zeroCopy);

	}
	if (l_current_read_size
			!= p_j2k->m_specific_param.m_decoder.tile_part_data_length) {
		p_j2k->m_specific_param.m_decoder.m_state = J2K_DEC_STATE_NEOC;
	} else {
		p_j2k->m_specific_param.m_decoder.m_state = J2K_DEC_STATE_TPHSOT;
	}
	return true;
}

static bool j2k_write_rgn(grk_j2k *p_j2k, uint16_t tile_no, uint32_t comp_no,
		uint32_t nb_comps, BufferedStream *p_stream) {
	uint32_t l_rgn_size;
	uint32_t l_comp_room;

	assert(p_j2k != nullptr);
	assert(p_stream != nullptr);

	auto l_cp = &(p_j2k->m_cp);
	auto l_tcp = &l_cp->tcps[tile_no];
	auto l_tccp = &l_tcp->tccps[comp_no];

	if (nb_comps <= 256) {
		l_comp_room = 1;
	} else {
		l_comp_room = 2;
	}
	l_rgn_size = 6 + l_comp_room;

	/* RGN  */
	if (!p_stream->write_short(J2K_MS_RGN)) {
		return false;
	}

	/* Lrgn */
	if (!p_stream->write_short((uint16_t) (l_rgn_size - 2))) {
		return false;
	}

	/* Crgn */
	if (l_comp_room == 2) {
		if (!p_stream->write_short((uint16_t) comp_no)) {
			return false;
		}
	} else {
		if (!p_stream->write_byte((uint8_t) comp_no)) {
			return false;
		}
	}

	/* Srgn */
	if (!p_stream->write_byte(0)) {
		return false;
	}

	/* SPrgn */
	if (!p_stream->write_byte((uint8_t) l_tccp->roishift)) {
		return false;
	}
	return true;
}

static bool j2k_write_eoc(grk_j2k *p_j2k, BufferedStream *p_stream) {
	assert(p_j2k != nullptr);
	assert(p_stream != nullptr);
	(void) p_j2k;
	if (!p_stream->write_short(J2K_MS_EOC)) {
		return false;
	}
	if (!p_stream->flush()) {
		return false;
	}
	return true;
}

/**
 * Reads a RGN marker (Region Of Interest)
 *
 * @param       p_header_data   the data contained in the POC box.
 * @param       p_j2k                   the jpeg2000 codec.
 * @param       header_size   the size of the data contained in the POC marker.

 */
static bool j2k_read_rgn(grk_j2k *p_j2k, uint8_t *p_header_data,
		uint16_t header_size) {
	uint32_t l_comp_room, l_comp_no, l_roi_sty;

	assert(p_header_data != nullptr);
	assert(p_j2k != nullptr);
	
	auto l_image = p_j2k->m_private_image;
	uint32_t l_nb_comp = l_image->numcomps;

	if (l_nb_comp <= 256) {
		l_comp_room = 1;
	} else {
		l_comp_room = 2;
	}

	if (header_size != 2 + l_comp_room) {
		GROK_ERROR( "Error reading RGN marker");
		return false;
	}

	auto l_tcp = p_j2k->get_current_decode_tcp();

	/* Crgn */
	grk_read_bytes(p_header_data, &l_comp_no, l_comp_room);
	p_header_data += l_comp_room;
	/* Srgn */
	grk_read_bytes(p_header_data, &l_roi_sty, 1);
	if (l_roi_sty != 0){
		GROK_WARN("RGN marker RS value of %d is not supported by JPEG 2000 Part 1", l_roi_sty);
	}
	++p_header_data;

	/* testcase 3635.pdf.asan.77.2930 */
	if (l_comp_no >= l_nb_comp) {
		GROK_ERROR(
				"bad component number in RGN (%d when there are only %d)\n",
				l_comp_no, l_nb_comp);
		return false;
	}

	/* SPrgn */
	grk_read_bytes(p_header_data,
			(uint32_t*) (&(l_tcp->tccps[l_comp_no].roishift)), 1);
	++p_header_data;

	return true;

}

static bool j2k_get_end_header(grk_j2k *p_j2k, BufferedStream *p_stream) {
	assert(p_j2k != nullptr);
	assert(p_stream != nullptr);

	p_j2k->cstr_index->main_head_end = p_stream->tell();

	return true;
}

static bool j2k_write_mct_data_group(grk_j2k *p_j2k, BufferedStream *p_stream) {
	uint32_t i;
	assert(p_j2k != nullptr);
	assert(p_stream != nullptr);
	if (!j2k_write_cbd(p_j2k, p_stream)) {
		return false;
	}

	auto l_tcp = &(p_j2k->m_cp.tcps[p_j2k->m_current_tile_number]);
	auto l_mct_record = l_tcp->m_mct_records;

	for (i = 0; i < l_tcp->m_nb_mct_records; ++i) {

		if (!j2k_write_mct_record(l_mct_record, p_stream)) {
			return false;
		}

		++l_mct_record;
	}

	auto l_mcc_record = l_tcp->m_mcc_records;

	for (i = 0; i < l_tcp->m_nb_mcc_records; ++i) {

		if (!j2k_write_mcc_record(l_mcc_record, p_stream)) {
			return false;
		}

		++l_mcc_record;
	}

	if (!j2k_write_mco(p_j2k, p_stream)) {
		return false;
	}

	return true;
}

static bool j2k_write_all_coc(grk_j2k *p_j2k, BufferedStream *p_stream) {
	uint32_t compno;

	assert(p_j2k != nullptr);
	assert(p_stream != nullptr);
	for (compno = 1; compno < p_j2k->m_private_image->numcomps; ++compno) {
		/* cod is first component of first tile */
		if (!j2k_compare_coc(p_j2k, 0, compno)) {
			if (!j2k_write_coc(p_j2k, compno, p_stream)) {
				return false;
			}
		}
	}

	return true;
}

static bool j2k_write_all_qcc(grk_j2k *p_j2k, BufferedStream *p_stream) {
	uint32_t compno;

	assert(p_j2k != nullptr);
	assert(p_stream != nullptr);
	for (compno = 1; compno < p_j2k->m_private_image->numcomps; ++compno) {
		/* qcd is first component of first tile */
		if (!j2k_compare_qcc(p_j2k, 0, compno)) {
			if (!j2k_write_qcc(p_j2k, compno, p_stream)) {
				return false;
			}
		}
	}
	return true;
}

static bool j2k_write_regions(grk_j2k *p_j2k, BufferedStream *p_stream) {
	uint32_t compno;
	assert(p_j2k != nullptr);
	assert(p_stream != nullptr);

	auto l_tccp = p_j2k->m_cp.tcps->tccps;
	for (compno = 0; compno < p_j2k->m_private_image->numcomps; ++compno) {
		if (l_tccp->roishift) {

			if (!j2k_write_rgn(p_j2k, 0, compno,
					p_j2k->m_private_image->numcomps, p_stream)) {
				return false;
			}
		}

		++l_tccp;
	}

	return true;
}

static bool j2k_write_epc(grk_j2k *p_j2k, BufferedStream *p_stream) {
	assert(p_j2k != nullptr);
	assert(p_stream != nullptr);

	auto l_cstr_index = p_j2k->cstr_index;
	if (l_cstr_index) {
		l_cstr_index->codestream_size = (uint64_t) p_stream->tell();
		/* The following adjustment is done to adjust the codestream size */
		/* if SOD is not at 0 in the buffer. Useful in case of JP2, where */
		/* the first bunch of bytes is not in the codestream              */
		l_cstr_index->codestream_size -=
				(uint64_t) l_cstr_index->main_head_start;

	}
	return true;
}

static bool j2k_read_unk(grk_j2k *p_j2k, BufferedStream *p_stream,
		uint32_t *output_marker) {
	uint32_t l_unknown_marker;
	const  grk_dec_memory_marker_handler  *l_marker_handler;
	uint32_t l_size_unk = 2;

	/* preconditions*/
	assert(p_j2k != nullptr);
	
	assert(p_stream != nullptr);

	GROK_WARN( "Unknown marker 0x%02x\n",
			*output_marker);

	for (;;) {
		/* Try to read 2 bytes (the next marker ID) from stream and copy them into the buffer*/
		if (p_stream->read(p_j2k->m_specific_param.m_decoder.m_marker_scratch, 2) != 2) {
			GROK_ERROR( "Stream too short");
			return false;
		}

		/* read 2 bytes as the new marker ID*/
		grk_read_bytes(p_j2k->m_specific_param.m_decoder.m_marker_scratch,
				&l_unknown_marker, 2);

		if (!(l_unknown_marker < 0xff00)) {

			/* Get the marker handler from the marker ID*/
			l_marker_handler = j2k_get_marker_handler(l_unknown_marker);

			if (!(p_j2k->m_specific_param.m_decoder.m_state
					& l_marker_handler->states)) {
				GROK_ERROR(
						"Marker is not compliant with its position");
				return false;
			} else {
				if (l_marker_handler->id != J2K_MS_UNK) {
					/* Add the marker to the codestream index*/
					if (p_j2k->cstr_index && l_marker_handler->id != J2K_MS_SOT) {
						bool res = j2k_add_mhmarker(p_j2k->cstr_index,
								J2K_MS_UNK,
								p_stream->tell() - l_size_unk,
								l_size_unk);
						if (res == false) {
							GROK_ERROR(
									"Not enough memory to add mh marker");
							return false;
						}
					}
					break; /* next marker is known and well located */
				} else
					l_size_unk += 2;
			}
		}
	}

	*output_marker = l_marker_handler->id;

	return true;
}

static bool j2k_write_mct_record(grk_mct_data *p_mct_record, BufferedStream *p_stream) {
	uint32_t l_mct_size;
	uint32_t l_tmp;
	
	assert(p_stream != nullptr);

	l_mct_size = 10 + p_mct_record->m_data_size;

	/* MCT */
	if (!p_stream->write_short(J2K_MS_MCT)) {
		return false;
	}

	/* Lmct */
	if (!p_stream->write_short((uint16_t) (l_mct_size - 2))) {
		return false;
	}

	/* Zmct */
	if (!p_stream->write_short(0)) {
		return false;
	}

	/* only one marker atm */
	l_tmp = (p_mct_record->m_index & 0xff) | (p_mct_record->m_array_type << 8)
			| (p_mct_record->m_element_type << 10);

	if (!p_stream->write_short((uint16_t) l_tmp)) {
		return false;
	}

	/* Ymct */
	if (!p_stream->write_short(0)) {
		return false;
	}

	if (!p_stream->write_bytes(p_mct_record->m_data, p_mct_record->m_data_size)) {
		return false;
	}
	return true;
}

/**
 * Reads a MCT marker (Multiple Component Transform)
 *
 * @param       p_header_data   the data contained in the MCT box.
 * @param       p_j2k                   the jpeg2000 codec.
 * @param       header_size   the size of the data contained in the MCT marker.

 */
static bool j2k_read_mct(grk_j2k *p_j2k, uint8_t *p_header_data,
		uint16_t header_size) {
	uint32_t i;
	uint32_t l_tmp;
	uint32_t l_indix;

	assert(p_header_data != nullptr);
	assert(p_j2k != nullptr);

	auto l_tcp = p_j2k->get_current_decode_tcp();

	if (header_size < 2) {
		GROK_ERROR( "Error reading MCT marker");
		return false;
	}

	/* first marker */
	/* Zmct */
	grk_read_bytes(p_header_data, &l_tmp, 2);
	p_header_data += 2;
	if (l_tmp != 0) {
		GROK_WARN(
				"Cannot take in charge mct data within multiple MCT records");
		return true;
	}

	if (header_size <= 6) {
		GROK_ERROR( "Error reading MCT marker");
		return false;
	}

	/* Imct -> no need for other values, take the first,
	 * type is double with decorrelation x0000 1101 0000 0000*/
	grk_read_bytes(p_header_data, &l_tmp, 2); /* Imct */
	p_header_data += 2;

	l_indix = l_tmp & 0xff;
	auto l_mct_data = l_tcp->m_mct_records;

	for (i = 0; i < l_tcp->m_nb_mct_records; ++i) {
		if (l_mct_data->m_index == l_indix) {
			break;
		}
		++l_mct_data;
	}

	bool newmct = false;
	// NOT FOUND
	if (i == l_tcp->m_nb_mct_records) {
		if (l_tcp->m_nb_mct_records == l_tcp->m_nb_max_mct_records) {
			grk_mct_data *new_mct_records;
			l_tcp->m_nb_max_mct_records += default_number_mct_records;

			new_mct_records = (grk_mct_data*) grk_realloc(l_tcp->m_mct_records,
					l_tcp->m_nb_max_mct_records * sizeof(grk_mct_data));
			if (!new_mct_records) {
				grok_free(l_tcp->m_mct_records);
				l_tcp->m_mct_records = nullptr;
				l_tcp->m_nb_max_mct_records = 0;
				l_tcp->m_nb_mct_records = 0;
				GROK_ERROR(
						"Not enough memory to read MCT marker");
				return false;
			}

			/* Update m_mcc_records[].m_offset_array and m_decorrelation_array
			 * to point to the new addresses */
			if (new_mct_records != l_tcp->m_mct_records) {
				for (i = 0; i < l_tcp->m_nb_mcc_records; ++i) {
					grk_simple_mcc_decorrelation_data *l_mcc_record =
							&(l_tcp->m_mcc_records[i]);
					if (l_mcc_record->m_decorrelation_array) {
						l_mcc_record->m_decorrelation_array = new_mct_records
								+ (l_mcc_record->m_decorrelation_array
										- l_tcp->m_mct_records);
					}
					if (l_mcc_record->m_offset_array) {
						l_mcc_record->m_offset_array = new_mct_records
								+ (l_mcc_record->m_offset_array
										- l_tcp->m_mct_records);
					}
				}
			}

			l_tcp->m_mct_records = new_mct_records;
			l_mct_data = l_tcp->m_mct_records + l_tcp->m_nb_mct_records;
			memset(l_mct_data, 0,
					(l_tcp->m_nb_max_mct_records - l_tcp->m_nb_mct_records)
							* sizeof(grk_mct_data));
		}

		l_mct_data = l_tcp->m_mct_records + l_tcp->m_nb_mct_records;
		newmct = true;
	}

	if (l_mct_data->m_data) {
		grok_free(l_mct_data->m_data);
		l_mct_data->m_data = nullptr;
		l_mct_data->m_data_size = 0;
	}

	l_mct_data->m_index = l_indix;
	l_mct_data->m_array_type = (J2K_MCT_ARRAY_TYPE) ((l_tmp >> 8) & 3);
	l_mct_data->m_element_type = (J2K_MCT_ELEMENT_TYPE) ((l_tmp >> 10) & 3);

	/* Ymct */
	grk_read_bytes(p_header_data, &l_tmp, 2);
	p_header_data += 2;
	if (l_tmp != 0) {
		GROK_WARN(
				"Cannot take in charge multiple MCT markers");
		return true;
	}
	if (header_size < 6) {
		GROK_ERROR( "Error reading MCT markers");
		return false;
	}
	header_size = (uint16_t)(header_size - 6);

	l_mct_data->m_data = (uint8_t*) grk_malloc(header_size);
	if (!l_mct_data->m_data) {
		GROK_ERROR( "Error reading MCT marker");
		return false;
	}
	memcpy(l_mct_data->m_data, p_header_data, header_size);
	l_mct_data->m_data_size = header_size;
	if (newmct)
		++l_tcp->m_nb_mct_records;

	return true;
}

static bool j2k_write_mcc_record(grk_simple_mcc_decorrelation_data *p_mcc_record,
		BufferedStream *p_stream) {
	uint32_t i;
	uint32_t l_mcc_size;
	uint32_t l_nb_bytes_for_comp;
	uint32_t l_mask;
	uint32_t l_tmcc;

	
	assert(p_stream != nullptr);

	if (p_mcc_record->m_nb_comps > 255) {
		l_nb_bytes_for_comp = 2;
		l_mask = 0x8000;
	} else {
		l_nb_bytes_for_comp = 1;
		l_mask = 0;
	}

	l_mcc_size = p_mcc_record->m_nb_comps * 2 * l_nb_bytes_for_comp + 19;

	/* MCC */
	if (!p_stream->write_short(J2K_MS_MCC)) {
		return false;
	}

	/* Lmcc */
	if (!p_stream->write_short((uint16_t) (l_mcc_size - 2))) {
		return false;
	}

	/* first marker */
	/* Zmcc */
	if (!p_stream->write_short(0)) {
		return false;
	}

	/* Imcc -> no need for other values, take the first */
	if (!p_stream->write_byte((uint8_t) p_mcc_record->m_index)) {
		return false;
	}

	/* only one marker atm */
	/* Ymcc */
	if (!p_stream->write_short(0)) {
		return false;
	}

	/* Qmcc -> number of collections -> 1 */
	if (!p_stream->write_short(1)) {
		return false;
	}

	/* Xmcci type of component transformation -> array based decorrelation */
	if (!p_stream->write_byte(0x1)) {
		return false;
	}

	/* Nmcci number of input components involved and size for each component offset = 8 bits */
	if (!p_stream->write_short((uint16_t) (p_mcc_record->m_nb_comps | l_mask))) {
		return false;
	}

	for (i = 0; i < p_mcc_record->m_nb_comps; ++i) {
		/* Cmccij Component offset*/
		if (l_nb_bytes_for_comp == 2) {
			if (!p_stream->write_short((uint16_t) i)) {
				return false;
			}
		} else {
			if (!p_stream->write_byte((uint8_t) i)) {
				return false;
			}
		}
	}

	/* Mmcci number of output components involved and size for each component offset = 8 bits */
	if (!p_stream->write_short((uint16_t) (p_mcc_record->m_nb_comps | l_mask))) {
		return false;
	}

	for (i = 0; i < p_mcc_record->m_nb_comps; ++i) {
		/* Wmccij Component offset*/
		if (l_nb_bytes_for_comp == 2) {
			if (!p_stream->write_short((uint16_t) i)) {
				return false;
			}
		} else {
			if (!p_stream->write_byte((uint8_t) i)) {
				return false;
			}
		}
	}

	l_tmcc = ((uint32_t) ((!p_mcc_record->m_is_irreversible) & 1U)) << 16;

	if (p_mcc_record->m_decorrelation_array) {
		l_tmcc |= p_mcc_record->m_decorrelation_array->m_index;
	}

	if (p_mcc_record->m_offset_array) {
		l_tmcc |= ((p_mcc_record->m_offset_array->m_index) << 8);
	}

	/* Tmcci : use MCT defined as number 1 and irreversible array based. */
	if (!p_stream->write_24(l_tmcc)) {
		return false;
	}
	return true;
}

static bool j2k_read_mcc(grk_j2k *p_j2k, uint8_t *p_header_data,
		uint16_t header_size) {
	uint32_t i, j;
	uint32_t l_tmp;
	uint32_t l_indix;
	uint32_t l_nb_collections;
	uint32_t l_nb_comps;
	uint32_t l_nb_bytes_by_comp;

	assert(p_header_data != nullptr);
	assert(p_j2k != nullptr);
	

	auto l_tcp = p_j2k->get_current_decode_tcp();

	if (header_size < 2) {
		GROK_ERROR( "Error reading MCC marker");
		return false;
	}

	/* first marker */
	/* Zmcc */
	grk_read_bytes(p_header_data, &l_tmp, 2);
	p_header_data += 2;
	if (l_tmp != 0) {
		GROK_WARN(
				"Cannot take in charge multiple data spanning");
		return true;
	}

	if (header_size < 7) {
		GROK_ERROR( "Error reading MCC marker");
		return false;
	}

	grk_read_bytes(p_header_data, &l_indix, 1); /* Imcc -> no need for other values, take the first */
	++p_header_data;

	auto l_mcc_record = l_tcp->m_mcc_records;

	for (i = 0; i < l_tcp->m_nb_mcc_records; ++i) {
		if (l_mcc_record->m_index == l_indix) {
			break;
		}
		++l_mcc_record;
	}

	/** NOT FOUND */
	bool newmcc = false;
	if (i == l_tcp->m_nb_mcc_records) {
		// resize l_tcp->m_nb_mcc_records if necessary
		if (l_tcp->m_nb_mcc_records == l_tcp->m_nb_max_mcc_records) {
			grk_simple_mcc_decorrelation_data *new_mcc_records;
			l_tcp->m_nb_max_mcc_records += default_number_mcc_records;

			new_mcc_records = (grk_simple_mcc_decorrelation_data*) grk_realloc(
					l_tcp->m_mcc_records,
					l_tcp->m_nb_max_mcc_records
							* sizeof(grk_simple_mcc_decorrelation_data));
			if (!new_mcc_records) {
				grok_free(l_tcp->m_mcc_records);
				l_tcp->m_mcc_records = nullptr;
				l_tcp->m_nb_max_mcc_records = 0;
				l_tcp->m_nb_mcc_records = 0;
				GROK_ERROR(
						"Not enough memory to read MCC marker");
				return false;
			}
			l_tcp->m_mcc_records = new_mcc_records;
			l_mcc_record = l_tcp->m_mcc_records + l_tcp->m_nb_mcc_records;
			memset(l_mcc_record, 0,
					(l_tcp->m_nb_max_mcc_records - l_tcp->m_nb_mcc_records)
							* sizeof(grk_simple_mcc_decorrelation_data));
		}
		// set pointer to prospective new mcc record
		l_mcc_record = l_tcp->m_mcc_records + l_tcp->m_nb_mcc_records;
		newmcc = true;
	}
	l_mcc_record->m_index = l_indix;

	/* only one marker atm */
	/* Ymcc */
	grk_read_bytes(p_header_data, &l_tmp, 2);
	p_header_data += 2;
	if (l_tmp != 0) {
		GROK_WARN(
				"Cannot take in charge multiple data spanning");
		return true;
	}

	/* Qmcc -> number of collections -> 1 */
	grk_read_bytes(p_header_data, &l_nb_collections, 2);
	p_header_data += 2;

	if (l_nb_collections > 1) {
		GROK_WARN(
				"Cannot take in charge multiple collections");
		return true;
	}
	header_size = (uint16_t)(header_size - 7);

	for (i = 0; i < l_nb_collections; ++i) {
		if (header_size < 3) {
			GROK_ERROR( "Error reading MCC marker");
			return false;
		}

		grk_read_bytes(p_header_data, &l_tmp, 1); /* Xmcci type of component transformation -> array based decorrelation */
		++p_header_data;

		if (l_tmp != 1) {
			GROK_WARN(
					"Cannot take in charge collections other than array decorrelation");
			return true;
		}

		grk_read_bytes(p_header_data, &l_nb_comps, 2);

		p_header_data += 2;
		header_size = (uint16_t)(header_size - 3);

		l_nb_bytes_by_comp = 1 + (l_nb_comps >> 15);
		l_mcc_record->m_nb_comps = l_nb_comps & 0x7fff;

		if (header_size
				< (l_nb_bytes_by_comp * l_mcc_record->m_nb_comps + 2)) {
			GROK_ERROR( "Error reading MCC marker");
			return false;
		}

		header_size = (uint16_t)(header_size - (l_nb_bytes_by_comp * l_mcc_record->m_nb_comps + 2));

		for (j = 0; j < l_mcc_record->m_nb_comps; ++j) {
			/* Cmccij Component offset*/
			grk_read_bytes(p_header_data, &l_tmp, l_nb_bytes_by_comp);
			p_header_data += l_nb_bytes_by_comp;

			if (l_tmp != j) {
				GROK_WARN(
						"Cannot take in charge collections with indix shuffle");
				return true;
			}
		}

		grk_read_bytes(p_header_data, &l_nb_comps, 2);
		p_header_data += 2;

		l_nb_bytes_by_comp = 1 + (l_nb_comps >> 15);
		l_nb_comps &= 0x7fff;

		if (l_nb_comps != l_mcc_record->m_nb_comps) {
			GROK_WARN(
					"Cannot take in charge collections without same number of indices");
			return true;
		}

		if (header_size
				< (l_nb_bytes_by_comp * l_mcc_record->m_nb_comps + 3)) {
			GROK_ERROR( "Error reading MCC marker");
			return false;
		}

		header_size = (uint16_t)(header_size - (l_nb_bytes_by_comp * l_mcc_record->m_nb_comps + 3));

		for (j = 0; j < l_mcc_record->m_nb_comps; ++j) {
			/* Wmccij Component offset*/
			grk_read_bytes(p_header_data, &l_tmp, l_nb_bytes_by_comp);
			p_header_data += l_nb_bytes_by_comp;

			if (l_tmp != j) {
				GROK_WARN(
						"Cannot take in charge collections with indix shuffle");
				return true;
			}
		}
		/* Wmccij Component offset*/
		grk_read_bytes(p_header_data, &l_tmp, 3);
		p_header_data += 3;

		l_mcc_record->m_is_irreversible = !((l_tmp >> 16) & 1);
		l_mcc_record->m_decorrelation_array = nullptr;
		l_mcc_record->m_offset_array = nullptr;

		l_indix = l_tmp & 0xff;
		if (l_indix != 0) {
			auto l_mct_data = l_tcp->m_mct_records;
			for (j = 0; j < l_tcp->m_nb_mct_records; ++j) {
				if (l_mct_data->m_index == l_indix) {
					l_mcc_record->m_decorrelation_array = l_mct_data;
					break;
				}
				++l_mct_data;
			}

			if (l_mcc_record->m_decorrelation_array == nullptr) {
				GROK_ERROR( "Error reading MCC marker");
				return false;
			}
		}

		l_indix = (l_tmp >> 8) & 0xff;
		if (l_indix != 0) {
			auto l_mct_data = l_tcp->m_mct_records;
			for (j = 0; j < l_tcp->m_nb_mct_records; ++j) {
				if (l_mct_data->m_index == l_indix) {
					l_mcc_record->m_offset_array = l_mct_data;
					break;
				}
				++l_mct_data;
			}

			if (l_mcc_record->m_offset_array == nullptr) {
				GROK_ERROR( "Error reading MCC marker");
				return false;
			}
		}
	}

	if (header_size != 0) {
		GROK_ERROR( "Error reading MCC marker");
		return false;
	}

	// only increment mcc record count if we are working on a new mcc
	// and everything succeeded
	if (newmcc)
		++l_tcp->m_nb_mcc_records;

	return true;
}

static bool j2k_write_mco(grk_j2k *p_j2k, BufferedStream *p_stream) {
	uint32_t l_mco_size;
	uint32_t i;

	assert(p_j2k != nullptr);
	assert(p_stream != nullptr);

	auto l_tcp = &(p_j2k->m_cp.tcps[p_j2k->m_current_tile_number]);
	l_mco_size = 5 + l_tcp->m_nb_mcc_records;

	/* MCO */
	if (!p_stream->write_short(J2K_MS_MCO)) {
		return false;
	}

	/* Lmco */
	if (!p_stream->write_short((uint16_t) (l_mco_size - 2))) {
		return false;
	}

	/* Nmco : only one transform stage*/
	if (!p_stream->write_byte((uint8_t) l_tcp->m_nb_mcc_records)) {
		return false;
	}

	auto l_mcc_record = l_tcp->m_mcc_records;
	for (i = 0; i < l_tcp->m_nb_mcc_records; ++i) {
		/* Imco -> use the mcc indicated by 1*/
		if (!p_stream->write_byte((uint8_t) l_mcc_record->m_index)) {
			return false;
		}
		++l_mcc_record;
	}
	return true;
}

/**
 * Reads a MCO marker (Multiple Component Transform Ordering)
 *
 * @param       p_header_data   the data contained in the MCO box.
 * @param       p_j2k                   the jpeg2000 codec.
 * @param       header_size   the size of the data contained in the MCO marker.

 */
static bool j2k_read_mco(grk_j2k *p_j2k, uint8_t *p_header_data,
		uint16_t header_size) {
	uint32_t l_tmp, i;
	uint32_t l_nb_stages;
	assert(p_header_data != nullptr);
	assert(p_j2k != nullptr);
	

	auto l_image = p_j2k->m_private_image;
	auto l_tcp = p_j2k->get_current_decode_tcp();

	if (header_size < 1) {
		GROK_ERROR( "Error reading MCO marker");
		return false;
	}
	/* Nmco : only one transform stage*/
	grk_read_bytes(p_header_data, &l_nb_stages, 1);
	++p_header_data;

	if (l_nb_stages > 1) {
		GROK_WARN(
				"Cannot take in charge multiple transformation stages.");
		return true;
	}

	if (header_size != l_nb_stages + 1) {
		GROK_WARN( "Error reading MCO marker");
		return false;
	}

	auto l_tccp = l_tcp->tccps;

	for (i = 0; i < l_image->numcomps; ++i) {
		l_tccp->m_dc_level_shift = 0;
		++l_tccp;
	}

	if (l_tcp->m_mct_decoding_matrix) {
		grok_free(l_tcp->m_mct_decoding_matrix);
		l_tcp->m_mct_decoding_matrix = nullptr;
	}

	for (i = 0; i < l_nb_stages; ++i) {
		grk_read_bytes(p_header_data, &l_tmp, 1);
		++p_header_data;

		if (!j2k_add_mct(l_tcp, p_j2k->m_private_image, l_tmp)) {
			return false;
		}
	}

	return true;
}

static bool j2k_add_mct(grk_tcp *p_tcp, grk_image *p_image, uint32_t index) {
	uint32_t i;
	uint32_t l_data_size, l_mct_size, l_offset_size;
	uint32_t l_nb_elem;
	grk_tccp *l_tccp;

	assert(p_tcp != nullptr);

	auto l_mcc_record = p_tcp->m_mcc_records;

	for (i = 0; i < p_tcp->m_nb_mcc_records; ++i) {
		if (l_mcc_record->m_index == index) {
			break;
		}
	}

	if (i == p_tcp->m_nb_mcc_records) {
		/** element discarded **/
		return true;
	}

	if (l_mcc_record->m_nb_comps != p_image->numcomps) {
		/** do not support number of comps != image */
		return true;
	}

	auto l_deco_array = l_mcc_record->m_decorrelation_array;

	if (l_deco_array) {
		l_data_size = MCT_ELEMENT_SIZE[l_deco_array->m_element_type]
				* p_image->numcomps * p_image->numcomps;
		if (l_deco_array->m_data_size != l_data_size) {
			return false;
		}

		l_nb_elem = p_image->numcomps * p_image->numcomps;
		l_mct_size = l_nb_elem * (uint32_t) sizeof(float);
		p_tcp->m_mct_decoding_matrix = (float*) grk_malloc(l_mct_size);

		if (!p_tcp->m_mct_decoding_matrix) {
			return false;
		}

		j2k_mct_read_functions_to_float[l_deco_array->m_element_type](
				l_deco_array->m_data, p_tcp->m_mct_decoding_matrix, l_nb_elem);
	}

	auto l_offset_array = l_mcc_record->m_offset_array;

	if (l_offset_array) {
		l_data_size = MCT_ELEMENT_SIZE[l_offset_array->m_element_type]
				* p_image->numcomps;
		if (l_offset_array->m_data_size != l_data_size) {
			return false;
		}

		l_nb_elem = p_image->numcomps;
		l_offset_size = l_nb_elem * (uint32_t) sizeof(uint32_t);
		auto l_offset_data = (uint32_t*) grk_malloc(l_offset_size);

		if (!l_offset_data) {
			return false;
		}

		j2k_mct_read_functions_to_int32[l_offset_array->m_element_type](
				l_offset_array->m_data, l_offset_data, l_nb_elem);

		l_tccp = p_tcp->tccps;
		auto l_current_offset_data = l_offset_data;

		for (i = 0; i < p_image->numcomps; ++i) {
			l_tccp->m_dc_level_shift = (int32_t) *(l_current_offset_data++);
			++l_tccp;
		}

		grok_free(l_offset_data);
	}

	return true;
}

static bool j2k_write_cbd(grk_j2k *p_j2k, BufferedStream *p_stream) {
	uint32_t i;
	assert(p_j2k != nullptr);
	assert(p_stream != nullptr);

	auto l_image = p_j2k->m_private_image;
	uint32_t l_cbd_size = 6 + p_j2k->m_private_image->numcomps;

	/* CBD */
	if (!p_stream->write_short(J2K_MS_CBD)) {
		return false;
	}

	/* L_CBD */
	if (!p_stream->write_short((uint16_t) (l_cbd_size - 2))) {
		return false;
	}

	/* Ncbd */
	if (!p_stream->write_short((uint16_t) l_image->numcomps)) {
		return false;
	}

	auto l_comp = l_image->comps;

	for (i = 0; i < l_image->numcomps; ++i) {
		/* Component bit depth */
		if (!p_stream->write_byte(
				(uint8_t) ((l_comp->sgnd << 7) | (l_comp->prec - 1)))) {
			return false;
		}
		++l_comp;
	}
	return true;
}

/**
 * Reads a CBD marker (Component bit depth definition)
 * @param       p_header_data   the data contained in the CBD box.
 * @param       p_j2k                   the jpeg2000 codec.
 * @param       header_size   the size of the data contained in the CBD marker.

 */
static bool j2k_read_cbd(grk_j2k *p_j2k, uint8_t *p_header_data,
		uint16_t header_size) {
	uint32_t l_nb_comp, l_num_comp;
	uint32_t l_comp_def;
	uint32_t i;
	assert(p_header_data != nullptr);
	assert(p_j2k != nullptr);
	

	l_num_comp = p_j2k->m_private_image->numcomps;

	if (header_size != (p_j2k->m_private_image->numcomps + 2)) {
		GROK_ERROR( "Crror reading CBD marker");
		return false;
	}
	/* Ncbd */
	grk_read_bytes(p_header_data, &l_nb_comp, 2);
	p_header_data += 2;

	if (l_nb_comp != l_num_comp) {
		GROK_ERROR( "Crror reading CBD marker");
		return false;
	}

	auto l_comp = p_j2k->m_private_image->comps;
	for (i = 0; i < l_num_comp; ++i) {
		/* Component bit depth */
		grk_read_bytes(p_header_data, &l_comp_def, 1);
		++p_header_data;
		l_comp->sgnd = (l_comp_def >> 7) & 1;
		l_comp->prec = (l_comp_def & 0x7f) + 1;
		++l_comp;
	}

	return true;
}

static bool j2k_check_poc_val(const  grk_poc  *p_pocs, uint32_t nb_pocs,
		uint32_t nb_resolutions, uint32_t num_comps, uint32_t num_layers) {
	uint32_t *packet_array;
	uint32_t index, resno, compno, layno;
	uint32_t i;
	uint32_t step_c = 1;
	uint32_t step_r = num_comps * step_c;
	uint32_t step_l = nb_resolutions * step_r;
	if (nb_pocs == 0) {
		return true;
	}
	packet_array = new uint32_t[step_l * num_layers];
	memset(packet_array, 0, step_l * num_layers * sizeof(uint32_t));

	/* iterate through all the pocs */
	for (i = 0; i < nb_pocs; ++i) {
		index = step_r * p_pocs->resno0;
		/* take each resolution for each poc */
		for (resno = p_pocs->resno0;
				resno < std::min<uint32_t>(p_pocs->resno1, nb_resolutions);
				++resno) {
			uint32_t res_index = index + p_pocs->compno0 * step_c;

			/* take each comp of each resolution for each poc */
			for (compno = p_pocs->compno0;
					compno < std::min<uint32_t>(p_pocs->compno1, num_comps);
					++compno) {
				const uint32_t layno0 = 0;
				uint32_t comp_index = res_index + layno0 * step_l;

				/* and finally take each layer of each res of ... */
				for (layno = layno0;
						layno < std::min<uint32_t>(p_pocs->layno1, num_layers);
						++layno) {
					/*index = step_r * resno + step_c * compno + step_l * layno;*/
					packet_array[comp_index] = 1;
					comp_index += step_l;
				}
				res_index += step_c;
			}
			index += step_r;
		}
		++p_pocs;
	}

	bool loss = false;
	index = 0;
	for (layno = 0; layno < num_layers; ++layno) {
		for (resno = 0; resno < nb_resolutions; ++resno) {
			for (compno = 0; compno < num_comps; ++compno) {
				loss |= (packet_array[index] != 1);
				index += step_c;
			}
		}
	}

	if (loss) {
		GROK_ERROR(
				"Missing packets possible loss of data");
	}
	delete[] packet_array;
	return !loss;
}


static bool j2k_add_mhmarker( grk_codestream_index  *cstr_index, uint32_t type,
		uint64_t pos, uint32_t len) {
	assert(cstr_index != nullptr);

	/* expand the list? */
	if ((cstr_index->marknum + 1) > cstr_index->maxmarknum) {
		 grk_marker_info  *new_marker;
		cstr_index->maxmarknum = (uint32_t) (100
				+ (float) cstr_index->maxmarknum);
		new_marker = ( grk_marker_info  * ) grk_realloc(cstr_index->marker,
				cstr_index->maxmarknum * sizeof( grk_marker_info) );
		if (!new_marker) {
			grok_free(cstr_index->marker);
			cstr_index->marker = nullptr;
			cstr_index->maxmarknum = 0;
			cstr_index->marknum = 0;
			/* GROK_ERROR( "Not enough memory to add mh marker"); */
			return false;
		}
		cstr_index->marker = new_marker;
	}

	/* add the marker */
	cstr_index->marker[cstr_index->marknum].type = (uint16_t) type;
	cstr_index->marker[cstr_index->marknum].pos = (uint64_t) pos;
	cstr_index->marker[cstr_index->marknum].len = (uint32_t) len;
	cstr_index->marknum++;
	return true;
}

static bool j2k_add_tlmarker(uint16_t tileno,
		 grk_codestream_index  *cstr_index, uint32_t type, uint64_t pos,
		uint32_t len) {
	assert(cstr_index != nullptr);
	assert(cstr_index->tile_index != nullptr);

	/* expand the list? */
	if ((cstr_index->tile_index[tileno].marknum + 1)
			> cstr_index->tile_index[tileno].maxmarknum) {
		 grk_marker_info  *new_marker;
		cstr_index->tile_index[tileno].maxmarknum = (uint32_t) (100
				+ (float) cstr_index->tile_index[tileno].maxmarknum);
		new_marker = ( grk_marker_info  * ) grk_realloc(
				cstr_index->tile_index[tileno].marker,
				cstr_index->tile_index[tileno].maxmarknum
						* sizeof( grk_marker_info) );
		if (!new_marker) {
			grok_free(cstr_index->tile_index[tileno].marker);
			cstr_index->tile_index[tileno].marker = nullptr;
			cstr_index->tile_index[tileno].maxmarknum = 0;
			cstr_index->tile_index[tileno].marknum = 0;
			GROK_ERROR( "Not enough memory to add tl marker");
			return false;
		}
		cstr_index->tile_index[tileno].marker = new_marker;
	}

	/* add the marker */
	cstr_index->tile_index[tileno].marker[cstr_index->tile_index[tileno].marknum].type =
			(uint16_t) type;
	cstr_index->tile_index[tileno].marker[cstr_index->tile_index[tileno].marknum].pos =
			pos;
	cstr_index->tile_index[tileno].marker[cstr_index->tile_index[tileno].marknum].len =
			(uint32_t) len;
	cstr_index->tile_index[tileno].marknum++;

	if (type == J2K_MS_SOT) {
		uint32_t l_current_tile_part =
				cstr_index->tile_index[tileno].current_tpsno;

		if (cstr_index->tile_index[tileno].tp_index)
			cstr_index->tile_index[tileno].tp_index[l_current_tile_part].start_pos =
					pos;

	}
	return true;
}

/* FIXME DOC*/
static bool j2k_copy_default_tcp_and_create_tcd(grk_j2k *p_j2k,
		BufferedStream *p_stream) {
	
	(void) p_stream;
	uint32_t l_nb_tiles;
	uint32_t i, j;
	uint32_t l_tccp_size;
	uint32_t l_mct_size;
	grk_image *l_image;
	uint32_t l_mcc_records_size, l_mct_records_size;
	grk_mct_data *l_src_mct_rec, *l_dest_mct_rec;
	grk_simple_mcc_decorrelation_data *l_src_mcc_rec, *l_dest_mcc_rec;
	uint32_t l_offset;

	assert(p_j2k != nullptr);
	assert(p_stream != nullptr);
	

	l_image = p_j2k->m_private_image;
	l_nb_tiles = p_j2k->m_cp.th * p_j2k->m_cp.tw;
	auto l_tcp = p_j2k->m_cp.tcps;
	l_tccp_size = l_image->numcomps * (uint32_t) sizeof(grk_tccp);
	auto l_default_tcp = p_j2k->m_specific_param.m_decoder.m_default_tcp;
	l_mct_size = l_image->numcomps * l_image->numcomps
			* (uint32_t) sizeof(float);

	/* For each tile */
	for (i = 0; i < l_nb_tiles; ++i) {
		/* keep the tile-compo coding parameters pointer of the current tile coding parameters*/
		auto l_current_tccp = l_tcp->tccps;
		/*Copy default coding parameters into the current tile coding parameters*/
		memcpy(l_tcp, l_default_tcp, sizeof(grk_tcp));
		/* Initialize some values of the current tile coding parameters*/
		l_tcp->cod = 0;
		l_tcp->ppt = 0;
		l_tcp->ppt_data = nullptr;
		/* Remove memory not owned by this tile in case of early error return. */
		l_tcp->m_mct_decoding_matrix = nullptr;
		l_tcp->m_nb_max_mct_records = 0;
		l_tcp->m_mct_records = nullptr;
		l_tcp->m_nb_max_mcc_records = 0;
		l_tcp->m_mcc_records = nullptr;
		/* Reconnect the tile-compo coding parameters pointer to the current tile coding parameters*/
		l_tcp->tccps = l_current_tccp;

		/* Get the mct_decoding_matrix of the dflt_tile_cp and copy them into the current tile cp*/
		if (l_default_tcp->m_mct_decoding_matrix) {
			l_tcp->m_mct_decoding_matrix = (float*) grk_malloc(l_mct_size);
			if (!l_tcp->m_mct_decoding_matrix) {
				return false;
			}
			memcpy(l_tcp->m_mct_decoding_matrix,
					l_default_tcp->m_mct_decoding_matrix, l_mct_size);
		}

		/* Get the mct_record of the dflt_tile_cp and copy them into the current tile cp*/
		l_mct_records_size = l_default_tcp->m_nb_max_mct_records
				* (uint32_t) sizeof(grk_mct_data);
		l_tcp->m_mct_records = (grk_mct_data*) grk_malloc(l_mct_records_size);
		if (!l_tcp->m_mct_records) {
			return false;
		}
		memcpy(l_tcp->m_mct_records, l_default_tcp->m_mct_records,
				l_mct_records_size);

		/* Copy the mct record data from dflt_tile_cp to the current tile*/
		l_src_mct_rec = l_default_tcp->m_mct_records;
		l_dest_mct_rec = l_tcp->m_mct_records;

		for (j = 0; j < l_default_tcp->m_nb_mct_records; ++j) {

			if (l_src_mct_rec->m_data) {

				l_dest_mct_rec->m_data = (uint8_t*) grk_malloc(
						l_src_mct_rec->m_data_size);
				if (!l_dest_mct_rec->m_data) {
					return false;
				}
				memcpy(l_dest_mct_rec->m_data, l_src_mct_rec->m_data,
						l_src_mct_rec->m_data_size);
			}

			++l_src_mct_rec;
			++l_dest_mct_rec;
			/* Update with each pass to free exactly what has been allocated on early return. */
			l_tcp->m_nb_max_mct_records += 1;
		}

		/* Get the mcc_record of the dflt_tile_cp and copy them into the current tile cp*/
		l_mcc_records_size = l_default_tcp->m_nb_max_mcc_records
				* (uint32_t) sizeof(grk_simple_mcc_decorrelation_data);
		l_tcp->m_mcc_records = (grk_simple_mcc_decorrelation_data*) grk_malloc(
				l_mcc_records_size);
		if (!l_tcp->m_mcc_records) {
			return false;
		}
		memcpy(l_tcp->m_mcc_records, l_default_tcp->m_mcc_records,
				l_mcc_records_size);
		l_tcp->m_nb_max_mcc_records = l_default_tcp->m_nb_max_mcc_records;

		/* Copy the mcc record data from dflt_tile_cp to the current tile*/
		l_src_mcc_rec = l_default_tcp->m_mcc_records;
		l_dest_mcc_rec = l_tcp->m_mcc_records;

		for (j = 0; j < l_default_tcp->m_nb_max_mcc_records; ++j) {

			if (l_src_mcc_rec->m_decorrelation_array) {
				l_offset = (uint32_t) (l_src_mcc_rec->m_decorrelation_array
						- l_default_tcp->m_mct_records);
				l_dest_mcc_rec->m_decorrelation_array = l_tcp->m_mct_records
						+ l_offset;
			}

			if (l_src_mcc_rec->m_offset_array) {
				l_offset = (uint32_t) (l_src_mcc_rec->m_offset_array
						- l_default_tcp->m_mct_records);
				l_dest_mcc_rec->m_offset_array = l_tcp->m_mct_records
						+ l_offset;
			}

			++l_src_mcc_rec;
			++l_dest_mcc_rec;
		}

		/* Copy all the dflt_tile_compo_cp to the current tile cp */
		memcpy(l_current_tccp, l_default_tcp->tccps, l_tccp_size);

		/* Move to next tile cp*/
		++l_tcp;
	}

	/* Create the current tile decoder*/
	p_j2k->m_tileProcessor = new TileProcessor(true);

	if (!p_j2k->m_tileProcessor->init(l_image, &(p_j2k->m_cp))) {
		delete p_j2k->m_tileProcessor;
		p_j2k->m_tileProcessor = nullptr;
		GROK_ERROR( "Cannot decode tile, memory error");
		return false;
	}

	return true;
}

static uint32_t j2k_get_SPCod_SPCoc_size(grk_j2k *p_j2k, uint16_t tile_no,
		uint32_t comp_no) {
	assert(p_j2k != nullptr);

	auto l_cp = &(p_j2k->m_cp);
	auto l_tcp = &l_cp->tcps[tile_no];
	auto l_tccp = &l_tcp->tccps[comp_no];

	/* preconditions again */
	assert(tile_no < (l_cp->tw * l_cp->th));
	assert(comp_no < p_j2k->m_private_image->numcomps);

	if (l_tccp->csty & J2K_CCP_CSTY_PRT) {
		return 5 + l_tccp->numresolutions;
	} else {
		return 5;
	}
}

static bool j2k_compare_SPCod_SPCoc(grk_j2k *p_j2k, uint16_t tile_no,
		uint32_t first_comp_no, uint32_t second_comp_no) {
	assert(p_j2k != nullptr);

	auto l_cp = &(p_j2k->m_cp);
	auto l_tcp = &l_cp->tcps[tile_no];
	auto l_tccp0 = &l_tcp->tccps[first_comp_no];
	auto l_tccp1 = &l_tcp->tccps[second_comp_no];

	if (l_tccp0->numresolutions != l_tccp1->numresolutions) {
		return false;
	}
	if (l_tccp0->cblkw != l_tccp1->cblkw) {
		return false;
	}
	if (l_tccp0->cblkh != l_tccp1->cblkh) {
		return false;
	}
	if (l_tccp0->cblk_sty != l_tccp1->cblk_sty) {
		return false;
	}
	if (l_tccp0->qmfbid != l_tccp1->qmfbid) {
		return false;
	}
	if ((l_tccp0->csty & J2K_CCP_CSTY_PRT)
			!= (l_tccp1->csty & J2K_CCP_CSTY_PRT)) {
		return false;
	}

	for (uint32_t i = 0U; i < l_tccp0->numresolutions; ++i) {
		if (l_tccp0->prcw[i] != l_tccp1->prcw[i]) {
			return false;
		}
		if (l_tccp0->prch[i] != l_tccp1->prch[i]) {
			return false;
		}
	}
	return true;
}

static bool j2k_write_SPCod_SPCoc(grk_j2k *p_j2k, uint16_t tile_no,
		uint32_t comp_no, BufferedStream *p_stream) {
	assert(p_j2k != nullptr);
	
	auto l_cp = &(p_j2k->m_cp);
	auto l_tcp = &l_cp->tcps[tile_no];
	auto l_tccp = &l_tcp->tccps[comp_no];

	/* preconditions again */
	assert(tile_no < (l_cp->tw * l_cp->th));
	assert(comp_no < (p_j2k->m_private_image->numcomps));

	/* SPcoc (D) */
	if (!p_stream->write_byte((uint8_t) (l_tccp->numresolutions - 1))) {
		return false;
	}

	/* SPcoc (E) */
	if (!p_stream->write_byte((uint8_t) (l_tccp->cblkw - 2))) {
		return false;
	}

	/* SPcoc (F) */
	if (!p_stream->write_byte((uint8_t) (l_tccp->cblkh - 2))) {
		return false;
	}

	/* SPcoc (G) */
	if (!p_stream->write_byte(l_tccp->cblk_sty)) {
		return false;
	}

	/* SPcoc (H) */
	if (!p_stream->write_byte((uint8_t) l_tccp->qmfbid)) {
		return false;
	}

	if (l_tccp->csty & J2K_CCP_CSTY_PRT) {
		for (uint32_t i = 0; i < l_tccp->numresolutions; ++i) {
			/* SPcoc (I_i) */
			if (!p_stream->write_byte(
					(uint8_t) (l_tccp->prcw[i] + (l_tccp->prch[i] << 4)))) {
				return false;
			}
		}
	}

	return true;
}

static bool j2k_read_SPCod_SPCoc(grk_j2k *p_j2k, uint32_t compno,
		uint8_t *p_header_data, uint16_t *header_size) {
	uint32_t i, l_tmp;
	assert(p_j2k != nullptr);
	assert(p_header_data != nullptr);
	assert(compno < p_j2k->m_private_image->numcomps);

	auto l_cp = &(p_j2k->m_cp);
	auto l_tcp = p_j2k->get_current_decode_tcp();
	auto l_tccp = &l_tcp->tccps[compno];
	auto l_current_ptr = p_header_data;

	/* make sure room is sufficient */
	if (*header_size < 5) {
		GROK_ERROR( "Error reading SPCod SPCoc element");
		return false;
	}
	/* SPcox (D) */
	grk_read_bytes(l_current_ptr, &l_tccp->numresolutions, 1);
	++l_tccp->numresolutions;
	if (l_tccp->numresolutions > GRK_J2K_MAXRLVLS) {
		GROK_ERROR(
				"Number of resolutions %d is greater than"
				" maximum allowed number %d\n",
				l_tccp->numresolutions, GRK_J2K_MAXRLVLS);
		return false;
	}
	++l_current_ptr;

	/* If user wants to remove more resolutions than the codestream contains, return error */
	if (l_cp->m_coding_param.m_dec.m_reduce >= l_tccp->numresolutions) {
		GROK_ERROR(
				"Error decoding component %d.\nThe number of resolutions"
				" to remove is higher than the number "
				"of resolutions of this component\n"
				"Modify the cp_reduce parameter.\n\n",
				compno);
		p_j2k->m_specific_param.m_decoder.m_state |= 0x8000;/* FIXME J2K_DEC_STATE_ERR;*/
		return false;
	}
	/* SPcoc (E) */
	grk_read_bytes(l_current_ptr, &l_tccp->cblkw, 1);
	++l_current_ptr;
	l_tccp->cblkw += 2;
	/* SPcoc (F) */
	grk_read_bytes(l_current_ptr, &l_tccp->cblkh, 1);
	++l_current_ptr;
	l_tccp->cblkh += 2;

	if ((l_tccp->cblkw > 10) || (l_tccp->cblkh > 10)
			|| ((l_tccp->cblkw + l_tccp->cblkh) > 12)) {
		GROK_ERROR(
				"Error reading SPCod SPCoc element,"
				" Invalid cblkw/cblkh combination");
		return false;
	}

	/* SPcoc (G) */
	grk_read_8(l_current_ptr, &l_tccp->cblk_sty);
	++l_current_ptr;
	/* SPcoc (H) */
	grk_read_8(l_current_ptr, &l_tccp->qmfbid);
	if (l_tccp->qmfbid > 1){
		GROK_ERROR("Invalid qmfbid : %d. "
				"Should be either 0 or 1", l_tccp->qmfbid);
		return false;
	}

	++l_current_ptr;

	*header_size = (uint16_t)(*header_size - 5);

	/* use custom precinct size ? */
	if (l_tccp->csty & J2K_CCP_CSTY_PRT) {
		if (*header_size < l_tccp->numresolutions) {
			GROK_ERROR(
					"Error reading SPCod SPCoc element");
			return false;
		}

		for (i = 0; i < l_tccp->numresolutions; ++i) {
			/* SPcoc (I_i) */
			grk_read_bytes(l_current_ptr, &l_tmp, 1);
			++l_current_ptr;
			/* Precinct exponent 0 is only allowed for lowest resolution level (Table A.21) */
			if ((i != 0) && (((l_tmp & 0xf) == 0) || ((l_tmp >> 4) == 0))) {
				GROK_ERROR( "Invalid precinct size");
				return false;
			}
			l_tccp->prcw[i] = l_tmp & 0xf;
			l_tccp->prch[i] = l_tmp >> 4;
		}

		*header_size = (uint16_t)(*header_size - l_tccp->numresolutions);
	} else {
		/* set default size for the precinct width and height */
		for (i = 0; i < l_tccp->numresolutions; ++i) {
			l_tccp->prcw[i] = 15;
			l_tccp->prch[i] = 15;
		}
	}

	return true;
}

static uint32_t j2k_get_SQcd_SQcc_size(grk_j2k *p_j2k, uint16_t tile_no,
		uint32_t comp_no) {
	assert(p_j2k != nullptr);

	auto l_cp = &(p_j2k->m_cp);
	auto l_tcp = &l_cp->tcps[tile_no];
	auto l_tccp = &l_tcp->tccps[comp_no];

	return l_tccp->quant.get_SQcd_SQcc_size(p_j2k,tile_no,comp_no);
}

static bool j2k_compare_SQcd_SQcc(grk_j2k *p_j2k, uint16_t tile_no,
		uint32_t first_comp_no, uint32_t second_comp_no) {
	assert(p_j2k != nullptr);

	auto l_cp = &(p_j2k->m_cp);
	auto l_tcp = &l_cp->tcps[tile_no];
	auto l_tccp0 = &l_tcp->tccps[first_comp_no];

	return l_tccp0->quant.compare_SQcd_SQcc(p_j2k,tile_no,first_comp_no,second_comp_no);
}

static bool j2k_write_SQcd_SQcc(grk_j2k *p_j2k, uint16_t tile_no,
		uint32_t comp_no, BufferedStream *p_stream) {
	assert(p_j2k != nullptr);
	
	auto l_cp = &(p_j2k->m_cp);
	auto l_tcp = &l_cp->tcps[tile_no];
	auto l_tccp = &l_tcp->tccps[comp_no];
	return l_tccp->quant.write_SQcd_SQcc(p_j2k,tile_no,comp_no,p_stream);
}

static bool j2k_read_SQcd_SQcc(bool fromQCC, grk_j2k *p_j2k, uint32_t comp_no,
		uint8_t *p_header_data, uint16_t *header_size) {
	assert(p_j2k != nullptr);
	assert(p_header_data != nullptr);
	assert(comp_no < p_j2k->m_private_image->numcomps);
	auto l_tcp = p_j2k->get_current_decode_tcp();
	auto l_tccp = l_tcp->tccps + comp_no;
	return l_tccp->quant.read_SQcd_SQcc(fromQCC,p_j2k, comp_no, p_header_data, header_size);
}

/*******************
 * Miscellaneous
 ******************/


void grk_coding_parameters::destroy() {
	uint32_t l_nb_tiles;
	grk_tcp *l_current_tile = nullptr;
	if (tcps != nullptr) {
		uint32_t i;
		l_current_tile = tcps;
		l_nb_tiles = th * tw;

		for (i = 0U; i < l_nb_tiles; ++i) {
			j2k_tcp_destroy(l_current_tile);
			++l_current_tile;
		}
		delete[] tcps;
		tcps = nullptr;
	}
	if (ppm_markers != nullptr) {
		uint32_t i;
		for (i = 0U; i < ppm_markers_count; ++i) {
			if (ppm_markers[i].m_data != nullptr) {
				grok_free(ppm_markers[i].m_data);
			}
		}
		ppm_markers_count = 0U;
		grok_free(ppm_markers);
		ppm_markers = nullptr;
	}
	grok_free(ppm_buffer);
	ppm_buffer = nullptr;
	ppm_data = nullptr; /* ppm_data belongs to the allocated buffer pointed by ppm_buffer */
	for (size_t i = 0; i < num_comments; ++i) {
		grk_buffer_delete((uint8_t*) comment[i]);
		comment[i] = nullptr;
	}
	num_comments = 0;
    delete pl_marker;
    delete tl_marker;
}


grk_tcp::grk_tcp() :
				csty(0),
				prg(GRK_PROG_UNKNOWN),
				numlayers(0),
				num_layers_to_decode(0),
				mct(0),
				numpocs(0),
				ppt_markers_count(0),
				ppt_markers(nullptr),
				ppt_data(nullptr),
				ppt_buffer(nullptr),
				ppt_data_size(0),
				ppt_len(0),
				main_qcd_qntsty(0),
				main_qcd_numStepSizes(0),
				tccps(nullptr),
				m_current_tile_part_number(-1),
				m_nb_tile_parts(0),
				m_tile_data(nullptr),
				mct_norms(nullptr),
				m_mct_decoding_matrix(nullptr),
				m_mct_coding_matrix(nullptr),
				m_mct_records(nullptr),
				m_nb_mct_records(0),
				m_nb_max_mct_records(0),
				m_mcc_records(nullptr),
				m_nb_mcc_records(0),
				m_nb_max_mcc_records(0),
				cod(0),
				ppt(0),
				POC(0),
				isHT(false)
{
	for (auto i = 0; i < 100; ++i)
		rates[i] = 0.0;
	for (auto i = 0; i < 100; ++i)
		distoratio[i] = 0;
	for (auto i = 0; i < 32; ++i)
		memset(pocs + i, 0, sizeof( grk_poc) );
}


}
