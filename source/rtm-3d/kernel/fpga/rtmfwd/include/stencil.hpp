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

#ifndef XF_RTM_STENCIL3D_HPP
#define XF_RTM_STENCIL3D_HPP

/**
 * @file stencil.hpp
 * @brief class Stencil3D is defined here, it provides L1 primary methods for
 * 2D-stencil kernels
 */

#ifndef __SYNTHESIS__
#include <cassert>
#endif
#include <cstring>
#include "hls_stream.h"
#include "dataMover.hpp"

namespace xf {
namespace rtm {

/**
 * @brief Stencil3D class defines isotrophic laplacian operation and time
 * iterations function
 *
 * @tparam t_DataType the basic wavefield datatype
 * @tparam t_Order is the spatial discretization order
 * @tparam t_MaxDimZ is the maximum depth this kernel can process
 * @tparam t_MaxDimY is the maximum width this kernel can process
 * @tparam t_PE is the number of processing elements
 */

template <typename t_DataType, int t_Order, int t_MaxDimZ = 128, int t_MaxDimY = 128, int t_PE = 1>
class Stencil3D { // column major

    inline int BOUND_ROT(int x) { return x >= t_Order ? x - t_Order : x; }

   public:
    static const unsigned int t_HalfOrder = t_Order >> 1;
    static const unsigned int t_NumData = 2;
    static const unsigned int t_FifoDepth = t_HalfOrder * t_MaxDimY * t_MaxDimZ / t_PE + 156;
    typedef blas::WideType<t_DataType, t_PE> t_WideType;
    typedef typename t_WideType::t_TypeInt t_InType;
    typedef blas::WideType<t_WideType, t_NumData, t_WideType::t_TypeWidth> t_PairType;
    typedef typename t_PairType::t_TypeInt t_PairInType;

    Stencil3D() {}
    /**
     * @brief setCoef set the coefficients for stencil kernel
     *
     * @param p_coefz is the coefficents along z-direction
     * @param p_coefx is the coefficents along x-direction
     */
    void setCoef(const t_DataType* p_coefz, const t_DataType* p_coefy, const t_DataType* p_coefx) {
#pragma HLS ARRAY_PARTITION variable = m_coefx dim = 1 complete
#pragma HLS ARRAY_PARTITION variable = m_coefy dim = 1 complete
#pragma HLS ARRAY_PARTITION variable = m_coefz dim = 1 complete
        for (int i = 0; i < t_HalfOrder + 1; i++) {
#pragma HLS PIPELINE
            m_coefx[i] = p_coefx[i];
        }
        for (int i = 0; i < t_HalfOrder + 1; i++) {
#pragma HLS PIPELINE
            m_coefy[i] = p_coefy[i];
        }
        for (int i = 0; i < t_HalfOrder + 1; i++) {
#pragma HLS PIPELINE
            m_coefz[i] = p_coefz[i];
        }
    }
    /**
     * @brief setDim set the dimension of the wavefields
     *
     * @param p_z is the dimension of z-direction
     * @param p_x is the dimension of x-direction
     */
    void setDim(const unsigned int p_z, const unsigned int p_y, const unsigned int p_x) {
#ifndef __SYNTHESIS__
        assert(p_z % t_PE == 0);
        assert(p_z / t_PE <= t_MaxDimZ / t_PE);
        assert(p_y <= t_MaxDimY);
        assert(t_HalfOrder % t_PE == 0);
#endif

        m_x = p_x;
        m_y = p_y;
        m_z = p_z;
        m_zPE = m_z / t_PE;
        m_cube = m_x * m_y * m_zPE;
    }

