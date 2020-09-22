/*
 * Copyright 2019 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file rtmforward.cpp
 * @brief It defines the forward kernel function
 */

#include "xf_blas.hpp"
#include "rtm.hpp"

using namespace xf::rtm;
typedef RTM3D<RTM_dataType, RTM_order, RTM_maxZ, RTM_maxY, RTM_MaxB, RTM_nPE> RTM_TYPE;
typedef RTM_TYPE::t_PairInType RTM_pairType;
typedef RTM_TYPE::t_InType RTM_vtType;
typedef RTM_TYPE::t_UpbInType RTM_upbType;

typedef xf::blas::WideType<RTM_dataType, RTM_parEntries> RTM_wideType;
typedef RTM_wideType::t_TypeInt RTM_interface;

void forward(RTM_TYPE s[RTM_numFSMs],
             const unsigned int p_t,
             const RTM_dataType* p_src,
             const RTM_interface* p_v2dt2,
             RTM_upbType* p_upb,
             RTM_interface* p_p0,
             RTM_interface* p_p1) {
    const unsigned int l_upbSize = s[0].getX() * s[0].getY();
    const int l_entries = s[0].getCube();
    const int l_size = s[0].getX() * s[0].getY() * s[0].getZ();
    const int l_sizeVt = l_size / RTM_parEntries;
    const int l_sizeP = l_size / RTM_parEntries * 2;
    const unsigned int RTM_pairTypeWidth = RTM_TYPE::t_PairType::t_TypeWidth;
    const unsigned int RTM_vtTypeWidth = RTM_TYPE::t_WideType::t_TypeWidth;
    const unsigned int RTM_multi = RTM_wideType::t_TypeWidth / RTM_pairTypeWidth;

    hls::stream<RTM_vtType> l_vt_in, l_vt_out;

    hls::stream<RTM_upbType> l_upb[RTM_numFSMs];
#pragma HLS ARRAY_PARTITION variable = l_upb complete dim = 1

    hls::stream<RTM_interface> l_pin;
#pragma HLS stream variable = l_pin depth = 4
    hls::stream<RTM_interface> l_pout;
#pragma HLS stream variable = l_pout depth = 4
    hls::stream<RTM_interface> l_v2dt2;
#pragma HLS stream variable = l_v2dt2 depth = 4

    hls::stream<RTM_pairType> l_p[RTM_numFSMs + 1];
#pragma HLS ARRAY_PARTITION variable = l_p complete dim = 1

#pragma HLS DATAFLOW

    xf::blas::mem2stream<RTM_interface>(l_sizeP, p_p0, l_pin);
    wide2stream<RTM_pairTypeWidth, RTM_multi>(l_sizeP, l_pin, l_p[0]);

    xf::blas::mem2stream(l_sizeVt, p_v2dt2, l_v2dt2);
    wide2stream<RTM_vtTypeWidth, RTM_multi << 1>(l_sizeVt, l_v2dt2, l_vt_in);

#if RTM_numFSMs == 1
    s[0].forward(p_src[0], l_upb[0], l_vt_in, l_vt_out, l_p[0], l_p[1]);
#else
    hls::stream<RTM_vtType> l_vt[RTM_numFSMs - 1];
#pragma HLS ARRAY_PARTITION variable = l_vt complete dim = 1
#pragma HLS stream depth = RTM_TYPE::t_FifoDepth variable = l_vt
#pragma HLS RESOURCE variable = l_vt core = fifo_uram
    s[0].forward(p_src[0], l_upb[0], l_vt_in, l_vt[0], l_p[0], l_p[1]);
    for (int i = 1; i < RTM_numFSMs - 1; i++) {
#pragma HLS UNROLL
        s[i].forward(p_src[i], l_upb[i], l_vt[i - 1], l_vt[i], l_p[i], l_p[i + 1]);
    }
    s[RTM_numFSMs - 1].forward(p_src[RTM_numFSMs - 1], l_upb[RTM_numFSMs - 1], l_vt[RTM_numFSMs - 2], l_vt_out,
                               l_p[RTM_numFSMs - 1], l_p[RTM_numFSMs]);
#endif
    dataConsumer(l_entries, l_vt_out);

    RTM_TYPE::saveUpb<RTM_numFSMs>(l_upbSize, p_t, l_upb, p_upb);
    stream2wide<RTM_pairTypeWidth, RTM_multi>(l_sizeP, l_p[RTM_numFSMs], l_pout);
    xf::blas::stream2mem<RTM_interface>(l_sizeP, l_pout, p_p1);
}

