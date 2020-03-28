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
 * Copyright (c) 2007, Callum Lerwick <seg@haxxed.com>
 * Copyright (c) 2012, Carl Hetherington
 * Copyright (c) 2017, IntoPIX SA <support@intopix.com>
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
#include "grok.h"
#include "t1_common.h"
#include "logger.h"
#include "dwt_utils.h"

using namespace std;

namespace grk {

#define T1_TYPE_MQ 0    /**< Normal coding using entropy coder */
#define T1_TYPE_RAW 1   /**< No encoding the information is store under raw format
							in codestream (mode switch RAW)*/

#include "t1_luts.h"
#define T1_FLAGS(x, y) (t1->flags[x + 1 + ((y>>2) + 1) * (t1->w+2)])
#define t1_setcurctx(curctx, ctxno)  curctx = &(mqc)->ctxs[(uint32_t)(ctxno)]

static INLINE uint8_t 	t1_getctxno_zc(mqc_t *mqc, uint32_t f);
static INLINE uint32_t 	t1_getctxno_mag(uint32_t f);
static int16_t 			t1_getnmsedec_sig(uint32_t x, uint32_t bitpos);
static int16_t 			t1_getnmsedec_ref(uint32_t x, uint32_t bitpos);
static INLINE void 		t1_update_flags(grk_flag *flagsp, uint32_t ci, uint32_t s,
										uint32_t stride, uint32_t vsc);
static INLINE void 		t1_dec_sigpass_step_raw(t1_info *t1, grk_flag *flagsp,
												int32_t *datap, int32_t oneplushalf,
												uint32_t vsc, uint32_t ci);
static INLINE void 		t1_dec_sigpass_step_mqc(t1_info *t1, grk_flag *flagsp,
												int32_t *datap, int32_t oneplushalf, uint32_t ci,
												uint32_t flags_stride, uint32_t vsc);
static void 			t1_enc_sigpass(t1_info *t1, int32_t bpno, int32_t *nmsedec,
										uint8_t type, uint32_t cblksty);
static void 			t1_dec_sigpass_raw(t1_info *t1, int32_t bpno, int32_t cblksty);
static void 			t1_enc_refpass(t1_info *t1, int32_t bpno, int32_t *nmsedec,
										uint8_t type);
static void 			t1_dec_refpass_raw(t1_info *t1, int32_t bpno);
static INLINE void 		t1_dec_refpass_step_raw(t1_info *t1, grk_flag *flagsp,
												int32_t *datap, int32_t poshalf, uint32_t ci);
static INLINE void 		t1_dec_refpass_step_mqc(t1_info *t1, grk_flag *flagsp,
												int32_t *datap, int32_t poshalf, uint32_t ci);
static void 			t1_dec_clnpass_step(t1_info *t1, grk_flag *flagsp, int32_t *datap,
											int32_t oneplushalf, uint32_t ci, uint32_t vsc);
static void 			t1_enc_clnpass(t1_info *t1, int32_t bpno, int32_t *nmsedec,
										uint32_t cblksty);
static bool 			t1_code_block_enc_allocate(tcd_cblk_enc_t *p_code_block);
static double 			t1_getwmsedec(int32_t nmsedec, uint32_t compno, uint32_t level,
										uint8_t orient, int32_t bpno,
										uint32_t qmfbid, double stepsize,
										uint32_t numcomps, const double *mct_norms,
										uint32_t mct_numcomps);

static INLINE uint8_t 	t1_getctxno_zc(mqc_t *mqc, uint32_t f) {
	return mqc->lut_ctxno_zc_orient[(f & T1_SIGMA_NEIGHBOURS)];
}

static INLINE uint32_t t1_getctxtno_sc_or_spb_index(uint32_t fX, uint32_t pfX,
		uint32_t nfX, uint32_t ci) {
	/*
	 0 pfX T1_CHI_THIS           T1_LUT_SGN_W
	 1 tfX T1_SIGMA_1            T1_LUT_SIG_N
	 2 nfX T1_CHI_THIS           T1_LUT_SGN_E
	 3 tfX T1_SIGMA_3            T1_LUT_SIG_W
	 4  fX T1_CHI_(THIS - 1)     T1_LUT_SGN_N
	 5 tfX T1_SIGMA_5            T1_LUT_SIG_E
	 6  fX T1_CHI_(THIS + 1)     T1_LUT_SGN_S
	 7 tfX T1_SIGMA_7            T1_LUT_SIG_S
	 */

	ci *= 3;
	uint32_t lu = (fX >> (ci)) & (T1_SIGMA_1 | T1_SIGMA_3 | T1_SIGMA_5 |
	T1_SIGMA_7);

	lu |= (pfX >> (T1_CHI_THIS_I + (ci))) & (1U << 0);
	lu |= (nfX >> (T1_CHI_THIS_I - 2U + (ci))) & (1U << 2);
	if (ci == 0U) {
		lu |= (fX >> (T1_CHI_0_I - 4U)) & (1U << 4);
	} else {
		lu |= (fX >> (T1_CHI_1_I - 4U + ((ci - 3U)))) & (1U << 4);
	}
	lu |= (fX >> (T1_CHI_2_I - 6U + (ci))) & (1U << 6);
	return lu;
}

static INLINE uint8_t t1_getctxno_sc(uint32_t lu) {
	return lut_ctxno_sc[lu];
}

static INLINE uint32_t t1_getctxno_mag(uint32_t f) {
	uint32_t tmp = (f & T1_SIGMA_NEIGHBOURS) ? T1_CTXNO_MAG + 1 : T1_CTXNO_MAG;
	uint32_t tmp2 = (f & T1_MU_0) ? T1_CTXNO_MAG + 2 : tmp;

	return tmp2;
}
static INLINE uint8_t t1_getspb(uint32_t lu) {
	return lut_spb[lu];
}
static int16_t t1_getnmsedec_sig(uint32_t x, uint32_t bitpos) {
	if (bitpos > 0)
		return lut_nmsedec_sig[(x >> (bitpos)) & ((1 << T1_NMSEDEC_BITS) - 1)];

	return lut_nmsedec_sig0[x & ((1 << T1_NMSEDEC_BITS) - 1)];
}
static int16_t t1_getnmsedec_ref(uint32_t x, uint32_t bitpos) {
	if (bitpos > 0)
		return lut_nmsedec_ref[(x >> (bitpos)) & ((1 << T1_NMSEDEC_BITS) - 1)];

	return lut_nmsedec_ref0[x & ((1 << T1_NMSEDEC_BITS) - 1)];
}
#define t1_update_flags_macro(flags, flagsp, ci, s, stride, vsc) \
{ \
    /* east */ \
    flagsp[-1] |= T1_SIGMA_5 << (3U * ci); \
    /* mark target as significant */ \
    flags |= ((s << T1_CHI_1_I) | T1_SIGMA_4) << (3U * ci); \
    /* west */ \
    flagsp[1] |= T1_SIGMA_3 << (3U * ci); \
    /* north-west, north, north-east */ \
    if (ci == 0U && !(vsc)) { \
        grk_flag* north = flagsp - (stride); \
        *north |= (s << T1_CHI_5_I) | T1_SIGMA_16; \
        north[-1] |= T1_SIGMA_17; \
        north[1] |= T1_SIGMA_15; \
    } \
    /* south-west, south, south-east */ \
    if (ci == 3U) { \
        grk_flag* south = flagsp + (stride); \
        *south |= (s << T1_CHI_0_I) | T1_SIGMA_1; \
        south[-1] |= T1_SIGMA_2; \
        south[1] |= T1_SIGMA_0; \
    } \
}

static INLINE void t1_update_flags(grk_flag *flagsp, uint32_t ci, uint32_t s,
									uint32_t stride, uint32_t vsc) {
	t1_update_flags_macro(*flagsp, flagsp, ci, s, stride, vsc);
}

static INLINE void t1_enc_sigpass_step(t1_info *t1, grk_flag *flagsp,
										int32_t *datap, int32_t bpno, int32_t one, int32_t *nmsedec,
										uint8_t type, uint32_t ci, uint32_t vsc) {
	uint32_t v;
	auto mqc = &(t1->mqc);
	uint32_t const flags = *flagsp;

	if ((flags & ((T1_SIGMA_THIS | T1_PI_THIS) << (ci * 3U))) == 0U
			&& (flags & (T1_SIGMA_NEIGHBOURS << (ci * 3U))) != 0U) {
		uint32_t ctxt1 = t1_getctxno_zc(mqc, flags >> (ci * 3U));
		v = (abs(*datap) & one) ? 1 : 0;
		mqc_setcurctx(mqc, ctxt1);
		if (type == T1_TYPE_RAW)
			mqc_bypass_enc(mqc, v);
		else
			mqc_encode(mqc, v);
		if (v) {
			uint32_t lu = t1_getctxtno_sc_or_spb_index(*flagsp, flagsp[-1],	flagsp[1], ci);
			uint32_t ctxt2 = t1_getctxno_sc(lu);
			v = *datap < 0 ? 1U : 0U;
			*nmsedec += t1_getnmsedec_sig((uint32_t) abs(*datap),(uint32_t) bpno);

			mqc_setcurctx(mqc, ctxt2);
			if (type == T1_TYPE_RAW) {
				mqc_bypass_enc(mqc, v);
			} else {
				uint32_t spb = t1_getspb(lu);
				mqc_encode(mqc, v ^ spb);
			}
			t1_update_flags(flagsp, ci, v, t1->w + 2, vsc);
		}
		*flagsp |= T1_PI_THIS << (ci * 3U);
	}
}

static INLINE void t1_dec_sigpass_step_raw(t1_info *t1, grk_flag *flagsp,
		int32_t *datap, int32_t oneplushalf, uint32_t vsc, uint32_t ci) {
	uint32_t v;
	auto mqc = &(t1->mqc);
	uint32_t const flags = *flagsp;

	if ((flags & ((T1_SIGMA_THIS | T1_PI_THIS) << (ci * 3U))) == 0U
			&& (flags & (T1_SIGMA_NEIGHBOURS << (ci * 3U))) != 0U) {
		if (mqc_raw_decode(mqc)) {
			v = mqc_raw_decode(mqc);
			*datap = v ? -oneplushalf : oneplushalf;
			t1_update_flags(flagsp, ci, v, t1->w + 2, vsc);
		}
		*flagsp |= T1_PI_THIS << (ci * 3U);
	}
}

#define t1_dec_sigpass_step_mqc_macro(flags, flagsp, flags_stride, data, \
                                          data_stride, ci, mqc, curctx, \
                                          v, a, c, ct, oneplushalf, vsc) \
{ \
    if ((flags & ((T1_SIGMA_THIS | T1_PI_THIS) << (ci * 3U))) == 0U && \
        (flags & (T1_SIGMA_NEIGHBOURS << (ci * 3U))) != 0U) { \
        uint32_t ctxt1 = t1_getctxno_zc(mqc, flags >> (ci * 3U)); \
        t1_setcurctx(curctx, ctxt1); \
        mqc_decode_macro(v, mqc, curctx, a, c, ct); \
        if (v) { \
            uint32_t lu = t1_getctxtno_sc_or_spb_index( \
                                flags, \
                                flagsp[-1], flagsp[1], \
                                ci); \
            uint32_t ctxt2 = t1_getctxno_sc(lu); \
            uint32_t spb = t1_getspb(lu); \
            t1_setcurctx(curctx, ctxt2); \
            mqc_decode_macro(v, mqc, curctx, a, c, ct); \
            v = v ^ spb; \
            data[ci*data_stride] = v ? -oneplushalf : oneplushalf; \
            t1_update_flags_macro(flags, flagsp, ci, v, flags_stride, vsc); \
        } \
        flags |= T1_PI_THIS << (ci * 3U); \
    } \
}

