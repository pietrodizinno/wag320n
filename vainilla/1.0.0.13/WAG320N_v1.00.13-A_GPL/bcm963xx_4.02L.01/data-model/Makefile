#***********************************************************************
#
#  Copyright (c) 2007  Broadcom Corporation
#  All Rights Reserved
#
# 
# 
# This program is the proprietary software of Broadcom Corporation and/or its 
# licensors, and may only be used, duplicated, modified or distributed pursuant 
# to the terms and conditions of a separate, written license agreement executed 
# between you and Broadcom (an "Authorized License").  Except as set forth in 
# an Authorized License, Broadcom grants no license (express or implied), right 
# to use, or waiver of any kind with respect to the Software, and Broadcom 
# expressly reserves all rights in and to the Software and all intellectual 
# property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE 
# NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY 
# BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE. 
# 
# Except as expressly set forth in the Authorized License, 
# 
# 1. This program, including its structure, sequence and organization, 
#    constitutes the valuable trade secrets of Broadcom, and you shall use 
#    all reasonable efforts to protect the confidentiality thereof, and to 
#    use this information only in connection with your use of Broadcom 
#    integrated circuit products.  (Note this clause prohibits you from 
#    linking, either statically or dynamically, this program with any software 
#    that is licensed under the GPL, as the terms of the GPL would force you 
#    to release the source code of this program, thus violating the 
#    confidentiality aspect of this clause.) 
# 
# 2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS" 
#    AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR 
#    WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH 
#    RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND 
#    ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT, 
#    FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR 
#    COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE 
#    TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR 
#    PERFORMANCE OF THE SOFTWARE. 
# 
# 3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR 
#    ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL, 
#    INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY 
#    WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN 
#    IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; 
#    OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE 
#    SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS 
#    SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY 
#    LIMITED REMEDY. 
#
#
#***********************************************************************/

#
# Set our CommEngine directory (by splitting the pwd into two words
# at /data-model and taking the first word only).
# Then include the common defines under CommEngine.
# You do not need to modify this part.
#
CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /data-model, /data-model,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))


# We only need to define BUILD_DIR, we don't need to include make.common
# in the data-model directory.


#
# Local defines for the data-model directory.
#
GENERATE = ./generate_from_dm.pl
PERL_MODULES = GenObjectNode.pm GenParamNode.pm Utils.pm
DMFILE = cms-data-model.xml


#
# Generate all the files from the data model file.
#
all: $(BUILD_DIR)/userspace/public/include/mdm_objectid.h  \
     $(BUILD_DIR)/userspace/public/include/mdm_object.h \
     $(BUILD_DIR)/userspace/public/include/mdm_validstrings.h \
     $(BUILD_DIR)/userspace/private/libs/mdm/Baseline_1.c \
     $(BUILD_DIR)/userspace/private/libs/cms_core/mdm_stringtable.c \
     $(BUILD_DIR)/userspace/private/libs/cms_core/mdm_handlerfuncstable.c \
     $(BUILD_DIR)/userspace/private/libs/cms_core/rcl.h \
     $(BUILD_DIR)/userspace/private/libs/cms_core/stl.h


$(BUILD_DIR)/userspace/public/include/mdm_objectid.h: $(DMFILE) $(GENERATE) $(PERL_MODULES)
	$(GENERATE) objectid $(BUILD_DIR) < $(DMFILE)

$(BUILD_DIR)/userspace/public/include/mdm_object.h: $(DMFILE) $(GENERATE) $(PERL_MODULES)
	$(GENERATE) object $(BUILD_DIR) < $(DMFILE)

$(BUILD_DIR)/userspace/public/include/mdm_validstrings.h: $(DMFILE) $(GENERATE) $(PERL_MODULES)
	$(GENERATE) validstrings $(BUILD_DIR) < $(DMFILE)

$(BUILD_DIR)/userspace/private/libs/cms_core/mdm_stringtable.c: $(DMFILE) $(GENERATE) $(PERL_MODULES)
	$(GENERATE) stringtable $(BUILD_DIR) < $(DMFILE)

$(BUILD_DIR)/userspace/private/libs/mdm/Baseline_1.c: $(DMFILE) $(GENERATE) $(PERL_MODULES)
	$(GENERATE) mdm $(BUILD_DIR) < $(DMFILE)

$(BUILD_DIR)/userspace/private/libs/cms_core/rcl.h $(BUILD_DIR)/userspace/private/libs/cms_core/stl.h: $(DMFILE) $(GENERATE) $(PERL_MODULES)
	$(GENERATE) prototypes $(BUILD_DIR) < $(DMFILE)

$(BUILD_DIR)/userspace/private/libs/cms_core/mdm_handlerfuncstable.c: $(DMFILE) $(GENERATE) $(PERL_MODULES)
	$(GENERATE) handlerfuncstable $(BUILD_DIR) < $(DMFILE)


clean:
	rm -f $(BUILD_DIR)/userspace/public/include/mdm_objectid.h
	rm -f $(BUILD_DIR)/userspace/public/include/mdm_object.h
	rm -f $(BUILD_DIR)/userspace/public/include/mdm_params.h
	rm -f $(BUILD_DIR)/userspace/public/include/mdm_validstrings.h
	rm -f $(BUILD_DIR)/userspace/private/libs/cms_core/mdm_stringtable.c
	rm -f $(BUILD_DIR)/userspace/private/libs/cms_core/mdm_handlerfuncstable.c
	rm -f $(BUILD_DIR)/userspace/private/libs/cms_core/rcl.h
	rm -f $(BUILD_DIR)/userspace/private/libs/cms_core/stl.h
	rm -f $(BUILD_DIR)/userspace/private/libs/mdm/*.c