    /**
     * @brief laplacian computes the laplacian of an given wavefield
     *
     * @param p_in is the input wavefield stream
     * @param p_pin is a copy of p_in
     * @param p_pout is a stream of laplacian results
     */
    void laplacian(hls::stream<t_InType>& p_in, hls::stream<t_InType>& p_pin, hls::stream<t_InType>& p_out) {
        constexpr int l_orderPE = t_Order / t_PE;

#ifndef __SYNTHESIS__
        static t_InType l_sliceBuffer[t_Order][t_MaxDimY][t_MaxDimZ / t_PE];
#else
        t_InType l_sliceBuffer[t_Order][t_MaxDimY][t_MaxDimZ / t_PE];
#pragma HLS ARRAY_PARTITION variable = l_sliceBuffer dim = 1 complete
#pragma HLS RESOURCE variable = l_sliceBuffer core = RAM_2P_URAM

#endif

        t_InType l_lineBuffer[t_Order + 1][t_Order][t_MaxDimZ / t_PE];
#pragma HLS ARRAY_PARTITION variable = l_lineBuffer dim = 1 complete
#pragma HLS ARRAY_PARTITION variable = l_lineBuffer dim = 2 complete

        t_DataType l_crossZ[t_Order + t_PE];
#pragma HLS ARRAY_PARTITION variable = l_crossZ dim = 1 complete
        t_DataType l_crossY[t_Order + 1][t_HalfOrder + t_PE];
#pragma HLS ARRAY_PARTITION variable = l_crossY dim = 0 complete
        t_DataType l_crossX[t_Order + 1][t_HalfOrder + t_PE];
#pragma HLS ARRAY_PARTITION variable = l_crossX dim = 0 complete

#ifndef __SYNTHESIS__
        int count = 0;
#endif
        for (int i = 0, i_buf = 0, j_buf = 0, j = 0, k = 0, t = 0;
             t < (this->m_cube + t_HalfOrder * m_zPE * m_y + t_HalfOrder * m_zPE + t_HalfOrder / t_PE); t++) {
#pragma HLS PIPELINE
#pragma HLS DEPENDENCE variable = l_sliceBuffer array inter false
            t_InType l_in = 0;
            if (i < m_x) l_in = p_in.read();

            for (int o = 0; o < t_Order; o++)
#pragma HLS UNROLL
                l_crossZ[o] = l_crossZ[o + t_PE];

            t_WideType l_zBuf = l_lineBuffer[t_HalfOrder][BOUND_ROT(j_buf + t_HalfOrder)][k];

            for (int pe = 0; pe < t_PE; pe++)
#pragma HLS UNROLL
                l_crossZ[t_Order + pe] = l_zBuf[pe];

            for (int o = 0; o < t_Order + 1; o++)
#pragma HLS UNROLL
                for (int s = 0; s < t_HalfOrder; s++) {
#pragma HLS UNROLL
                    l_crossX[o][s] = l_crossX[o][s + t_PE];
                    l_crossY[o][s] = l_crossY[o][s + t_PE];
                }

            for (int o = 0; o < t_Order; o++) {
#pragma HLS UNROLL
                t_WideType l_yBuf = l_lineBuffer[t_HalfOrder][BOUND_ROT(j_buf + o)][k];
                for (int pe = 0; pe < t_PE; pe++) {
#pragma HLS UNROLL
                    l_crossY[o][t_HalfOrder + pe] = l_yBuf[pe];
                }
            }

            t_WideType l_yBuf = l_sliceBuffer[BOUND_ROT(i_buf + t_HalfOrder)][j][k];
            for (int pe = 0; pe < t_PE; pe++) {
#pragma HLS UNROLL
                l_crossY[t_Order][t_HalfOrder + pe] = l_yBuf[pe];
            }

            for (int o = 0; o < t_Order + 1; o++) {
#pragma HLS UNROLL
                t_WideType l_xBuf = l_lineBuffer[o][BOUND_ROT(j_buf + t_HalfOrder)][k];
                for (int pe = 0; pe < t_PE; pe++) {
#pragma HLS UNROLL

                    l_crossX[o][t_HalfOrder + pe] = l_xBuf[pe];
                }
            }

            for (int o = 0; o < t_Order; o++) l_lineBuffer[o][j_buf][k] = l_sliceBuffer[BOUND_ROT(i_buf + o)][j][k];

            l_lineBuffer[t_Order][j_buf][k] = l_in;
            l_sliceBuffer[i_buf][j][k] = l_in;

            t_WideType l_sum;

            for (int pe = 0; pe < t_PE; pe++) {
#pragma HLS UNROLL
                t_DataType l_z = l_crossZ[t_HalfOrder + pe] * m_coefz[t_HalfOrder];
                t_DataType l_y = l_crossX[t_HalfOrder][pe] * m_coefy[t_HalfOrder];
                t_DataType l_x = l_crossX[t_HalfOrder][pe] * m_coefx[t_HalfOrder];
                for (int o = 0; o < t_HalfOrder; o++) {
#pragma HLS UNROLL
                    l_z += (l_crossZ[o + pe] + l_crossZ[t_Order - o + pe]) * m_coefz[o];
                    l_y += (l_crossY[o][pe] + l_crossY[t_Order - o][pe]) * m_coefy[o];
                    l_x += (l_crossX[o][pe] + l_crossX[t_Order - o][pe]) * m_coefx[o];
                }
                l_sum[pe] = l_x + l_y + l_z;
            }
            if (i < t_Order || j < t_Order || k < t_Order / t_PE || i >= m_x) l_sum = 0;
            if (i > t_HalfOrder ||
                (i == t_HalfOrder && (j > t_HalfOrder || (j == t_HalfOrder && k >= t_HalfOrder / t_PE)))) {
                t_WideType l_pin;
                for (int pe = 0; pe < t_PE; pe++) {
#pragma HLS UNROLL
#ifndef __SYNTHESIS__
                    /*
                    if (l_crossZ[t_HalfOrder + pe] != l_crossX[t_HalfOrder][pe]) {
                        std::cout << l_crossZ[t_HalfOrder + pe] << ' ' << l_crossX[t_HalfOrder][pe] << std::endl;
                    }
                    */
                    assert(l_crossZ[t_HalfOrder + pe] == l_crossX[t_HalfOrder][pe]);
                    assert(l_crossX[t_HalfOrder][pe] == l_crossY[t_HalfOrder][pe]);
#endif
                    l_pin[pe] = l_crossZ[t_HalfOrder + pe];
                }
                p_pin.write(l_pin);
                p_out.write(l_sum);
#ifndef __SYNTHESIS__
                count++;
#endif
            }

            if (k == this->m_zPE - 1 && j == this->m_y - 1) {
                k = 0;
                j = 0;
                i++;
                i_buf = BOUND_ROT(++i_buf);
                j_buf = BOUND_ROT(++j_buf);
            } else if (k == this->m_zPE - 1) {
                k = 0;
                j++;
                j_buf = BOUND_ROT(++j_buf);
            } else
                k++;
        }
#ifndef __SYNTHESIS__
        assert(count == m_cube);
#endif
    }