static INLINE void t1_dec_sigpass_step_mqc(t1_info *t1, grk_flag *flagsp,
											int32_t *datap, int32_t oneplushalf,
											uint32_t ci, uint32_t flags_stride,
											uint32_t vsc) {
	uint32_t v;
	auto mqc = &(t1->mqc);

	t1_dec_sigpass_step_mqc_macro(*flagsp, flagsp, flags_stride, datap, 0, ci,
			mqc, mqc->curctx, v, mqc->a, mqc->c, mqc->ct, oneplushalf, vsc);
}

static void t1_enc_sigpass(t1_info *t1, int32_t bpno, int32_t *nmsedec,
							uint8_t type, uint32_t cblksty) {
	uint32_t i, k;
	int32_t const one = 1 << (bpno + T1_NMSEDEC_FRACBITS);
	auto f = &T1_FLAGS(0, 0);
	uint32_t const extra = 2;
	*nmsedec = 0;

	for (k = 0; k < (t1->h & ~3U); k += 4) {
		for (i = 0; i < t1->w; ++i) {
			if (*f == 0U) {
				/* Nothing to do for any of the 4 data points */
				f++;
				continue;
			}
			t1_enc_sigpass_step(t1, f,
					&t1->data[((k + 0) * t1->data_stride) + i], bpno, one,
					nmsedec, type, 0, cblksty & J2K_CCP_CBLKSTY_VSC);
			t1_enc_sigpass_step(t1, f,
					&t1->data[((k + 1) * t1->data_stride) + i], bpno, one,
					nmsedec, type, 1, 0);
			t1_enc_sigpass_step(t1, f,
					&t1->data[((k + 2) * t1->data_stride) + i], bpno, one,
					nmsedec, type, 2, 0);
			t1_enc_sigpass_step(t1, f,
					&t1->data[((k + 3) * t1->data_stride) + i], bpno, one,
					nmsedec, type, 3, 0);
			++f;
		}
		f += extra;
	}

	if (k < t1->h) {
		for (i = 0; i < t1->w; ++i) {
			if (*f == 0U) {
				/* Nothing to do for any of the 4 data points */
				f++;
				continue;
			}
			uint32_t j;
			int32_t* pdata = t1->data + k* t1->data_stride + i;
			for (j = k;	j < t1->h; 	++j) {
				t1_enc_sigpass_step(t1, f, pdata,
						bpno, one, nmsedec, type, j - k,
						(j == k && (cblksty & J2K_CCP_CBLKSTY_VSC) != 0));
				pdata += t1->data_stride;
			}
			++f;
		}
	}
}

