
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <Misc.hpp>
#include <RTMGrid.hpp>
#include <RTM.hpp>
#include <RTMController.hpp>
#include <RTMFPGAPlatform.hpp>

#ifdef RTM_ACC_FPGA
using namespace cl;
#include "xcl2/xcl2.hpp"
#endif
using namespace std;

void RTMFPGAPlatform::initRTMPlatform()
{
#ifdef RTM_ACC_FPGA
    if (fpgaDevice!=nullptr){
        delete fpgaDevice;
    }
    RTM_PRINT("Initializing Xilinx FPGA Platform...", rtmParam->verbose);

    /* check model dimensions*/
    assert(nt % RTM_FPGA_nFSM == 0);
    assert(plen_z % RTM_FPGA_nPEZ == 0);
    assert(plen_x % RTM_FPGA_nPEX == 0);

    /* creates fpga device, context and command queue */
    fpgaDevice = new RTMFPGADevice(rtmParam->fpga_xclbin);

    fwdKernel = new RTMForwardKernel(fpgaDevice, plen_z, plen_y, plen_x, 
        blen, blen, blen,nt,1, 
        RTM_FPGA_V2DT2_BASE,
        RTM_FPGA_P0_BASE,
        RTM_FPGA_P1_BASE,
        RTM_FPGA_PP0_BASE,
        RTM_FPGA_PP1_BASE);

#endif
}

void RTMFPGAPlatform::destroyRTMPlatform()
{
#ifdef RTM_ACC_FPGA
    if (fpgaDevice!=nullptr){
        delete fpgaDevice;
        fpgaDevice=nullptr;
    }
#endif
}

/* HBC Forward Propagation */
void RTMFPGAPlatform::rtmForwardPropagation(
        RTMShotDescriptor<RTMData_t, RTMDevPtr_t> * shotDesc,
        RTMStencil<RTMData_t,RTMDevPtr_t> *stencil,
        RTMTaperFunction<RTMData_t,RTMDevPtr_t> *rtmTaper,
        const RTMVelocityModel<RTMData_t,RTMDevPtr_t> &v2dt2Grid,
        RTMCube<RTMData_t, RTMDevPtr_t> *snap0Grid,
        RTMCube<RTMData_t, RTMDevPtr_t> *snap1Grid, 
        RTMGridCollection<RTMData_t,RTMDevPtr_t> *upbGrid)
{
#ifdef RTM_ACC_FPGA
    host_buffer_t<RTM_wideType> l_snap0, l_snap1;
    host_buffer_t<RTMData_t> * l_upb = new host_buffer_t<RTMData_t>(upbGrid->size());
    int sx = shotDesc->getSource()->getX()+blen;
    int sy = shotDesc->getSource()->getY()+blen;
    int sz = shotDesc->getSource()->getZ()+blen;
#ifdef __FPGATEST__
    readBin("debug/p.bin", v2dt2Grid.size()*sizeof(RTMData_t), l_snap0);
    readBin("debug/pp.bin", v2dt2Grid.size()*sizeof(RTMData_t), l_snap1);
    readBin("debug/upb_out.bin", upbGrid->size()*sizeof(RTMData_t), *l_upb);
#else
    RTM_PRINT("Running FPGA Foward Kernel......", rtmParam->verbose);
    /* saves intermediary files */
    writeBin(rtmParam->tmpdir+"/v2dt2.bin", v2dt2Grid.size()*sizeof(RTMData_t) ,v2dt2Grid.data());
    writeBin(rtmParam->tmpdir+"/src_s0.bin", (rtmParam->nt)*sizeof(RTMData_t) ,shotDesc->getSource()->data());
    writeBin(rtmParam->tmpdir+"/coefx.bin", (stencil->getOrder()+1)*sizeof(RTMData_t) , 
    stencil->getStencilCoefArray(RTMDim::Xdim));
    writeBin(rtmParam->tmpdir+"/coefy.bin", (stencil->getOrder()+1)*sizeof(RTMData_t) , 
    stencil->getStencilCoefArray(RTMDim::Ydim));
    writeBin(rtmParam->tmpdir+"/coefz.bin", (stencil->getOrder()+1)*sizeof(RTMData_t) , 
    stencil->getStencilCoefArray(RTMDim::Zdim));
    writeBin(rtmParam->tmpdir+"/taperx.bin", rtmParam->blen*sizeof(RTMData_t) ,rtmTaper->data());
    writeBin(rtmParam->tmpdir+"/tapery.bin", rtmParam->blen*sizeof(RTMData_t) ,rtmTaper->data());
    writeBin(rtmParam->tmpdir+"/taperz.bin", rtmParam->blen*sizeof(RTMData_t) ,rtmTaper->data());
    fwdKernel->loadData(rtmParam->tmpdir+"/");
    bool selF = (rtmParam->nt / RTM_FPGA_nFSM) % 2 == 0;
    fwdKernel->run(selF, 0, sy, sx, l_snap0, l_snap1, *l_upb);
#endif
    converter<RTM_FPGA_nPEX, RTM_FPGA_nPEZ, RTMData_t>(plen_x, plen_y, plen_z, l_snap0.data(), snap0Grid->data());
    converter<RTM_FPGA_nPEX, RTM_FPGA_nPEZ, RTMData_t>(plen_x, plen_y, plen_z, l_snap1.data(), snap1Grid->data());
    converter_upb<RTM_FPGA_nPEX, RTM_FPGA_nPEZ, RTM_FPGA_stOrder, RTMData_t>(plen_x, plen_y, 
        rtmParam->ntstep, *l_upb, upbGrid->data());
    string upbFile;
    int unxe = upbGrid->getNX();
    int unye = upbGrid->getNY();
    RTM_HBCUPB_NAME(upbFile, rtmParam->tmpdir, sx-blen, sy-blen, sz-blen, unxe, unye,
                    rtmParam->stencil_order/2, ntstep, 0, pLimits->pRank, pLimits->nProcesses);
    // save upb tmp file
    upbGrid->saveToFile(upbFile);

    remove(string(rtmParam->tmpdir+"/src_s0.bin").c_str());
    remove(string(rtmParam->tmpdir+"/coefx.bin").c_str());
    remove(string(rtmParam->tmpdir+"/coefy.bin").c_str());
    remove(string(rtmParam->tmpdir+"/coefz.bin").c_str());
    remove(string(rtmParam->tmpdir+"/taperx.bin").c_str());
    remove(string(rtmParam->tmpdir+"/tapery.bin").c_str());
    remove(string(rtmParam->tmpdir+"/taperz.bin").c_str());
    remove(string(rtmParam->tmpdir+"/v2dt2.bin").c_str());
#endif 
}