    void laplacian(hls::stream<t_InType>& p_in, hls::stream<t_InType>& p_out) {
#pragma HLS DATAFLOW
        hls::stream<t_InType> l_pin;
        laplacian(p_in, l_pin, p_out);
        dataConsumer(m_cube, l_pin);
    }

    void merge(hls::stream<t_InType>& p_in0, hls::stream<t_InType>& p_in1, hls::stream<t_PairInType>& p_out) {
        for (int i = 0; i < m_cube; i++) {
#pragma HLS PIPELINE
            t_PairType l_out;
            l_out[0] = p_in0.read();
            l_out[1] = p_in1.read();
            p_out.write(l_out);
        }
    }

    void split(hls::stream<t_PairInType>& p_in, hls::stream<t_InType>& p_out0, hls::stream<t_InType>& p_out1) {
        for (int i = 0; i < m_cube; i++) {
#pragma HLS PIPELINE
            t_PairType l_in = p_in.read();
            p_out0.write(l_in[0]);
            p_out1.write(l_in[1]);
        }
    }

    void calculate(hls::stream<t_InType>& p_p0,
                   hls::stream<t_InType>& p_p1,
                   hls::stream<t_InType>& p_lap,
                   hls::stream<t_InType>& p_v2dt2,
                   hls::stream<t_InType>& p_cpvt,
                   hls::stream<t_InType>& p_out0,
                   hls::stream<t_InType>& p_out1,
                   bool p_sw = false) {
        if (p_sw) {
            for (int i = 0; i < m_cube; i++) {
#pragma HLS PIPELINE
                t_InType l_vt = p_v2dt2.read();
                t_InType s_p0 = p_p0.read();
                t_InType s_p1 = p_p1.read();
                t_InType s_lap = p_lap.read();
                p_out0.write(s_p1);
                p_out1.write(s_p0);
                p_cpvt.write(l_vt);
            }
        } else {
            for (int i = 0; i < m_cube; i++) {
#pragma HLS PIPELINE
                t_WideType l_w;
                t_WideType l_vt = p_v2dt2.read();
                t_WideType s_p0 = p_p0.read();
                t_WideType s_p1 = p_p1.read();
                t_WideType s_lap = p_lap.read();
                for (int pe = 0; pe < t_PE; pe++) {
#pragma HLS UNROLL
                    l_w[pe] = 2 * s_p1[pe] - s_p0[pe] + l_vt[pe] * s_lap[pe];
                }
                p_out0.write(s_p1);
                p_out1.write(l_w);
                p_cpvt.write(l_vt);
            }
        }
    }