/**
 * @brief rfmforward kernel function
 *
 * @param p_z is the number of grids along z
 * @param p_y is the number of grids along y
 * @param p_x is the number of grids along x
 * @param p_t is the number of detecting time parititons
 *
 * @param p_srcz is the source z coordinate
 * @param p_srcy is the source y coordinate
 * @param p_srcx is the source x coordinate
 * @param p_src is the source wavefiled
 *
 * @param p_coefz is the laplacian z-direction coefficients
 * @param p_coefy is the laplacian y-direction coefficients
 * @param p_coefx is the laplacian x-direction coefficients
 * @param p_taperz is the absorbing factor along z
 * @param p_tapery is the absorbing factor along y
 * @param p_taperx is the absorbing factor along x
 *
 * @param p_v2dt2 is the velocity model v^2 * dt^2
 *
 * @param p_p0 is the first input memory of source wavefield
 * @param p_p1 is the second input memory of source wavefield
 * @param p_upb is the uppper bounday wavefiled
 */
extern "C" void rtmforward(const unsigned int p_z,
                           const unsigned int p_y,
                           const unsigned int p_x,
                           const unsigned int p_t,
                           const unsigned int p_srcz,
                           const unsigned int p_srcy,
                           const unsigned int p_srcx,
                           const RTM_dataType* p_src,
                           const RTM_dataType* p_coefz,
                           const RTM_dataType* p_coefy,
                           const RTM_dataType* p_coefx,
                           const RTM_dataType* p_taperz,
                           const RTM_dataType* p_tapery,
                           const RTM_dataType* p_taperx,
                           const RTM_interface* p_v2dt2,
                           RTM_interface* p_p0,
                           RTM_interface* p_p1,
                           RTM_upbType* p_upb) {
#pragma HLS INTERFACE s_axilite port = p_x bundle = control
#pragma HLS INTERFACE s_axilite port = p_y bundle = control
#pragma HLS INTERFACE s_axilite port = p_z bundle = control
#pragma HLS INTERFACE s_axilite port = p_t bundle = control
#pragma HLS INTERFACE s_axilite port = p_srcx bundle = control
#pragma HLS INTERFACE s_axilite port = p_srcy bundle = control
#pragma HLS INTERFACE s_axilite port = p_srcz bundle = control
#pragma HLS INTERFACE s_axilite port = p_src bundle = control
#pragma HLS INTERFACE s_axilite port = p_coefx bundle = control
#pragma HLS INTERFACE s_axilite port = p_coefy bundle = control
#pragma HLS INTERFACE s_axilite port = p_coefz bundle = control
#pragma HLS INTERFACE s_axilite port = p_taperx bundle = control
#pragma HLS INTERFACE s_axilite port = p_tapery bundle = control
#pragma HLS INTERFACE s_axilite port = p_taperz bundle = control
#pragma HLS INTERFACE s_axilite port = p_v2dt2 bundle = control
#pragma HLS INTERFACE s_axilite port = p_p0 bundle = control
#pragma HLS INTERFACE s_axilite port = p_p1 bundle = control
#pragma HLS INTERFACE s_axilite port = p_upb bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

#pragma HLS INTERFACE m_axi port = p_coefx offset = slave bundle = gmemDDR
#pragma HLS INTERFACE m_axi port = p_coefy offset = slave bundle = gmemDDR
#pragma HLS INTERFACE m_axi port = p_coefz offset = slave bundle = gmemDDR
#pragma HLS INTERFACE m_axi port = p_taperx offset = slave bundle = gmemDDR
#pragma HLS INTERFACE m_axi port = p_tapery offset = slave bundle = gmemDDR
#pragma HLS INTERFACE m_axi port = p_taperz offset = slave bundle = gmemDDR
#pragma HLS INTERFACE m_axi port = p_src offset = slave bundle = gmemDDR
#pragma HLS INTERFACE m_axi port = p_upb offset = slave bundle = gmemDDR

#pragma HLS INTERFACE m_axi port = p_p0 offset = slave bundle = gmem_p0
#pragma HLS INTERFACE m_axi port = p_p1 offset = slave bundle = gmem_p1
#pragma HLS INTERFACE m_axi port = p_v2dt2 offset = slave bundle = gmem_v2dt2

    RTM_TYPE l_s[RTM_numFSMs];
#pragma HLS ARRAY_PARTITION variable = l_s complete dim = 1
    RTM_dataType l_src[RTM_numFSMs];
#pragma HLS ARRAY_PARTITION variable = l_src complete dim = 1
    for (int i = 0; i < RTM_numFSMs; i++) {
        l_s[i].setDim(p_z, p_y, p_x);
        l_s[i].setCoef(p_coefz, p_coefy, p_coefx);
        l_s[i].setTaper(p_taperz, p_tapery, p_taperx);
        l_s[i].setSrc(p_srcz, p_srcy, p_srcx);
    }
    for (int t = 0; t < p_t / RTM_numFSMs; t++) {
        for (int i = 0; i < RTM_numFSMs; i++) l_src[i] = p_src[t * RTM_numFSMs + i];

        forward(l_s, t, l_src, p_v2dt2, p_upb, p_p0, p_p1);
    }
}