static void t1_dec_sigpass_raw(t1_info *t1, int32_t bpno, int32_t cblksty) {
	int32_t one, half, oneplushalf;
	uint32_t i, j, k;
	auto data = t1->data;
	auto flagsp = &T1_FLAGS(0, 0);
	const uint32_t l_w = t1->w;

	one = 1 << bpno;
	half = one >> 1;
	oneplushalf = one | half;

	for (k = 0; k < (t1->h & ~3U); k += 4, flagsp += 2, data += 3 * l_w) {
		for (i = 0; i < l_w; ++i, ++flagsp, ++data) {
			grk_flag flags = *flagsp;
			if (flags != 0) {
				t1_dec_sigpass_step_raw(t1, flagsp, data, oneplushalf,
						cblksty & J2K_CCP_CBLKSTY_VSC, /* vsc */
						0U);
				t1_dec_sigpass_step_raw(t1, flagsp, data + l_w, oneplushalf,
						false, /* vsc */
						1U);
				t1_dec_sigpass_step_raw(t1, flagsp, data + 2 * l_w, oneplushalf,
						false, /* vsc */
						2U);
				t1_dec_sigpass_step_raw(t1, flagsp, data + 3 * l_w, oneplushalf,
						false, /* vsc */
						3U);
			}
		}
	}
	if (k < t1->h) {
		for (i = 0; i < l_w; ++i, ++flagsp, ++data) {
			for (j = 0; j < t1->h - k; ++j) {
				t1_dec_sigpass_step_raw(t1, flagsp, data + j * l_w, oneplushalf,
						cblksty & J2K_CCP_CBLKSTY_VSC, /* vsc */
						j);
			}
		}
	}
}

#define t1_dec_sigpass_mqc_internal(t1, bpno, vsc, w, h, flags_stride) \
{ \
        int32_t one, half, oneplushalf; \
        uint32_t i, j, k; \
        auto data = t1->data; \
        auto flagsp = &t1->flags[(flags_stride) + 1]; \
        const uint32_t l_w = w; \
        auto mqc = &(t1->mqc); \
  \
        DOWNLOAD_MQC_VARIABLES(mqc, curctx, c, a, ct); \
        uint32_t v; \
        one = 1 << bpno; \
        half = one >> 1; \
        oneplushalf = one | half; \
        for (k = 0; k < (h & ~3u); k += 4, data += 3*l_w, flagsp += 2) { \
                for (i = 0; i < l_w; ++i, ++data, ++flagsp) { \
                        grk_flag flags = *flagsp; \
                        if( flags != 0 ) { \
                            t1_dec_sigpass_step_mqc_macro( \
                                flags, flagsp, flags_stride, data, \
                                l_w, 0, mqc, curctx, v, a, c, ct, oneplushalf, vsc); \
                            t1_dec_sigpass_step_mqc_macro( \
                                flags, flagsp, flags_stride, data, \
                                l_w, 1, mqc, curctx, v, a, c, ct, oneplushalf, false); \
                            t1_dec_sigpass_step_mqc_macro( \
                                flags, flagsp, flags_stride, data, \
                                l_w, 2, mqc, curctx, v, a, c, ct, oneplushalf, false); \
                            t1_dec_sigpass_step_mqc_macro( \
                                flags, flagsp, flags_stride, data, \
                                l_w, 3, mqc, curctx, v, a, c, ct, oneplushalf, false); \
                            *flagsp = flags; \
                        } \
                } \
        } \
        UPLOAD_MQC_VARIABLES(mqc, curctx, c, a, ct); \
        if( k < h ) { \
            for (i = 0; i < l_w; ++i, ++data, ++flagsp) { \
                for (j = 0; j < h - k; ++j) { \
                        t1_dec_sigpass_step_mqc(t1, flagsp, \
                            data + j * l_w, oneplushalf, j, flags_stride, vsc); \
                } \
            } \
        } \
}

static void t1_dec_sigpass_mqc(t1_info *t1, int32_t bpno, int32_t cblksty) {
	if (t1->w == 64 && t1->h == 64) {
		if (cblksty & J2K_CCP_CBLKSTY_VSC){
			t1_dec_sigpass_mqc_internal(t1, bpno, true, 64, 64, 66);
		}
		else {
			t1_dec_sigpass_mqc_internal(t1, bpno, false, 64, 64, 66);
		}
	} else {
		if (cblksty & J2K_CCP_CBLKSTY_VSC) {
			t1_dec_sigpass_mqc_internal(t1, bpno, true, t1->w, t1->h, t1->w + 2U);
		} else {
			t1_dec_sigpass_mqc_internal(t1, bpno, false, t1->w, t1->h, t1->w + 2U);
		}
	}
}

static INLINE void t1_enc_refpass_step(t1_info *t1, grk_flag *flagsp,
										int32_t *datap, int32_t bpno, int32_t one, int32_t *nmsedec,
										uint8_t type, uint32_t ci) {
	uint32_t v;
	auto mqc = &(t1->mqc);
	uint32_t const shift_flags = (*flagsp >> (ci * 3U));

	if ((shift_flags & (T1_SIGMA_THIS | T1_PI_THIS)) == T1_SIGMA_THIS) {
		uint32_t ctxt = t1_getctxno_mag(shift_flags);
		*nmsedec += t1_getnmsedec_ref((uint32_t) abs(*datap), (uint32_t) bpno);
		v = (abs(*datap) & one) ? 1 : 0;
		mqc_setcurctx(mqc, ctxt);
		if (type == T1_TYPE_RAW) { /* BYPASS/LAZY MODE */
			mqc_bypass_enc(mqc, v);
		} else {
			mqc_encode(mqc, v);
		}
		*flagsp |= T1_MU_THIS << (ci * 3U);
	}
}

static INLINE void t1_dec_refpass_step_raw(t1_info *t1, grk_flag *flagsp,
		int32_t *datap, int32_t poshalf, uint32_t ci) {
	auto mqc = &(t1->mqc);

	if ((*flagsp & ((T1_SIGMA_THIS | T1_PI_THIS) << (ci * 3U)))
			== (T1_SIGMA_THIS << (ci * 3U))) {
		uint32_t v = mqc_raw_decode(mqc);
		*datap += (v ^ (*datap < 0)) ? poshalf : -poshalf;
		*flagsp |= T1_MU_THIS << (ci * 3U);
	}
}