    /**
     * @brief propagate computes the time iteration for FDTD
     *
     * @param p_v2dt2 is the pow(v * dt, 2)
     * @param p_cpvt is a copy of p_v2dt2
     * @param p_i0 is a stream of input wavefield
     * @param p_i1 is a stream of input wavefield
     * @param p_o0 is a stream of output wavefield
     * @param p_o1 is a stream of output wavefield
     */
    void propagate(hls::stream<t_InType>& p_v2dt2,
                   hls::stream<t_InType>& p_cpvt,
                   hls::stream<t_InType>& p_i0,
                   hls::stream<t_InType>& p_i1,
                   hls::stream<t_InType>& p_o0,
                   hls::stream<t_InType>& p_o1,
                   bool p_sw = false) {
#pragma HLS DATAFLOW
        hls::stream<t_InType> l_ps, l_lap;

        laplacian(p_i1, l_ps, l_lap);
        calculate(p_i0, l_ps, l_lap, p_v2dt2, p_cpvt, p_o0, p_o1, p_sw);
    }

    /**
     * @brief propagate computes the time iteration for FDTD
     *
     * @param p_v2dt2 is the pow(v * dt, 2)
     * @param p_cpvt is a copy of p_v2dt2
     * @param p_in is a stream of input wavefield
     * @param p_pout is a stream of output wavefield
     */
    void propagate(hls::stream<t_InType>& p_v2dt2,
                   hls::stream<t_InType>& p_cpvt,
                   hls::stream<t_PairInType>& p_in,
                   hls::stream<t_PairInType>& p_out,
                   bool p_sw = false) {
#pragma HLS DATAFLOW
        hls::stream<t_InType> l_p0, l_p1, l_ps, l_lap, l_out0, l_out1;

#pragma HLS stream variable = l_p0 depth = t_FifoDepth
#pragma HLS RESOURCE variable = l_p0 core = fifo_uram

        split(p_in, l_p0, l_p1);
        propagate(p_v2dt2, p_cpvt, l_p0, l_p1, l_out0, l_out1, p_sw);
        merge(l_out0, l_out1, p_out);
    }

    inline unsigned int getX() const {
#pragma HLS INLINE
        return m_x;
    }

    inline unsigned int getY() const {
#pragma HLS INLINE
        return m_y;
    }

    inline unsigned int getZ() const {
#pragma HLS INLINE
        return m_z;
    }

    inline unsigned int getCube() const {
#pragma HLS INLINE
        return m_cube;
    }

   protected:
    t_DataType m_coefz[t_HalfOrder + 1];
    t_DataType m_coefy[t_HalfOrder + 1];
    t_DataType m_coefx[t_HalfOrder + 1];

    unsigned int m_x;
    unsigned int m_y;
    unsigned int m_z;
    unsigned int m_cube;
    unsigned int m_zPE;
};
}
}
#endif
