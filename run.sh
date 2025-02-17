#!/bin/bash

# Set required environment variables
export __NV_PRIME_RENDER_OFFLOAD=1
export __GLX_VENDOR_LIBRARY_NAME=nvidia
export __VK_LAYER_NV_optimus=NVIDIA_only
export DRI_PRIME=1

# Add OpenCL ICD configuration
export OCL_ICD_VENDORS=/etc/OpenCL/vendors
export OPENCL_VENDOR_PATH=/etc/OpenCL/vendors
export CUDA_VISIBLE_DEVICES=0
export OCL_ICD_FILENAMES=/etc/OpenCL/vendors/nvidia.icd
export LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:/usr/local/cuda/lib64:$LD_LIBRARY_PATH

# Run the application
./particle_system "$@"
