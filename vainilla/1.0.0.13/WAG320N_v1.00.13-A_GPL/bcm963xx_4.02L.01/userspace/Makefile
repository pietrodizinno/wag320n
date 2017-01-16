#
# This is the list of directories.  But we cannot do a simple
# traversal into each directory because we need to build all libraries
# before building any of the apps.  See the "all" target.
#
USERSPACE_DIRS = public



#
# We need to declare these directory names as phony targets,
# otherwise make will think the directories themselves are
# the targets and will never execute the cmds.
# See GNU Make, 4.6 
#
.PHONY : $(USERSPACE_DIRS)


all    : 


public-include:
	$(MAKE) -C public/include

private-libs:

public-libs:
	$(MAKE) -C public/libs

gpl-apps:
	$(MAKE) -C gpl/apps

public-apps:
	$(MAKE) -C public/apps

private-apps:


#
# Generic rule for making the dirs
# You can still make an entire directory at once, but be careful
# of the dependencies, you need to make all the libs before you
# can make any of the apps.
#
$(USERSPACE_DIRS):
	$(MAKE) -C $@



#
# Cleaning rules
# See example in GNU Make, 4.6
#
clean: 
	for dir in $(USERSPACE_DIRS); do \
		$(MAKE) -C $$dir clean; \
	done