#define t1_dec_refpass_step_mqc_macro(flags, data, data_stride, ci, \
                                          mqc, curctx, v, a, c, ct, poshalf) \
{ \
    if ((flags & ((T1_SIGMA_THIS | T1_PI_THIS) << (ci * 3U))) == \
            (T1_SIGMA_THIS << (ci * 3U))) { \
        uint32_t ctxt = t1_getctxno_mag(flags >> (ci * 3U)); \
        t1_setcurctx(curctx, ctxt); \
        mqc_decode_macro(v, mqc, curctx, a, c, ct); \
        data[ci*data_stride] += (v ^ (data[ci*data_stride] < 0)) ? poshalf : -poshalf; \
        flags |= T1_MU_THIS << (ci * 3U); \
    } \
}

static INLINE void t1_dec_refpass_step_mqc(t1_info *t1, grk_flag *flagsp,
											int32_t *datap, int32_t poshalf, uint32_t ci) {
	uint32_t v;
	auto mqc = &(t1->mqc);
	t1_dec_refpass_step_mqc_macro(*flagsp, datap, 0, ci, mqc, mqc->curctx, v,
			mqc->a, mqc->c, mqc->ct, poshalf);
}

static void t1_enc_refpass(t1_info *t1, int32_t bpno, int32_t *nmsedec,
		uint8_t type) {
	uint32_t i, k;
	const int32_t one = 1 << (bpno + T1_NMSEDEC_FRACBITS);
	auto f = &T1_FLAGS(0, 0);
	const uint32_t extra = 2U;

	*nmsedec = 0;
	for (k = 0; k < (t1->h & ~3U); k += 4) {
		for (i = 0; i < t1->w; ++i) {
			if ((*f & (T1_SIGMA_4 | T1_SIGMA_7 | T1_SIGMA_10 | T1_SIGMA_13))
					== 0) {
				/* none significant */
				f++;
				continue;
			}
			if ((*f & (T1_PI_0 | T1_PI_1 | T1_PI_2 | T1_PI_3))
					== (T1_PI_0 | T1_PI_1 | T1_PI_2 | T1_PI_3)) {
				/* all processed by sigpass */
				f++;
				continue;
			}

			t1_enc_refpass_step(t1, f,
					&t1->data[((k + 0) * t1->data_stride) + i], bpno, one,
					nmsedec, type, 0);
			t1_enc_refpass_step(t1, f,
					&t1->data[((k + 1) * t1->data_stride) + i], bpno, one,
					nmsedec, type, 1);
			t1_enc_refpass_step(t1, f,
					&t1->data[((k + 2) * t1->data_stride) + i], bpno, one,
					nmsedec, type, 2);
			t1_enc_refpass_step(t1, f,
					&t1->data[((k + 3) * t1->data_stride) + i], bpno, one,
					nmsedec, type, 3);
			++f;
		}
		f += extra;
	}

	if (k < t1->h) {
		uint32_t j;
		for (i = 0; i < t1->w; ++i) {
			if ((*f & (T1_SIGMA_4 | T1_SIGMA_7 | T1_SIGMA_10 | T1_SIGMA_13))
					== 0) {
				/* none significant */
				f++;
				continue;
			}
			for (j = k; j < t1->h; ++j) {
				t1_enc_refpass_step(t1, f, &t1->data[(j * t1->data_stride) + i],
						bpno, one, nmsedec, type, j - k);
			}
			++f;
		}
	}
}

static void t1_dec_refpass_raw(t1_info *t1, int32_t bpno) {
	int32_t one, poshalf;
	uint32_t i, j, k;
	auto data = t1->data;
	auto flagsp = &T1_FLAGS(0, 0);
	const uint32_t l_w = t1->w;

	one = 1 << bpno;
	poshalf = one >> 1;
	for (k = 0; k < (t1->h & ~3U); k += 4, flagsp += 2, data += 3 * l_w) {
		for (i = 0; i < l_w; ++i, ++flagsp, ++data) {
			grk_flag flags = *flagsp;
			if (flags != 0) {
				t1_dec_refpass_step_raw(t1, flagsp, data, poshalf, 0U);
				t1_dec_refpass_step_raw(t1, flagsp, data + l_w, poshalf, 1U);
				t1_dec_refpass_step_raw(t1, flagsp, data + 2 * l_w, poshalf,
						2U);
				t1_dec_refpass_step_raw(t1, flagsp, data + 3 * l_w, poshalf,
						3U);
			}
		}
	}
	if (k < t1->h) {
		for (i = 0; i < l_w; ++i, ++flagsp, ++data) {
			for (j = 0; j < t1->h - k; ++j) {
				t1_dec_refpass_step_raw(t1, flagsp, data + j * l_w, poshalf, j);
			}
		}
	}
}

#define t1_dec_refpass_mqc_internal(t1, bpno, w, h, flags_stride) \
{ \
        int32_t one, poshalf; \
        uint32_t i, j, k; \
        auto data = t1->data; \
        auto flagsp = &t1->flags[flags_stride + 1]; \
        const uint32_t l_w = w; \
        auto mqc = &(t1->mqc); \
 \
        DOWNLOAD_MQC_VARIABLES(mqc, curctx, c, a, ct); \
        uint32_t v; \
        one = 1 << bpno; \
        poshalf = one >> 1; \
        for (k = 0; k < (h & ~3u); k += 4, data += 3*l_w, flagsp += 2) { \
                for (i = 0; i < l_w; ++i, ++data, ++flagsp) { \
                        grk_flag flags = *flagsp; \
                        if( flags != 0 ) { \
                            t1_dec_refpass_step_mqc_macro( \
                                flags, data, l_w, 0, \
                                mqc, curctx, v, a, c, ct, poshalf); \
                            t1_dec_refpass_step_mqc_macro( \
                                flags, data, l_w, 1, \
                                mqc, curctx, v, a, c, ct, poshalf); \
                            t1_dec_refpass_step_mqc_macro( \
                                flags, data, l_w, 2, \
                                mqc, curctx, v, a, c, ct, poshalf); \
                            t1_dec_refpass_step_mqc_macro( \
                                flags, data, l_w, 3, \
                                mqc, curctx, v, a, c, ct, poshalf); \
                            *flagsp = flags; \
                        } \
                } \
        } \
        UPLOAD_MQC_VARIABLES(mqc, curctx, c, a, ct); \
        if( k < h ) { \
            for (i = 0; i < l_w; ++i, ++data, ++flagsp) { \
                for (j = 0; j < h - k; ++j) { \
                        t1_dec_refpass_step_mqc(t1, flagsp, data + j * l_w, poshalf, j); \
                } \
            } \
        } \
}

