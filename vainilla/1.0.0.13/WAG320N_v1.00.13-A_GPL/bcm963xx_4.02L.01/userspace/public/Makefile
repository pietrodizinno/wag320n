#***********************************************************************
#
#  Copyright (c) 2006  Broadcom Corporation
#  All Rights Reserved
#  No portions of this material may be reproduced in any form without the
#  written permission of:
#          Broadcom Corporation
#          16215 Alton Parkway
#          Irvine, California 92619
#  All information contained in this document is Broadcom Corporation
#  company private, proprietary, and trade secret.
#
#***********************************************************************/

#
# You probably don't need to modify anything in this Makefile.
#



#
# This is the list of directories to traverse.  
#
PUBLIC_DIRS =


#
# We need to declare these directory names as phony targets,
# otherwise make will think the directories themselves are
# the targets and will never execute the cmds.
# See GNU Make, 4.6 
#
.PHONY : $(PUBLIC_DIRS)
all    : $(PUBLIC_DIRS)


#
# Generic rule for making the dirs
#
$(PUBLIC_DIRS):
	$(MAKE) -C $@


#
# Cleaning rules
# See example in GNU Make, 4.6
#
clean: 
	for dir in $(PUBLIC_DIRS); do \
		$(MAKE) -C $$dir clean; \
	done