/* RBC Forward Propagation */
void RTMFPGAPlatform::rtmForwardPropagation(
        RTMShotDescriptor<RTMData_t, RTMDevPtr_t> * shotDesc,
        RTMStencil<RTMData_t,RTMDevPtr_t> *stencil,
        RTMTaperFunction<RTMData_t,RTMDevPtr_t> *rtmTaper,
        const RTMVelocityModel<RTMData_t,RTMDevPtr_t> &v2dt2Grid,
        RTMCube<RTMData_t, RTMDevPtr_t> *snap0Grid,
        RTMCube<RTMData_t, RTMDevPtr_t> *snap1Grid)
{

#ifdef RTM_ACC_FPGA
    host_buffer_t<RTM_wideType> l_snap0, l_snap1;
    host_buffer_t<RTMData_t> l_upb;
    int sx = shotDesc->getSource()->getX()+blen;
    int sy = shotDesc->getSource()->getY()+blen;
    int sz = shotDesc->getSource()->getZ()+blen;
#ifdef __FPGATEST__
    readBin("debug/p.bin", v2dt2Grid.size()*sizeof(RTMData_t), l_snap0);
    readBin("debug/pp.bin", v2dt2Grid.size()*sizeof(RTMData_t), l_snap1);
#else
    RTM_PRINT("Running FPGA Foward Kernel......", rtmParam->verbose);
    /* saves intermediary files */
    writeBin(rtmParam->tmpdir+"/v2dt2.bin", v2dt2Grid.size()*sizeof(RTMData_t) ,v2dt2Grid.data());
    writeBin(rtmParam->tmpdir+"/src_s0.bin", (rtmParam->nt)*sizeof(RTMData_t) ,shotDesc->getSource()->data());
    writeBin(rtmParam->tmpdir+"/coefx.bin", (stencil->getOrder()+1)*sizeof(RTMData_t) , 
    stencil->getStencilCoefArray(RTMDim::Xdim));
    writeBin(rtmParam->tmpdir+"/coefy.bin", (stencil->getOrder()+1)*sizeof(RTMData_t) , 
    stencil->getStencilCoefArray(RTMDim::Ydim));
    writeBin(rtmParam->tmpdir+"/coefz.bin", (stencil->getOrder()+1)*sizeof(RTMData_t) , 
    stencil->getStencilCoefArray(RTMDim::Zdim));
    writeBin(rtmParam->tmpdir+"/taperx.bin", rtmParam->blen*sizeof(RTMData_t) ,rtmTaper->data());
    writeBin(rtmParam->tmpdir+"/tapery.bin", rtmParam->blen*sizeof(RTMData_t) ,rtmTaper->data());
    writeBin(rtmParam->tmpdir+"/taperz.bin", rtmParam->blen*sizeof(RTMData_t) ,rtmTaper->data());
    fwdKernel->loadData(rtmParam->tmpdir+"/");
    bool selF = (rtmParam->nt / RTM_FPGA_nFSM) % 2 == 0;
    fwdKernel->run(selF, 0, sy, sx, l_snap0, l_snap1);
#endif
    converter<RTM_FPGA_nPEX, RTM_FPGA_nPEZ, RTMData_t>(plen_x, plen_y, plen_z, l_snap0.data(), snap0Grid->data());
    converter<RTM_FPGA_nPEX, RTM_FPGA_nPEZ, RTMData_t>(plen_x, plen_y, plen_z, l_snap1.data(), snap1Grid->data());

    remove(string(rtmParam->tmpdir+"/src_s0.bin").c_str());
    remove(string(rtmParam->tmpdir+"/coefx.bin").c_str());
    remove(string(rtmParam->tmpdir+"/coefy.bin").c_str());
    remove(string(rtmParam->tmpdir+"/coefz.bin").c_str());
    remove(string(rtmParam->tmpdir+"/taperx.bin").c_str());
    remove(string(rtmParam->tmpdir+"/tapery.bin").c_str());
    remove(string(rtmParam->tmpdir+"/taperz.bin").c_str());
    remove(string(rtmParam->tmpdir+"/v2dt2.bin").c_str());

#endif 

}