static void t1_dec_refpass_mqc(t1_info *t1, int32_t bpno) {
	if (t1->w == 64 && t1->h == 64) {
		t1_dec_refpass_mqc_internal(t1, bpno, 64, 64, 66);
	} else {
		t1_dec_refpass_mqc_internal(t1, bpno, t1->w, t1->h, t1->w + 2U);
	}
}

static void t1_enc_clnpass_step(t1_info *t1, grk_flag *flagsp, int32_t *datap,
		int32_t bpno, int32_t one, int32_t *nmsedec, uint32_t agg,
		uint32_t runlen, uint32_t lim, uint32_t cblksty) {
	uint32_t v;
	uint32_t ci;
	auto mqc = &(t1->mqc);

	const uint32_t check = (T1_SIGMA_4 | T1_SIGMA_7 | T1_SIGMA_10 | T1_SIGMA_13
			|
			T1_PI_0 | T1_PI_1 | T1_PI_2 | T1_PI_3);

	if ((*flagsp & check) == check) {
		if (runlen == 0) {
			*flagsp &= ~(T1_PI_0 | T1_PI_1 | T1_PI_2 | T1_PI_3);
		} else if (runlen == 1) {
			*flagsp &= ~(T1_PI_1 | T1_PI_2 | T1_PI_3);
		} else if (runlen == 2) {
			*flagsp &= ~(T1_PI_2 | T1_PI_3);
		} else if (runlen == 3) {
			*flagsp &= ~(T1_PI_3);
		}
		return;
	}

	for (ci = runlen; ci < lim; ++ci) {
		uint32_t vsc;
		grk_flag flags;
		uint32_t ctxt1;

		flags = *flagsp;

		if ((agg != 0) && (ci == runlen)) {
			goto LABEL_PARTIAL;
		}

		if (!(flags & ((T1_SIGMA_THIS | T1_PI_THIS) << (ci * 3U)))) {
			ctxt1 = t1_getctxno_zc(mqc, flags >> (ci * 3U));
			mqc_setcurctx(mqc, ctxt1);
			v = (abs(*datap) & one) ? 1 : 0;
			mqc_encode(mqc, v);
			if (v) {
				uint32_t ctxt2, spb;
				uint32_t lu;
				LABEL_PARTIAL: lu = t1_getctxtno_sc_or_spb_index(*flagsp,
						flagsp[-1], flagsp[1], ci);
				*nmsedec += t1_getnmsedec_sig((uint32_t) abs(*datap),
						(uint32_t) bpno);
				ctxt2 = t1_getctxno_sc(lu);
				mqc_setcurctx(mqc, ctxt2);
				v = *datap < 0 ? 1U : 0U;
				spb = t1_getspb(lu);
				mqc_encode(mqc, v ^ spb);
				vsc = ((cblksty & J2K_CCP_CBLKSTY_VSC) && (ci == 0)) ? 1 : 0;
				t1_update_flags(flagsp, ci, v, t1->w + 2U, vsc);
			}
		}
		*flagsp &= ~(T1_PI_THIS << (3U * ci));
		datap += t1->data_stride;
	}
}

#define t1_dec_clnpass_step_macro(check_flags, partial, \
                                      flags, flagsp, flags_stride, data, \
                                      data_stride, ci, mqc, curctx, \
                                      v, a, c, ct, oneplushalf, vsc) \
{ \
    if ( !check_flags || !(flags & ((T1_SIGMA_THIS | T1_PI_THIS) << (ci * 3U)))) {\
        do { \
            if( !partial ) { \
                uint32_t ctxt1 = t1_getctxno_zc(mqc, flags >> (ci * 3U)); \
                t1_setcurctx(curctx, ctxt1); \
                mqc_decode_macro(v, mqc, curctx, a, c, ct); \
                if( !v ) \
                    break; \
            } \
            { \
                uint32_t lu = t1_getctxtno_sc_or_spb_index( \
                                    flags, flagsp[-1], flagsp[1], \
                                    ci); \
                t1_setcurctx(curctx, t1_getctxno_sc(lu)); \
                mqc_decode_macro(v, mqc, curctx, a, c, ct); \
                v = v ^ t1_getspb(lu); \
                data[ci*data_stride] = v ? -oneplushalf : oneplushalf; \
                t1_update_flags_macro(flags, flagsp, ci, v, flags_stride, vsc); \
            } \
        } while(0); \
    } \
}

static void t1_dec_clnpass_step(t1_info *t1, grk_flag *flagsp, int32_t *datap,
		int32_t oneplushalf, uint32_t ci, uint32_t vsc) {
	uint32_t v;
	auto mqc = &(t1->mqc);

	t1_dec_clnpass_step_macro(true, false, *flagsp, flagsp, t1->w + 2U, datap,
			0, ci, mqc, mqc->curctx, v, mqc->a, mqc->c, mqc->ct, oneplushalf,
			vsc);
}

static void t1_enc_clnpass(t1_info *t1, int32_t bpno, int32_t *nmsedec,	uint32_t cblksty) {
	uint32_t i, k;
	const int32_t one = 1 << (bpno + T1_NMSEDEC_FRACBITS);
	uint32_t agg, runlen;
	auto mqc = &(t1->mqc);
	*nmsedec = 0;

	for (k = 0; k < (t1->h & ~3U); k += 4) {
		for (i = 0; i < t1->w; ++i) {
			agg = !(T1_FLAGS(i, k));
			if (agg) {
				for (runlen = 0; runlen < 4; ++runlen) {
					if (abs(t1->data[((k + runlen) * t1->data_stride) + i])
							& one) {
						break;
					}
				}
				mqc_setcurctx(mqc, T1_CTXNO_AGG);
				mqc_encode(mqc, runlen != 4);
				if (runlen == 4) {
					continue;
				}
				mqc_setcurctx(mqc, T1_CTXNO_UNI);
				mqc_encode(mqc, runlen >> 1);
				mqc_encode(mqc, runlen & 1);
			} else {
				runlen = 0;
			}
			t1_enc_clnpass_step(t1, &T1_FLAGS(i, k),
					&t1->data[((k + runlen) * t1->data_stride) + i], bpno, one,
					nmsedec, agg, runlen, 4U, cblksty);
		}
	}
	if (k < t1->h) {
		agg = 0;
		runlen = 0;
		for (i = 0; i < t1->w; ++i) {
			t1_enc_clnpass_step(t1, &T1_FLAGS(i, k),
					&t1->data[((k + runlen) * t1->data_stride) + i], bpno, one,
					nmsedec, agg, runlen, t1->h - k, cblksty);
		}
	}
}

#define t1_dec_clnpass_internal(t1, bpno, vsc, w, h, flags_stride) \
{ \
    int32_t one, half, oneplushalf; \
    uint32_t runlen; \
    uint32_t i, j, k; \
    const uint32_t l_w = w; \
    auto mqc = &(t1->mqc); \
    auto data = t1->data; \
    auto flagsp = &t1->flags[flags_stride + 1]; \
 \
    DOWNLOAD_MQC_VARIABLES(mqc, curctx, c, a, ct); \
    uint32_t v; \
    one = 1 << bpno; \
    half = one >> 1; \
    oneplushalf = one | half; \
    for (k = 0; k < (h & ~3u); k += 4, data += 3*l_w, flagsp += 2) { \
        for (i = 0; i < l_w; ++i, ++data, ++flagsp) { \
            grk_flag flags = *flagsp; \
            if (flags == 0) { \
                uint32_t partial = true; \
                t1_setcurctx(curctx, T1_CTXNO_AGG); \
                mqc_decode_macro(v, mqc, curctx, a, c, ct); \
                if (!v) \
                    continue; \
                t1_setcurctx(curctx, T1_CTXNO_UNI); \
                mqc_decode_macro(runlen, mqc, curctx, a, c, ct); \
                mqc_decode_macro(v, mqc, curctx, a, c, ct); \
                runlen = (runlen << 1) | v; \
                switch(runlen) { \
                    case 0: \
                        t1_dec_clnpass_step_macro(false, true,\
                                            flags, flagsp, flags_stride, data, \
                                            l_w, 0, mqc, curctx, \
                                            v, a, c, ct, oneplushalf, vsc); \
                        partial = false; \
                        /* FALLTHRU */ \
                    case 1: \
                        t1_dec_clnpass_step_macro(false, partial,\
                                            flags, flagsp, flags_stride, data, \
                                            l_w, 1, mqc, curctx, \
                                            v, a, c, ct, oneplushalf, false); \
                        partial = false; \
                        /* FALLTHRU */ \
                    case 2: \
                        t1_dec_clnpass_step_macro(false, partial,\
                                            flags, flagsp, flags_stride, data, \
                                            l_w, 2, mqc, curctx, \
                                            v, a, c, ct, oneplushalf, false); \
                        partial = false; \
                        /* FALLTHRU */ \
                    case 3: \
                        t1_dec_clnpass_step_macro(false, partial,\
                                            flags, flagsp, flags_stride, data, \
                                            l_w, 3, mqc, curctx, \
                                            v, a, c, ct, oneplushalf, false); \
                        break; \
                } \
            } else { \
                t1_dec_clnpass_step_macro(true, false, \
                                    flags, flagsp, flags_stride, data, \
                                    l_w, 0, mqc, curctx, \
                                    v, a, c, ct, oneplushalf, vsc); \
                t1_dec_clnpass_step_macro(true, false, \
                                    flags, flagsp, flags_stride, data, \
                                    l_w, 1, mqc, curctx, \
                                    v, a, c, ct, oneplushalf, false); \
                t1_dec_clnpass_step_macro(true, false, \
                                    flags, flagsp, flags_stride, data, \
                                    l_w, 2, mqc, curctx, \
                                    v, a, c, ct, oneplushalf, false); \
                t1_dec_clnpass_step_macro(true, false, \
                                    flags, flagsp, flags_stride, data, \
                                    l_w, 3, mqc, curctx, \
                                    v, a, c, ct, oneplushalf, false); \
            } \
            *flagsp = flags & ~(T1_PI_0 | T1_PI_1 | T1_PI_2 | T1_PI_3); \
        } \
    } \
    UPLOAD_MQC_VARIABLES(mqc, curctx, c, a, ct); \
    if( k < h ) { \
        for (i = 0; i < l_w; ++i, ++flagsp, ++data) { \
            for (j = 0; j < h - k; ++j) \
                t1_dec_clnpass_step(t1, flagsp, data + j * l_w, oneplushalf, j, vsc); \
            *flagsp &= ~(T1_PI_0 | T1_PI_1 | T1_PI_2 | T1_PI_3); \
        } \
    } \
}

static void t1_dec_clnpass_check_segsym(t1_info *t1, int32_t cblksty) {
	if (cblksty & J2K_CCP_CBLKSTY_SEGSYM) {
		auto mqc = &(t1->mqc);
		uint32_t v, v2;

		mqc_setcurctx(mqc, T1_CTXNO_UNI);
		mqc_decode(v, mqc);
		mqc_decode(v2, mqc);
		v = (v << 1) | v2;
		mqc_decode(v2, mqc);
		v = (v << 1) | v2;
		mqc_decode(v2, mqc);
		v = (v << 1) | v2;
		if (v!=0xa) {
		 GROK_WARN("Bad segmentation symbol %x", v);
		}
	}
}

template <uint32_t w, uint32_t h, bool vsc> void t1_dec_clnpass(t1_info *t1, int32_t bpno) {
	t1_dec_clnpass_internal(t1, bpno, vsc, w, h, w+2);
}

static void t1_dec_clnpass(t1_info *t1, int32_t bpno, int32_t cblksty) {
	if (t1->w == 64 && t1->h == 64) {
		if (cblksty & J2K_CCP_CBLKSTY_VSC)
			t1_dec_clnpass<64,64,true>(t1, bpno);
		else
			t1_dec_clnpass<64,64,false>(t1, bpno);
	} else {
		if (cblksty & J2K_CCP_CBLKSTY_VSC){
			t1_dec_clnpass_internal(t1, bpno, true,t1->w,t1->h,t1->w+2);
		}
		else {
			t1_dec_clnpass_internal(t1, bpno, false,t1->w,t1->h,t1->w+2);
		}
	}
	t1_dec_clnpass_check_segsym(t1, cblksty);
}

static double t1_getwmsedec(int32_t nmsedec,
							uint32_t compno, uint32_t level,
							uint8_t orient, int32_t bpno,
							uint32_t qmfbid, double stepsize,
							uint32_t numcomps, const double *mct_norms,
							uint32_t mct_numcomps) {
	double w1 = 1, w2, wmsedec;
	(void) (numcomps);

	if (mct_norms && (compno < mct_numcomps))
		w1 = mct_norms[compno];

	if (qmfbid == 1)
		w2 = grk::dwt_utils::getnorm(level, orient);
	else /* if (qmfbid == 0) */
		w2 = grk::dwt_utils::getnorm_real(level, orient);

	wmsedec = w1 * w2 * stepsize * (1 << bpno);
	wmsedec *= wmsedec * nmsedec / 8192.0;

	return wmsedec;
}

bool t1_allocate_buffers(t1_info *t1, uint32_t w, uint32_t h) {
	uint32_t flagssize;
	uint32_t flags_stride;

	/* No risk of overflow. Prior checks ensure those assert are met */
	/* They are per the specification */
	assert(w <= 1024);
	assert(h <= 1024);
	assert(w * h <= 4096);

	/* encoder uses tile buffer, so no need to allocate */
	uint32_t datasize = w * h;

	if (datasize > t1->datasize) {
		grk::grk_aligned_free(t1->data);
		t1->data =
				(int32_t*) grk::grk_aligned_malloc(datasize * sizeof(int32_t));
		if (!t1->data) {
			GROK_ERROR("Out of memory");
			return false;
		}
		t1->datasize = datasize;
	}
	/* memset first arg is declared to never be null by gcc */
	if (t1->data && !t1->encoder)
		memset(t1->data, 0, datasize * sizeof(int32_t));

	flags_stride = w + 2U; /* can't be 0U */
	flagssize = (h + 3U) / 4U + 2U;
	flagssize *= flags_stride;
	uint32_t x;
	uint32_t flags_height = (h + 3U) / 4U;

	if (flagssize > t1->flagssize) {

		grk::grk_aligned_free(t1->flags);
		t1->flags = (grk_flag*) grk::grk_aligned_malloc(
				flagssize * sizeof(grk_flag));
		if (!t1->flags) {
			/* FIXME event manager error callback */
			return false;
		}
	}
	t1->flagssize = flagssize;

	memset(t1->flags, 0, flagssize * sizeof(grk_flag));
	auto p = &t1->flags[0];
	for (x = 0; x < flags_stride; ++x) {
		/* magic value to hopefully stop any passes being interested in this entry */
		*p++ = (T1_PI_0 | T1_PI_1 | T1_PI_2 | T1_PI_3);
	}
	p = &t1->flags[((flags_height + 1) * flags_stride)];
	for (x = 0; x < flags_stride; ++x) {
		/* magic value to hopefully stop any passes being interested in this entry */
		*p++ = (T1_PI_0 | T1_PI_1 | T1_PI_2 | T1_PI_3);
	}
	if (h % 4) {
		uint32_t v = 0;
		p = &t1->flags[((flags_height) * flags_stride)];
		if ((h&3) == 1) {
			v |= T1_PI_1 | T1_PI_2 | T1_PI_3;
		} else if ((h&3) == 2) {
			v |= T1_PI_2 | T1_PI_3;
		} else if ((h&3) == 3) {
			v |= T1_PI_3;
		}
		for (x = 0; x < flags_stride; ++x) {
			*p++ = v;
		}
	}

	t1->w = w;
	t1->h = h;

	return true;
}
t1_info* t1_create(bool isEncoder) {
	t1_info *t1 = (t1_info*) grk::grk_calloc(1, sizeof(t1_info));
	if (!t1){
		GROK_ERROR("Out of memory");
		return nullptr;
	}
	t1->encoder = isEncoder;

	return t1;
}
void t1_destroy(t1_info *p_t1) {
	if (!p_t1)
		return;

	if (p_t1->data) {
		grk::grk_aligned_free(p_t1->data);
		p_t1->data = 00;
	}

	if (p_t1->flags) {
		grk::grk_aligned_free(p_t1->flags);
		p_t1->flags = 00;
	}

	grk::grok_free(p_t1->cblkdatabuffer);
	grk::grok_free(p_t1);
}

bool t1_decode_cblk(t1_info *t1, tcd_cblk_dec_t *cblk, uint32_t orient,
		uint32_t roishift, uint32_t cblksty, bool check_pterm) {
	auto mqc = &(t1->mqc);
	int32_t bpno_plus_one;
	uint32_t passtype;
	uint32_t segno, passno;
	uint8_t *cblkdata = nullptr;
	uint32_t cblkdataindex = 0;
	uint8_t type = T1_TYPE_MQ;

	mqc->lut_ctxno_zc_orient = lut_ctxno_zc + (orient << 9);

	if (!t1_allocate_buffers(t1, (uint32_t) (cblk->x1 - cblk->x0),
							(uint32_t) (cblk->y1 - cblk->y0)))
		return false;


	bpno_plus_one = (int32_t) (roishift + cblk->numbps);
	if (bpno_plus_one >= 31) {
		grk::GROK_ERROR("unsupported bpno_plus_one = %d >= 31\n",
				bpno_plus_one);
		return false;
	}
	passtype = 2;

	mqc_resetstates(mqc);
	cblkdata = cblk->chunks[0].data;

	for (segno = 0; segno < cblk->real_num_segs; ++segno) {
		auto seg = cblk->segs + segno;

		/* BYPASS mode */
		type = ((bpno_plus_one <= ((int32_t) (cblk->numbps)) - 4)
				&& (passtype < 2) && (cblksty & J2K_CCP_CBLKSTY_LAZY)) ?
				T1_TYPE_RAW : T1_TYPE_MQ;

		if (type == T1_TYPE_RAW) {
			mqc_raw_init_dec(mqc, cblkdata + cblkdataindex, seg->len,
			GRK_FAKE_MARKER_BYTES);
		} else {
			mqc_init_dec(mqc, cblkdata + cblkdataindex, seg->len,
			GRK_FAKE_MARKER_BYTES);
		}
		cblkdataindex += seg->len;

		for (passno = 0;
				(passno < seg->real_num_passes) && (bpno_plus_one >= 1);
				++passno) {
			switch (passtype) {
			case 0:
				if (type == T1_TYPE_RAW)
					t1_dec_sigpass_raw(t1, bpno_plus_one, (int32_t) cblksty);
				else
					t1_dec_sigpass_mqc(t1, bpno_plus_one, (int32_t) cblksty);
				break;
			case 1:
				if (type == T1_TYPE_RAW)
					t1_dec_refpass_raw(t1, bpno_plus_one);
				else
					t1_dec_refpass_mqc(t1, bpno_plus_one);
				break;
			case 2:
				t1_dec_clnpass(t1, bpno_plus_one, (int32_t) cblksty);
				break;
			}

			if ((cblksty & J2K_CCP_CBLKSTY_RESET) && type == T1_TYPE_MQ)
				mqc_resetstates(mqc);
			if (++passtype == 3) {
				passtype = 0;
				bpno_plus_one--;
			}
		}

		opq_mqc_finish_dec(mqc);
	}

	if (check_pterm) {
		if (mqc->bp + 2 < mqc->end) {
			grk::GROK_WARN(
					"PTERM check failure: %d remaining bytes in code block (%d used / %d)\n",
					(int) (mqc->end - mqc->bp) - 2,
					(int) (mqc->bp - mqc->start),
					(int) (mqc->end - mqc->start));
		} else if (mqc->end_of_byte_stream_counter > 2) {
			grk::GROK_WARN(
					"PTERM check failure: %d synthetized 0xFF markers read\n",
					mqc->end_of_byte_stream_counter);
		}
	}

	return true;
}
static int t1_enc_is_term_pass(tcd_cblk_enc_t *cblk, uint32_t cblksty,
								int32_t bpno, uint32_t passtype) {
	/* Is it the last cleanup pass ? */
	if (passtype == 2 && bpno == 0)
		return true;

	if (cblksty & J2K_CCP_CBLKSTY_TERMALL)
		return true;

	if ((cblksty & J2K_CCP_CBLKSTY_LAZY)) {
		/* For bypass arithmetic bypass, terminate the 4th cleanup pass */
		if ((bpno == ((int32_t) cblk->numbps - 4)) && (passtype == 2))
			return true;
		/* and beyond terminate all the magnitude refinement passes (in raw) */
		/* and cleanup passes (in MQC) */
		if ((bpno < ((int32_t) (cblk->numbps) - 4)) && (passtype > 0))
			return true;
	}

	return false;
}

/**
 * Deallocates the encoding data of the given precinct.
 */
void t1_code_block_enc_deallocate(tcd_cblk_enc_t *l_code_block) {
	if (l_code_block->layers) {
		grk::grok_free(l_code_block->layers);
		l_code_block->layers = 00;
	}
	if (l_code_block->passes) {
		grk::grok_free(l_code_block->passes);
		l_code_block->passes = 00;
	}
}
static bool t1_code_block_enc_allocate(tcd_cblk_enc_t *p_code_block) {
	if (!p_code_block->layers) {
		p_code_block->layers = (tcd_layer_t*) grk::grk_calloc(100,
				sizeof(tcd_layer_t));
		if (!p_code_block->layers)
			return false;
	}
	if (!p_code_block->passes) {
		p_code_block->passes = (tcd_pass_t*) grk::grk_calloc(100,
				sizeof(tcd_pass_t));
		if (!p_code_block->passes)
			return false;
	}
	return true;
}

double t1_encode_cblk(t1_info *t1, tcd_cblk_enc_t *cblk, uint32_t max,
					uint8_t orient, uint32_t compno, uint32_t level, uint32_t qmfbid,
					double stepsize, uint32_t cblksty, uint32_t numcomps,
					const double *mct_norms, uint32_t mct_numcomps, bool doRateControl) {
	if (!t1_code_block_enc_allocate(cblk))
		return 0;

	auto mqc = &t1->mqc;
	mqc_init_enc(mqc, cblk->data);

	uint32_t passno;
	int32_t bpno;
	uint32_t passtype;
	int32_t nmsedec = 0;
	uint8_t type = T1_TYPE_MQ;
	double tempwmsedec;

	mqc->lut_ctxno_zc_orient = lut_ctxno_zc + (orient << 9);
	cblk->numbps = 0;
	if (max) {
		uint32_t temp = (uint32_t) int_floorlog2(max) + 1;
		if (temp <= T1_NMSEDEC_FRACBITS)
			cblk->numbps = 0;
		else
			cblk->numbps = temp - T1_NMSEDEC_FRACBITS;
	}
	if (cblk->numbps == 0) {
		cblk->totalpasses = 0;
		return 0;
	}

	bpno = (int32_t) (cblk->numbps - 1);
	passtype = 2;

	mqc_resetstates(mqc);
	mqc_init_enc(mqc, cblk->data);

	double cumwmsedec = 0.0;
	for (passno = 0; bpno >= 0; ++passno) {
		tcd_pass_t *pass = &cblk->passes[passno];
		type = ((bpno < ((int32_t) (cblk->numbps) - 4)) &&
				(passtype < 2)	&&
				(cblksty & J2K_CCP_CBLKSTY_LAZY)) ? T1_TYPE_RAW : T1_TYPE_MQ;

		/* If the previous pass was terminating, we need to reset the encoder */
		if (passno > 0 && cblk->passes[passno - 1].term) {
			if (type == T1_TYPE_RAW)
				mqc_bypass_init_enc(mqc);
			else
				mqc_restart_init_enc(mqc);
		}

		switch (passtype) {
		case 0:
			t1_enc_sigpass(t1, bpno, &nmsedec, type, cblksty);
			break;
		case 1:
			t1_enc_refpass(t1, bpno, &nmsedec, type);
			break;
		case 2:
			t1_enc_clnpass(t1, bpno, &nmsedec, cblksty);
			if (cblksty & J2K_CCP_CBLKSTY_SEGSYM)
				mqc_segmark_enc(mqc);
			break;
		}

		if (doRateControl) {
			tempwmsedec = t1_getwmsedec(nmsedec, compno, level, orient, bpno,
					qmfbid, stepsize, numcomps, mct_norms, mct_numcomps);
			cumwmsedec += tempwmsedec;
			pass->distortiondec = cumwmsedec;
		}

		if (t1_enc_is_term_pass(cblk, cblksty, bpno, passtype)) {
			/* If it is a terminated pass, terminate it */
			if (type == T1_TYPE_RAW) {
				mqc_bypass_flush_enc(mqc, cblksty & J2K_CCP_CBLKSTY_PTERM);
			} else {
				if (cblksty & J2K_CCP_CBLKSTY_PTERM)
					mqc_erterm_enc(mqc);
				else
					mqc_flush(mqc);
			}
			pass->term = 1;
			pass->rate = mqc_numbytes(mqc);
		} else {
			/* Non terminated pass */
			// correction term is used for non-terminated passes,
			// to ensure that maximal bits are
			// extracted from the partial segment when code block
			// is truncated at this pass
			// See page 498 of Taubman and Marcellin for more details
			// note: we add 1 because rates for non-terminated passes
			// are based on mqc_numbytes(mqc),
			// which is always 1 less than actual rate
			uint32_t rate_extra_bytes;
			if (type == T1_TYPE_RAW) {
				rate_extra_bytes = mqc_bypass_get_extra_bytes(mqc,
						(cblksty & J2K_CCP_CBLKSTY_PTERM));
			} else {
				rate_extra_bytes = 4 + 1;
				if (mqc->ct < 5)
					rate_extra_bytes++;
			}
			pass->term = 0;
			pass->rate = mqc_numbytes(mqc) + rate_extra_bytes;
		}

		if (++passtype == 3) {
			passtype = 0;
			bpno--;
		}

		/* Code-switch "RESET" */
		if (cblksty & J2K_CCP_CBLKSTY_RESET)
			mqc_reset_enc(mqc);
	}

	cblk->totalpasses = passno;

	if (cblk->totalpasses) {
		/* Make sure that pass rates are increasing */
		uint32_t last_pass_rate = mqc_numbytes(mqc);
		for (passno = cblk->totalpasses; passno > 0;) {
			tcd_pass_t *pass = &cblk->passes[--passno];
			if (pass->rate > last_pass_rate)
				pass->rate = last_pass_rate;
			else
				last_pass_rate = pass->rate;
		}
	}

	for (passno = 0; passno < cblk->totalpasses; passno++) {
		auto pass = cblk->passes + passno;

		/* Prevent generation of FF as last data byte of a pass*/
		/* For terminating passes, the flushing procedure ensured this already */
		assert(pass->rate > 0);
		if (cblk->data[pass->rate - 1] == 0xFF)
			pass->rate--;
		pass->len = pass->rate - (passno == 0 ? 0 : cblk->passes[passno - 1].rate);
	}
	return cumwmsedec;
}

}
